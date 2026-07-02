#ifndef CONFIG_H
#define CONFIG_H

#include "app_error.h"

#define CONFIG_MAX_STR 512
#define CONFIG_MAX_PATTERNS 32

typedef struct app_config {
    char host[CONFIG_MAX_STR];
    int port;
    char username[CONFIG_MAX_STR];
    char password[CONFIG_MAX_STR];
    char ssh_key_path[CONFIG_MAX_STR];
    char remote_dir[CONFIG_MAX_STR];
    char local_download_root[CONFIG_MAX_STR];
    int poll_interval_seconds;
    int recursive;
    int max_depth;

    int include_pattern_count;
    char include_patterns[CONFIG_MAX_PATTERNS][CONFIG_MAX_STR];

    int exclude_pattern_count;
    char exclude_patterns[CONFIG_MAX_PATTERNS][CONFIG_MAX_STR];

    char log_level[64];
    char log_destination[CONFIG_MAX_STR];
} app_config_t;

app_error_t config_load(const char *path, app_config_t *out_config);
void config_free(app_config_t *config);

#endif
