#pragma once

#include <stddef.h>
#include <stdbool.h>

#include <SDL_thread.h>

typedef struct commons_gcdb_updater_t {
    char *path;
    char *platform;
    char *platform_use;

    SDL_mutex *lock;

    char *platform_match_substr;
    size_t platform_match_substr_len;

    SDL_Thread *update_thread;
    bool update_running;
} commons_gcdb_updater_t;

void commons_gcdb_updater_init(commons_gcdb_updater_t *updater);

void commons_gcdb_updater_deinit(commons_gcdb_updater_t *updater);

void commons_gcdb_updater_update(commons_gcdb_updater_t *updater);
