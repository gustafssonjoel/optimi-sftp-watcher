#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_DEFAULT_PORT 22
#define CONFIG_DEFAULT_POLL_SECONDS 30
#define CONFIG_DEFAULT_RECURSIVE 1
#define CONFIG_DEFAULT_MAX_DEPTH 0

static const char *skip_ws(const char *s) {
    while (*s != '\0' && isspace((unsigned char)*s) != 0) {
        ++s;
    }
    return s;
}

static int copy_string(char *dst, size_t dst_len, const char *src) {
    size_t src_len;

    if (dst == NULL || src == NULL || dst_len == 0U) {
        return 0;
    }

    src_len = strlen(src);
    if (src_len >= dst_len) {
        return 0;
    }

    memcpy(dst, src, src_len + 1U);
    return 1;
}

static const char *find_key_value(const char *json, const char *key) {
    char needle[128];
    size_t key_len;
    const char *p;

    key_len = strlen(key);
    if (key_len + 3U >= sizeof(needle)) {
        return NULL;
    }

    needle[0] = '\"';
    memcpy(needle + 1, key, key_len);
    needle[key_len + 1U] = '\"';
    needle[key_len + 2U] = '\0';

    p = json;
    while ((p = strstr(p, needle)) != NULL) {
        const char *colon = skip_ws(p + key_len + 2U);
        if (*colon == ':') {
            return skip_ws(colon + 1);
        }
        p += key_len + 2U;
    }

    return NULL;
}

static int parse_json_string(const char *value, char *out, size_t out_len, const char **end_ptr) {
    size_t written = 0U;
    const char *p = value;

    if (p == NULL || out == NULL || out_len == 0U || *p != '\"') {
        return 0;
    }

    ++p;
    while (*p != '\0' && *p != '\"') {
        char c = *p;

        if (c == '\\') {
            ++p;
            if (*p == '\0') {
                return 0;
            }
            switch (*p) {
                case '\\':
                case '\"':
                case '/':
                    c = *p;
                    break;
                case 'n':
                    c = '\n';
                    break;
                case 'r':
                    c = '\r';
                    break;
                case 't':
                    c = '\t';
                    break;
                case 'b':
                    c = '\b';
                    break;
                case 'f':
                    c = '\f';
                    break;
                default:
                    return 0;
            }
        }

        if (written + 1U >= out_len) {
            return 0;
        }

        out[written++] = c;
        ++p;
    }

    if (*p != '\"') {
        return 0;
    }

    out[written] = '\0';
    if (end_ptr != NULL) {
        *end_ptr = p + 1;
    }
    return 1;
}

static int parse_json_int(const char *value, int *out) {
    char *end = NULL;
    long parsed;

    if (value == NULL || out == NULL) {
        return 0;
    }

    errno = 0;
    parsed = strtol(value, &end, 10);
    if (end == value || errno != 0) {
        return 0;
    }

    end = (char *)skip_ws(end);
    if (*end != ',' && *end != '}' && *end != ']' && *end != '\0') {
        return 0;
    }

    if (parsed < INT_MIN || parsed > INT_MAX) {
        return 0;
    }

    *out = (int)parsed;
    return 1;
}

static int parse_json_bool(const char *value, int *out) {
    if (value == NULL || out == NULL) {
        return 0;
    }

    if (strncmp(value, "true", 4) == 0) {
        *out = 1;
        return 1;
    }

    if (strncmp(value, "false", 5) == 0) {
        *out = 0;
        return 1;
    }

    return 0;
}

static int parse_json_string_array(
    const char *value,
    char out[CONFIG_MAX_PATTERNS][CONFIG_MAX_STR],
    int *out_count
) {
    const char *p = skip_ws(value);
    int count = 0;

    if (p == NULL || out == NULL || out_count == NULL || *p != '[') {
        return 0;
    }

    p = skip_ws(p + 1);
    if (*p == ']') {
        *out_count = 0;
        return 1;
    }

    while (*p != '\0') {
        if (count >= CONFIG_MAX_PATTERNS) {
            return 0;
        }

        if (!parse_json_string(p, out[count], CONFIG_MAX_STR, &p)) {
            return 0;
        }
        ++count;

        p = skip_ws(p);
        if (*p == ']') {
            *out_count = count;
            return 1;
        }
        if (*p != ',') {
            return 0;
        }
        p = skip_ws(p + 1);
    }

    return 0;
}

static int json_get_string(const char *json, const char *key, char *out, size_t out_len) {
    const char *value = find_key_value(json, key);
    if (value == NULL) {
        return 0;
    }
    return parse_json_string(value, out, out_len, NULL);
}

static int json_get_int(const char *json, const char *key, int *out) {
    const char *value = find_key_value(json, key);
    if (value == NULL) {
        return 0;
    }
    return parse_json_int(value, out);
}

static int json_get_bool(const char *json, const char *key, int *out) {
    const char *value = find_key_value(json, key);
    if (value == NULL) {
        return 0;
    }
    return parse_json_bool(value, out);
}

