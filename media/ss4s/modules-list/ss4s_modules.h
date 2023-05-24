#pragma once

#include <stdbool.h>

#include "os_info.h"
#include "version_info.h"
#include "ss4s/module.h"

#define MODULE_PREFERENCE_AUTO "auto"

typedef struct array_list_t array_list_t;

typedef struct str_list_t {
    int count;
    char **elements;
    char *raw;
} str_list_t;

typedef struct module_info_t {
    char *id;
    char *group;
    char *name;
    bool has_audio;
    bool has_video;
    int weight;
    str_list_t conflicts;

    version_constraints_t os_version;
} module_info_t;

typedef struct module_preferences_t {
    const char *audio_module;
    const char *video_module;
} module_preferences_t;

typedef struct module_selection_t {
    const module_info_t *audio_module;
    const module_info_t *video_module;
} module_selection_t;

/**
 * Load modules info into the list pointer
 * @param list Array list pointer to load into
 * @param os_info Runtime system information
 * @return 0 if succeeded
 */
int modules_load(array_list_t *list, const os_info_t *os_info);

/**
 * This DOES NOT free the list pointer itself.
 * @param list Modules list to clear all the content
 */
void modules_clear(array_list_t *list);

bool module_conflicts(const module_info_t *a, const module_info_t *b);

bool module_select(const array_list_t *list, const module_preferences_t *preferences, module_selection_t *selection,
                   bool check_module);

const char *module_info_get_id(const module_info_t *info);

const char *module_info_get_name(const module_info_t *info);

const char *module_info_get_group(const module_info_t *info);