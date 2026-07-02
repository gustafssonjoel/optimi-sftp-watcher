#include "config.h"

app_error_t config_load(const char *path, app_config_t *out_config) {
    (void)path;
    (void)out_config;
    return APP_ERR_NOT_IMPLEMENTED;
}

void config_free(app_config_t *config) {
    (void)config;
}