static int json_get_string_array(
    const char *json,
    const char *key,
    char out[CONFIG_MAX_PATTERNS][CONFIG_MAX_STR],
    int *out_count
) {
    const char *value = find_key_value(json, key);
    if (value == NULL) {
        return 0;
    }
    return parse_json_string_array(value, out, out_count);
}

static app_error_t validate_config(const app_config_t *config) {
    if (config->host[0] == '\0') {
        return APP_ERR_CONFIG;
    }
    if (config->username[0] == '\0') {
        return APP_ERR_CONFIG;
    }
    if (config->remote_dir[0] == '\0') {
        return APP_ERR_CONFIG;
    }
    if (config->local_download_root[0] == '\0') {
        return APP_ERR_CONFIG;
    }
    if (config->port <= 0 || config->port > 65535) {
        return APP_ERR_CONFIG;
    }
    if (config->poll_interval_seconds <= 0) {
        return APP_ERR_CONFIG;
    }
    if (config->max_depth < 0) {
        return APP_ERR_CONFIG;
    }
    if (config->password[0] == '\0' && config->ssh_key_path[0] == '\0') {
        return APP_ERR_CONFIG;
    }
    return APP_OK;
}

static app_error_t read_entire_file(const char *path, char **out_contents) {
    FILE *fp = NULL;
    long size;
    char *buf;

    if (path == NULL || out_contents == NULL) {
        return APP_ERR_INVALID_ARG;
    }

    fp = fopen(path, "rb");
    if (fp == NULL) {
        return APP_ERR_IO;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return APP_ERR_IO;
    }

    size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        return APP_ERR_IO;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return APP_ERR_IO;
    }

    buf = (char *)malloc((size_t)size + 1U);
    if (buf == NULL) {
        fclose(fp);
        return APP_ERR_IO;
    }

    if (size > 0 && fread(buf, 1U, (size_t)size, fp) != (size_t)size) {
        free(buf);
        fclose(fp);
        return APP_ERR_IO;
    }

    buf[size] = '\0';
    fclose(fp);
    *out_contents = buf;
    return APP_OK;
}

app_error_t config_load(const char *path, app_config_t *out_config) {
    char *json = NULL;
    app_config_t cfg;
    app_error_t err;

    if (path == NULL || out_config == NULL) {
        return APP_ERR_INVALID_ARG;
    }

    memset(&cfg, 0, sizeof(cfg));
    cfg.port = CONFIG_DEFAULT_PORT;
    cfg.poll_interval_seconds = CONFIG_DEFAULT_POLL_SECONDS;
    cfg.recursive = CONFIG_DEFAULT_RECURSIVE;
    cfg.max_depth = CONFIG_DEFAULT_MAX_DEPTH;
    if (!copy_string(cfg.log_level, sizeof(cfg.log_level), "info")) {
        return APP_ERR_UNKNOWN;
    }
    if (!copy_string(cfg.log_destination, sizeof(cfg.log_destination), "stderr")) {
        return APP_ERR_UNKNOWN;
    }

    err = read_entire_file(path, &json);
    if (err != APP_OK) {
        return err;
    }

    (void)json_get_string(json, "host", cfg.host, sizeof(cfg.host));
    (void)json_get_int(json, "port", &cfg.port);
    (void)json_get_string(json, "username", cfg.username, sizeof(cfg.username));
    (void)json_get_string(json, "password", cfg.password, sizeof(cfg.password));
    (void)json_get_string(json, "ssh_key_path", cfg.ssh_key_path, sizeof(cfg.ssh_key_path));
    (void)json_get_string(json, "remote_dir", cfg.remote_dir, sizeof(cfg.remote_dir));

    if (!json_get_string(json, "local_root", cfg.local_download_root, sizeof(cfg.local_download_root))) {
        (void)json_get_string(
            json,
            "local_download_root",
            cfg.local_download_root,
            sizeof(cfg.local_download_root)
        );
    }

    (void)json_get_int(json, "poll_interval_seconds", &cfg.poll_interval_seconds);
    (void)json_get_bool(json, "recursive", &cfg.recursive);
    (void)json_get_int(json, "max_depth", &cfg.max_depth);
    (void)json_get_string(json, "log_level", cfg.log_level, sizeof(cfg.log_level));
    (void)json_get_string(json, "destination", cfg.log_destination, sizeof(cfg.log_destination));

    if (find_key_value(json, "include_patterns") != NULL &&
        !json_get_string_array(json, "include_patterns", cfg.include_patterns, &cfg.include_pattern_count)) {
        free(json);
        return APP_ERR_CONFIG;
    }

    if (find_key_value(json, "exclude_patterns") != NULL &&
        !json_get_string_array(json, "exclude_patterns", cfg.exclude_patterns, &cfg.exclude_pattern_count)) {
        free(json);
        return APP_ERR_CONFIG;
    }

    free(json);

    err = validate_config(&cfg);
    if (err != APP_OK) {
        return err;
    }

    *out_config = cfg;
    return APP_OK;
}

void config_free(app_config_t *config) {
    (void)config;
}
