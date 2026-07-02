#include "config.h"

#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_DEFAULT_PORT 22
#define CONFIG_DEFAULT_POLL_SECONDS 30
#define CONFIG_DEFAULT_RECURSIVE 1
#define CONFIG_DEFAULT_MAX_DEPTH 0

static int copy_string_value(char *dst, size_t dst_len, const char *src) {
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

static app_error_t copy_required_string(
    const cJSON *obj,
    const char *key,
    char *out,
    size_t out_len
) {
    const cJSON *item;

    if (obj == NULL || key == NULL || out == NULL || out_len == 0U) {
        return APP_ERR_INVALID_ARG;
    }

    item = cJSON_GetObjectItemCaseSensitive((cJSON *)obj, key);
    if (!cJSON_IsString(item) || item->valuestring == NULL || item->valuestring[0] == '\0') {
        return APP_ERR_CONFIG;
    }

    if (!copy_string_value(out, out_len, item->valuestring)) {
        return APP_ERR_CONFIG;
    }

    return APP_OK;
}

static app_error_t copy_optional_string(
    const cJSON *obj,
    const char *key,
    char *out,
    size_t out_len
) {
    const cJSON *item;

    if (obj == NULL || key == NULL || out == NULL || out_len == 0U) {
        return APP_ERR_INVALID_ARG;
    }

    item = cJSON_GetObjectItemCaseSensitive((cJSON *)obj, key);
    if (item == NULL) {
        return APP_OK;
    }

    if (!cJSON_IsString(item) || item->valuestring == NULL) {
        return APP_ERR_CONFIG;
    }

    if (!copy_string_value(out, out_len, item->valuestring)) {
        return APP_ERR_CONFIG;
    }

    return APP_OK;
}

static app_error_t copy_optional_int(const cJSON *obj, const char *key, int *out) {
    const cJSON *item;

    if (obj == NULL || key == NULL || out == NULL) {
        return APP_ERR_INVALID_ARG;
    }

    item = cJSON_GetObjectItemCaseSensitive((cJSON *)obj, key);
    if (item == NULL) {
        return APP_OK;
    }

    if (!cJSON_IsNumber(item)) {
        return APP_ERR_CONFIG;
    }

    *out = item->valueint;
    return APP_OK;
}

static app_error_t copy_optional_bool(const cJSON *obj, const char *key, int *out) {
    const cJSON *item;

    if (obj == NULL || key == NULL || out == NULL) {
        return APP_ERR_INVALID_ARG;
    }

    item = cJSON_GetObjectItemCaseSensitive((cJSON *)obj, key);
    if (item == NULL) {
        return APP_OK;
    }

    if (!cJSON_IsBool(item)) {
        return APP_ERR_CONFIG;
    }

    *out = cJSON_IsTrue(item) ? 1 : 0;
    return APP_OK;
}

static app_error_t copy_optional_string_array(
    const cJSON *obj,
    const char *key,
    char out[CONFIG_MAX_PATTERNS][CONFIG_MAX_STR],
    int *out_count
) {
    const cJSON *arr;
    int i;
    int arr_size;

    if (obj == NULL || key == NULL || out == NULL || out_count == NULL) {
        return APP_ERR_INVALID_ARG;
    }

    arr = cJSON_GetObjectItemCaseSensitive((cJSON *)obj, key);
    if (arr == NULL) {
        return APP_OK;
    }

    if (!cJSON_IsArray(arr)) {
        return APP_ERR_CONFIG;
    }

    arr_size = cJSON_GetArraySize(arr);
    if (arr_size < 0 || arr_size > CONFIG_MAX_PATTERNS) {
        return APP_ERR_CONFIG;
    }

    for (i = 0; i < arr_size; ++i) {
        const cJSON *entry = cJSON_GetArrayItem((cJSON *)arr, i);
        if (!cJSON_IsString(entry) || entry->valuestring == NULL) {
            return APP_ERR_CONFIG;
        }
        if (!copy_string_value(out[i], CONFIG_MAX_STR, entry->valuestring)) {
            return APP_ERR_CONFIG;
        }
    }

    *out_count = arr_size;
    return APP_OK;
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

static app_error_t parse_config_json(const char *json, app_config_t *out_cfg) {
    cJSON *root = NULL;
    cJSON *sftp = NULL;
    cJSON *watch = NULL;
    cJSON *download = NULL;
    cJSON *logging = NULL;
    cJSON *auth = NULL;
    app_error_t err = APP_OK;

    if (json == NULL || out_cfg == NULL) {
        return APP_ERR_INVALID_ARG;
    }

    root = cJSON_Parse(json);
    if (root == NULL || !cJSON_IsObject(root)) {
        if (root != NULL) {
            cJSON_Delete(root);
        }
        return APP_ERR_CONFIG;
    }

    sftp = cJSON_GetObjectItemCaseSensitive(root, "sftp");
    if (!cJSON_IsObject(sftp)) {
        cJSON_Delete(root);
        return APP_ERR_CONFIG;
    }

    err = copy_required_string(sftp, "host", out_cfg->host, sizeof(out_cfg->host));
    if (err != APP_OK) {
        cJSON_Delete(root);
        return err;
    }

    err = copy_optional_int(sftp, "port", &out_cfg->port);
    if (err != APP_OK) {
        cJSON_Delete(root);
        return err;
    }

    err = copy_required_string(sftp, "username", out_cfg->username, sizeof(out_cfg->username));
    if (err != APP_OK) {
        cJSON_Delete(root);
        return err;
    }

    err = copy_required_string(sftp, "remote_dir", out_cfg->remote_dir, sizeof(out_cfg->remote_dir));
    if (err != APP_OK) {
        cJSON_Delete(root);
        return err;
    }

    auth = cJSON_GetObjectItemCaseSensitive(sftp, "auth");
    if (auth != NULL) {
        if (!cJSON_IsObject(auth)) {
            cJSON_Delete(root);
            return APP_ERR_CONFIG;
        }
        err = copy_optional_string(auth, "password", out_cfg->password, sizeof(out_cfg->password));
        if (err != APP_OK) {
            cJSON_Delete(root);
            return err;
        }
        err = copy_optional_string(auth, "ssh_key_path", out_cfg->ssh_key_path, sizeof(out_cfg->ssh_key_path));
        if (err != APP_OK) {
            cJSON_Delete(root);
            return err;
        }
    } else {
        err = copy_optional_string(sftp, "password", out_cfg->password, sizeof(out_cfg->password));
        if (err != APP_OK) {
            cJSON_Delete(root);
            return err;
        }
        err = copy_optional_string(sftp, "ssh_key_path", out_cfg->ssh_key_path, sizeof(out_cfg->ssh_key_path));
        if (err != APP_OK) {
            cJSON_Delete(root);
            return err;
        }
    }

    download = cJSON_GetObjectItemCaseSensitive(root, "download");
    if (!cJSON_IsObject(download)) {
        cJSON_Delete(root);
        return APP_ERR_CONFIG;
    }

    err = copy_required_string(download, "local_root", out_cfg->local_download_root, sizeof(out_cfg->local_download_root));
    if (err != APP_OK) {
        cJSON_Delete(root);
        return err;
    }

    watch = cJSON_GetObjectItemCaseSensitive(root, "watch");
    if (watch != NULL) {
        if (!cJSON_IsObject(watch)) {
            cJSON_Delete(root);
            return APP_ERR_CONFIG;
        }

        err = copy_optional_int(watch, "poll_interval_seconds", &out_cfg->poll_interval_seconds);
        if (err != APP_OK) {
            cJSON_Delete(root);
            return err;
        }
        err = copy_optional_bool(watch, "recursive", &out_cfg->recursive);
        if (err != APP_OK) {
            cJSON_Delete(root);
            return err;
        }
        err = copy_optional_int(watch, "max_depth", &out_cfg->max_depth);
        if (err != APP_OK) {
            cJSON_Delete(root);
            return err;
        }
        err = copy_optional_string_array(
            watch,
            "include_patterns",
            out_cfg->include_patterns,
            &out_cfg->include_pattern_count
        );
        if (err != APP_OK) {
            cJSON_Delete(root);
            return err;
        }
        err = copy_optional_string_array(
            watch,
            "exclude_patterns",
            out_cfg->exclude_patterns,
            &out_cfg->exclude_pattern_count
        );
        if (err != APP_OK) {
            cJSON_Delete(root);
            return err;
        }
    }

    logging = cJSON_GetObjectItemCaseSensitive(root, "logging");
    if (logging != NULL) {
        if (!cJSON_IsObject(logging)) {
            cJSON_Delete(root);
            return APP_ERR_CONFIG;
        }
        err = copy_optional_string(logging, "level", out_cfg->log_level, sizeof(out_cfg->log_level));
        if (err != APP_OK) {
            cJSON_Delete(root);
            return err;
        }
        err = copy_optional_string(logging, "destination", out_cfg->log_destination, sizeof(out_cfg->log_destination));
        if (err != APP_OK) {
            cJSON_Delete(root);
            return err;
        }
    }

    cJSON_Delete(root);
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
    if (!copy_string_value(cfg.log_level, sizeof(cfg.log_level), "info")) {
        return APP_ERR_UNKNOWN;
    }
    if (!copy_string_value(cfg.log_destination, sizeof(cfg.log_destination), "stderr")) {
        return APP_ERR_UNKNOWN;
    }

    err = read_entire_file(path, &json);
    if (err != APP_OK) {
        return err;
    }

    err = parse_config_json(json, &cfg);
    if (err != APP_OK) {
        free(json);
        return err;
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
