#include "path_utils.h"

app_error_t path_join(char *out, size_t out_size, const char *left, const char *right) {
    (void)out;
    (void)out_size;
    (void)left;
    (void)right;
    return APP_ERR_NOT_IMPLEMENTED;
}

app_error_t path_make_local_from_remote(
    char *out,
    size_t out_size,
    const char *local_root,
    const char *remote_root,
    const char *remote_file
) {
    (void)out;
    (void)out_size;
    (void)local_root;
    (void)remote_root;
    (void)remote_file;
    return APP_ERR_NOT_IMPLEMENTED;
}
