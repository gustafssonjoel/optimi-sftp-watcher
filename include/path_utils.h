#ifndef PATH_UTILS_H
#define PATH_UTILS_H

#include <stddef.h>

#include "app_error.h"

app_error_t path_join(char *out, size_t out_size, const char *left, const char *right);
app_error_t path_make_local_from_remote(
    char *out,
    size_t out_size,
    const char *local_root,
    const char *remote_root,
    const char *remote_file
);

#endif
