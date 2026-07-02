#ifndef SFTP_CLIENT_H
#define SFTP_CLIENT_H

#include "app_error.h"
#include "config.h"
#include "snapshot.h"

typedef struct sftp_client sftp_client_t;

app_error_t sftp_client_create(const app_config_t *config, sftp_client_t **out_client);
void sftp_client_destroy(sftp_client_t *client);

app_error_t sftp_client_list_files(
    sftp_client_t *client,
    const char *remote_dir,
    int recursive,
    int max_depth,
    snapshot_t *out_snapshot
);

app_error_t sftp_client_download_file(
    sftp_client_t *client,
    const char *remote_path,
    const char *local_path
);

#endif
