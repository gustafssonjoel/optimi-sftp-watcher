#include "sftp_client.h"

struct sftp_client {
    int reserved;
};

app_error_t sftp_client_create(const app_config_t *config, sftp_client_t **out_client) {
    (void)config;
    (void)out_client;
    return APP_ERR_NOT_IMPLEMENTED;
}

void sftp_client_destroy(sftp_client_t *client) {
    (void)client;
}

app_error_t sftp_client_list_files(
    sftp_client_t *client,
    const char *remote_dir,
    int recursive,
    int max_depth,
    snapshot_t *out_snapshot
) {
    (void)client;
    (void)remote_dir;
    (void)recursive;
    (void)max_depth;
    (void)out_snapshot;
    return APP_ERR_NOT_IMPLEMENTED;
}

app_error_t sftp_client_download_file(
    sftp_client_t *client,
    const char *remote_path,
    const char *local_path
) {
    (void)client;
    (void)remote_path;
    (void)local_path;
    return APP_ERR_NOT_IMPLEMENTED;
}
