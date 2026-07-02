#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "app_error.h"
#include "config.h"

static int g_failures = 0;

typedef void (*test_fn_t)(void);

typedef struct test_case {
    const char *name;
    test_fn_t fn;
} test_case_t;

#define EXPECT_TRUE(condition)                                                            \
    do {                                                                                  \
        if (!(condition)) {                                                               \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);        \
            ++g_failures;                                                                 \
        }                                                                                 \
    } while (0)

#define EXPECT_EQ_INT(actual, expected)                                                   \
    do {                                                                                  \
        int _actual = (actual);                                                           \
        int _expected = (expected);                                                       \
        if (_actual != _expected) {                                                       \
            fprintf(                                                                      \
                stderr,                                                                   \
                "FAIL %s:%d: expected %d, got %d\n",                                    \
                __FILE__,                                                                 \
                __LINE__,                                                                 \
                _expected,                                                                \
                _actual                                                                   \
            );                                                                            \
            ++g_failures;                                                                 \
        }                                                                                 \
    } while (0)

static int write_temp_config(const char *json, char *out_path, size_t out_path_len) {
    static unsigned int counter = 0;
    char path[128];
    FILE *fp;

    if (json == NULL || out_path == NULL || out_path_len == 0U) {
        return 0;
    }

    ++counter;
    if (snprintf(path, sizeof(path), "/tmp/optimi-config-%ld-%u.json", (long)time(NULL), counter) <= 0) {
        return 0;
    }

    fp = fopen(path, "w");
    if (fp == NULL) {
        return 0;
    }

    if (fputs(json, fp) == EOF || fclose(fp) != 0) {
        unlink(path);
        return 0;
    }

    if (strlen(path) + 1U > out_path_len) {
        unlink(path);
        return 0;
    }

    memcpy(out_path, path, strlen(path) + 1U);
    return 1;
}

static app_error_t load_from_json(const char *json, app_config_t *out_cfg) {
    char path[64];
    app_error_t err;

    if (!write_temp_config(json, path, sizeof(path))) {
        return APP_ERR_IO;
    }

    err = config_load(path, out_cfg);
    unlink(path);
    return err;
}

static void test_config_rejects_missing_host(void) {
    const char *json =
        "{"
        "\"sftp\":{"
        "\"username\":\"alice\"," 
        "\"auth\":{\"password\":\"pw\"},"
        "\"remote_dir\":\"/incoming\""
        "},"
        "\"download\":{\"local_root\":\"./downloads\"}"
        "}";
    app_config_t cfg;

    EXPECT_EQ_INT(load_from_json(json, &cfg), APP_ERR_CONFIG);
}

static void test_config_rejects_missing_remote_dir(void) {
    const char *json =
        "{"
        "\"sftp\":{"
        "\"host\":\"example.com\"," 
        "\"username\":\"alice\"," 
        "\"auth\":{\"password\":\"pw\"}"
        "},"
        "\"download\":{\"local_root\":\"./downloads\"}"
        "}";
    app_config_t cfg;

    EXPECT_EQ_INT(load_from_json(json, &cfg), APP_ERR_CONFIG);
}

static void test_config_rejects_invalid_poll_interval(void) {
    const char *json =
        "{"
        "\"sftp\":{"
        "\"host\":\"example.com\"," 
        "\"username\":\"alice\"," 
        "\"auth\":{\"password\":\"pw\"},"
        "\"remote_dir\":\"/incoming\""
        "},"
        "\"watch\":{\"poll_interval_seconds\":0},"
        "\"download\":{\"local_root\":\"./downloads\"}"
        "}";
    app_config_t cfg;

    EXPECT_EQ_INT(load_from_json(json, &cfg), APP_ERR_CONFIG);
}

static void test_config_rejects_invalid_max_depth(void) {
    const char *json =
        "{"
        "\"sftp\":{"
        "\"host\":\"example.com\"," 
        "\"username\":\"alice\"," 
        "\"auth\":{\"password\":\"pw\"},"
        "\"remote_dir\":\"/incoming\""
        "},"
        "\"watch\":{\"max_depth\":-1},"
        "\"download\":{\"local_root\":\"./downloads\"}"
        "}";
    app_config_t cfg;

    EXPECT_EQ_INT(load_from_json(json, &cfg), APP_ERR_CONFIG);
}

static void test_config_rejects_empty_credentials(void) {
    const char *json =
        "{"
        "\"sftp\":{"
        "\"host\":\"example.com\"," 
        "\"username\":\"alice\"," 
        "\"remote_dir\":\"/incoming\""
        "},"
        "\"download\":{\"local_root\":\"./downloads\"}"
        "}";
    app_config_t cfg;

    EXPECT_EQ_INT(load_from_json(json, &cfg), APP_ERR_CONFIG);
}

static void test_config_accepts_password_auth_config(void) {
    const char *json =
        "{"
        "\"sftp\":{"
        "\"host\":\"example.com\"," 
        "\"port\":2022," 
        "\"username\":\"alice\"," 
        "\"auth\":{\"password\":\"secret\"},"
        "\"remote_dir\":\"/incoming\""
        "},"
        "\"watch\":{"
        "\"poll_interval_seconds\":15," 
        "\"recursive\":false," 
        "\"max_depth\":4"
        "},"
        "\"download\":{\"local_root\":\"./downloads\"}"
        "}";
    app_config_t cfg;

    EXPECT_EQ_INT(load_from_json(json, &cfg), APP_OK);
    EXPECT_EQ_INT(cfg.port, 2022);
    EXPECT_EQ_INT(cfg.poll_interval_seconds, 15);
    EXPECT_EQ_INT(cfg.recursive, 0);
    EXPECT_EQ_INT(cfg.max_depth, 4);
    EXPECT_TRUE(strcmp(cfg.password, "secret") == 0);
}

