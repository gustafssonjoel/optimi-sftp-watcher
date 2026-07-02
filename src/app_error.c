#include "app_error.h"

const char *app_error_to_string(app_error_t err) {
    switch (err) {
        case APP_OK:
            return "ok";
        case APP_ERR_INVALID_ARG:
            return "invalid argument";
        case APP_ERR_CONFIG:
            return "configuration error";
        case APP_ERR_IO:
            return "io error";
        case APP_ERR_NETWORK:
            return "network error";
        case APP_ERR_NOT_IMPLEMENTED:
            return "not implemented";
        case APP_ERR_UNKNOWN:
        default:
            return "unknown error";
    }
}
