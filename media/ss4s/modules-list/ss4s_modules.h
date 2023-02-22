#pragma once

#include <stdbool.h>

#include "os_info.h"
#include "version_info.h"
#include "ss4s/module.h"

typedef struct array_list_t array_list_t;

typedef struct str_list_t {
    int count;
    char **elements;
    char *raw;
} str_list_t;

typedef struct module_info_t {
    char *section;
    char *name;
    bool has_audio;
    bool has_video;
    int weight;
    str_list_t modules;
    str_list_t conflicts;

    version_constraint_t os_version;
} module_info_t;

typedef struct module_selection_t {
    const char *audio;
    const char *video;
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

const char *module_first_available(const module_info_t *info, SS4S_ModuleCheckFlag flags);

bool module_select(const array_list_t *list, module_selection_t * selection);