static void test_config_accepts_ssh_key_auth_config(void) {
    const char *json =
        "{"
        "\"sftp\":{"
        "\"host\":\"example.com\"," 
        "\"username\":\"alice\"," 
        "\"auth\":{\"ssh_key_path\":\"~/.ssh/id_rsa\"},"
        "\"remote_dir\":\"/incoming\""
        "},"
        "\"download\":{\"local_root\":\"./downloads\"}"
        "}";
    app_config_t cfg;

    EXPECT_EQ_INT(load_from_json(json, &cfg), APP_OK);
    EXPECT_TRUE(strcmp(cfg.ssh_key_path, "~/.ssh/id_rsa") == 0);
    EXPECT_EQ_INT(cfg.port, 22);
}

static void test_config_accepts_valid_minimal_config(void) {
    const char *json =
        "{"
        "\"sftp\":{"
        "\"host\":\"example.com\"," 
        "\"username\":\"alice\"," 
        "\"auth\":{\"password\":\"pw\"},"
        "\"remote_dir\":\"/incoming\""
        "},"
        "\"download\":{\"local_root\":\"./downloads\"}"
        "}";
    app_config_t cfg;

    EXPECT_EQ_INT(load_from_json(json, &cfg), APP_OK);
    EXPECT_EQ_INT(cfg.port, 22);
    EXPECT_EQ_INT(cfg.poll_interval_seconds, 30);
    EXPECT_EQ_INT(cfg.recursive, 1);
    EXPECT_EQ_INT(cfg.max_depth, 0);
    EXPECT_EQ_INT(cfg.include_pattern_count, 0);
    EXPECT_EQ_INT(cfg.exclude_pattern_count, 0);
}

static void test_config_rejects_invalid_port_type(void) {
    const char *json =
        "{"
        "\"sftp\":{"
        "\"host\":\"example.com\"," 
        "\"port\":\"22\"," 
        "\"username\":\"alice\"," 
        "\"auth\":{\"password\":\"pw\"},"
        "\"remote_dir\":\"/incoming\""
        "},"
        "\"download\":{\"local_root\":\"./downloads\"}"
        "}";
    app_config_t cfg;

    EXPECT_EQ_INT(load_from_json(json, &cfg), APP_ERR_CONFIG);
}

static void test_config_rejects_invalid_recursive_type(void) {
    const char *json =
        "{"
        "\"sftp\":{"
        "\"host\":\"example.com\"," 
        "\"username\":\"alice\"," 
        "\"auth\":{\"password\":\"pw\"},"
        "\"remote_dir\":\"/incoming\""
        "},"
        "\"watch\":{\"recursive\":\"true\"},"
        "\"download\":{\"local_root\":\"./downloads\"}"
        "}";
    app_config_t cfg;

    EXPECT_EQ_INT(load_from_json(json, &cfg), APP_ERR_CONFIG);
}

static void test_config_rejects_invalid_include_patterns_type(void) {
    const char *json =
        "{"
        "\"sftp\":{"
        "\"host\":\"example.com\"," 
        "\"username\":\"alice\"," 
        "\"auth\":{\"password\":\"pw\"},"
        "\"remote_dir\":\"/incoming\""
        "},"
        "\"watch\":{\"include_patterns\":[\"*.log\",1]},"
        "\"download\":{\"local_root\":\"./downloads\"}"
        "}";
    app_config_t cfg;

    EXPECT_EQ_INT(load_from_json(json, &cfg), APP_ERR_CONFIG);
}

int main(void) {
    size_t i;
    test_case_t tests[] = {
        {"test_config_rejects_missing_host", test_config_rejects_missing_host},
        {"test_config_rejects_missing_remote_dir", test_config_rejects_missing_remote_dir},
        {"test_config_rejects_invalid_poll_interval", test_config_rejects_invalid_poll_interval},
        {"test_config_rejects_invalid_max_depth", test_config_rejects_invalid_max_depth},
        {"test_config_rejects_empty_credentials", test_config_rejects_empty_credentials},
        {"test_config_accepts_password_auth_config", test_config_accepts_password_auth_config},
        {"test_config_accepts_ssh_key_auth_config", test_config_accepts_ssh_key_auth_config},
        {"test_config_accepts_valid_minimal_config", test_config_accepts_valid_minimal_config},
        {"test_config_rejects_invalid_port_type", test_config_rejects_invalid_port_type},
        {"test_config_rejects_invalid_recursive_type", test_config_rejects_invalid_recursive_type},
        {"test_config_rejects_invalid_include_patterns_type", test_config_rejects_invalid_include_patterns_type},
    };

    for (i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i) {
        printf("RUN %s\n", tests[i].name);
        tests[i].fn();
    }

    if (g_failures == 0) {
        puts("test_config: all tests passed");
        return 0;
    }

    fprintf(stderr, "test_config: %d failure(s)\n", g_failures);
    return 1;
}