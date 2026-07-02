#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include <stddef.h>
#include <time.h>

#include "app_error.h"

typedef struct remote_entry {
    char *path;
    long long size;
    time_t mtime;
} remote_entry_t;

typedef struct snapshot {
    remote_entry_t *entries;
    size_t count;
} snapshot_t;

typedef enum change_type {
    CHANGE_CREATED,
    CHANGE_MODIFIED,
    CHANGE_DELETED
} change_type_t;

typedef struct snapshot_change {
    change_type_t type;
    const char *path;
} snapshot_change_t;

typedef struct snapshot_diff {
    snapshot_change_t *changes;
    size_t count;
} snapshot_diff_t;

void snapshot_init(snapshot_t *snapshot);
void snapshot_free(snapshot_t *snapshot);

app_error_t snapshot_diff_compute(
    const snapshot_t *previous,
    const snapshot_t *current,
    snapshot_diff_t *out_diff
);

void snapshot_diff_free(snapshot_diff_t *diff);

#endif
