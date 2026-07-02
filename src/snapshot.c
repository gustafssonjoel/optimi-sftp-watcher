#include "snapshot.h"

void snapshot_init(snapshot_t *snapshot) {
    if (snapshot == 0) {
        return;
    }
    snapshot->entries = 0;
    snapshot->count = 0;
}

void snapshot_free(snapshot_t *snapshot) {
    (void)snapshot;
}

app_error_t snapshot_diff_compute(
    const snapshot_t *previous,
    const snapshot_t *current,
    snapshot_diff_t *out_diff
) {
    (void)previous;
    (void)current;
    (void)out_diff;
    return APP_ERR_NOT_IMPLEMENTED;
}

void snapshot_diff_free(snapshot_diff_t *diff) {
    (void)diff;
}
