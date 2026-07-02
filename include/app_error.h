#ifndef APP_ERROR_H
#define APP_ERROR_H

typedef enum app_error {
    APP_OK = 0,
    APP_ERR_INVALID_ARG,
    APP_ERR_CONFIG,
    APP_ERR_IO,
    APP_ERR_NETWORK,
    APP_ERR_NOT_IMPLEMENTED,
    APP_ERR_UNKNOWN
} app_error_t;

const char *app_error_to_string(app_error_t err);

#endif
