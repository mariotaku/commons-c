#include "ss4s_modules.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "ini.h"
#include "array_list.h"

#ifndef SS4S_MODULES_INI_PATH
#define SS4S_MODULES_INI_PATH "lib/ss4s_modules.ini"
#endif

typedef struct modules_parse_context {
    module_info_t current;
    const os_info_t *os_info;
    array_list_t *modules;
} modules_parse_context;

static void module_info_clear(module_info_t *info);

static void section_changed(modules_parse_context *context);

static void modules_parse_context_destroy(modules_parse_context *context);

static int modules_ini_handler(void *user, const char *section, const char *name, const char *value);

static int module_weight_compare_fn(const void *a, const void *b);

static const module_info_t *module_by_id(const array_list_t *list, const char *id);

/* Delimited string list */

static int str_list_parse(str_list_t *list, const char *value, const char *delim);

static void str_list_clear(str_list_t *list);

static bool preference_auto(const char *value);

int modules_load(array_list_t *list, const os_info_t *os_info) {
    array_list_init(list, sizeof(module_info_t), 16);
    FILE *f = fopen(SS4S_MODULES_INI_PATH, "r");
    if (f == NULL) return errno;
    modules_parse_context mpc = {.modules = list, .os_info = os_info};
    module_info_clear(&mpc.current);
    ini_parse_file(f, modules_ini_handler, &mpc);
    section_changed(&mpc);
    modules_parse_context_destroy(&mpc);
    array_list_qsort(list, module_weight_compare_fn);
    return 0;
}

void modules_clear(array_list_t *list) {
    for (int i = array_list_size(list) - 1; i >= 0; --i) {
        module_info_clear(array_list_get(list, i));
    }
    array_list_deinit(list);
}

bool module_conflicts(const module_info_t *a, const module_info_t *b) {
    for (int i = 0; i < a->conflicts.count; i++) {
        const char *item = a->conflicts.elements[i];
        if (strcmp(item, b->id) == 0 || (b->group != NULL && strcmp(item, b->group) == 0)) {
            return true;
        }
    }
    for (int i = 0; i < b->conflicts.count; i++) {
        const char *item = b->conflicts.elements[i];
        if (strcmp(item, a->id) == 0 || (a->group != NULL && strcmp(item, a->group) == 0)) {
            return true;
        }
    }
    return false;
}

bool module_select(const array_list_t *list, const module_preferences_t *preferences, module_selection_t *selection,
                   bool check_module) {
    const module_info_t *selected_video_module = NULL, *selected_audio_module = NULL;
    if (preferences != NULL && !preference_auto(preferences->video_module)) {
        const module_info_t *selected = module_by_id(list, preferences->video_module);
        if (selected != NULL && selected->has_video) {
            SS4S_ModuleCheckFlag flags = SS4S_MODULE_CHECK_VIDEO;
            bool set_audio = selected->has_audio && preference_auto(preferences->audio_module);
            if (set_audio) {
                flags = SS4S_MODULE_CHECK_AUDIO;
            }
            if (!check_module || SS4S_ModuleAvailable(selected->id, flags)) {
                selected_video_module = selected;
                if (set_audio) {
                    selected_audio_module = selected;
                }
            }
        }
    }
    if (selected_video_module == NULL) {
        for (int i = 0, j = array_list_size(list); i < j; ++i) {
            const module_info_t *info = array_list_get((array_list_t *) list, i);
            if (!info->has_video) {
                continue;
            }
            SS4S_ModuleCheckFlag flags = SS4S_MODULE_CHECK_VIDEO;
            bool set_audio = info->has_audio && (preferences == NULL || preference_auto(preferences->audio_module));
            if (set_audio) {
                flags = SS4S_MODULE_CHECK_AUDIO;
            }
            if (!check_module || SS4S_ModuleAvailable(info->id, flags)) {
                selected_video_module = info;
                if (set_audio) {
                    selected_audio_module = info;
                }
                break;
            }
        }
    }
    if (selected_video_module == NULL) {
        return false;
    }
    if (preferences != NULL && !preference_auto(preferences->audio_module)) {
        const module_info_t *selected = module_by_id(list, preferences->audio_module);
        if (selected != NULL && selected->has_audio &&
            (!check_module || !module_conflicts(selected_video_module, selected) &&
                              SS4S_ModuleAvailable(selected->id, SS4S_MODULE_CHECK_AUDIO))) {
            selected_audio_module = selected;
        }
    }
    if (selected_audio_module == NULL) {
        for (int i = 0, j = array_list_size(list); i < j; ++i) {
            const module_info_t *info = array_list_get((array_list_t *) list, i);
            if (!info->has_audio || check_module && module_conflicts(selected_video_module, info)) {
                continue;
            }
            if (!check_module || SS4S_ModuleAvailable(info->id, SS4S_MODULE_CHECK_AUDIO)) {
                selected_audio_module = info;
            }
        }
    }
    if (selected_audio_module == NULL) {
        return false;
    }
    selection->audio_module = selected_audio_module;
    selection->video_module = selected_video_module;
    return true;
}

const char *module_info_get_id(const module_info_t *info) {
    if (info == NULL) {
        return NULL;
    }
    return info->id;
}

const char *module_info_get_name(const module_info_t *info) {
    if (info == NULL) {
        return NULL;
    }
    return info->name;
}

const char *module_info_get_group(const module_info_t *info) {
    if (info == NULL) {
        return NULL;
    }
    if (info->group == NULL) {
        return info->id;
    }
    return info->group;
}

static int modules_ini_handler(void *user, const char *section, const char *name, const char *value) {
    modules_parse_context *mpc = user;
    // Only check for same pointer, ignoring content
    if (((section != NULL) != (mpc->current.id != NULL)) ||
        (section != NULL && strcmp(section, mpc->current.id) != 0)) {
        section_changed(mpc);
        mpc->current.id = strdup(section);
    }
    if (strcmp("name", name) == 0) {
        mpc->current.name = strdup(value);
    }
    if (strcmp("group", name) == 0) {
        mpc->current.group = strdup(value);
    } else if (strcmp("weight", name) == 0) {
        mpc->current.weight = (int) strtol(value, NULL, 10);
        if (mpc->current.weight < 0) {
            mpc->current.weight = 0;
        } else if (mpc->current.weight > 100) {
            mpc->current.weight = 100;
        }
    } else if (strcmp("audio", name) == 0) {
        mpc->current.has_audio = strcmp("true", value) == 0;
    } else if (strcmp("video", name) == 0) {
        mpc->current.has_video = strcmp("true", value) == 0;
    } else if (strcmp("os_version", name) == 0) {
        version_constraints_parse(&mpc->current.os_version, value);
    } else if (strcmp("conflicts", name) == 0) {
        str_list_parse(&mpc->current.conflicts, value, ",");
    }
    return 1;
}

static void section_changed(modules_parse_context *context) {
    if (context->current.id == NULL) {
        return;
    }
    if (!version_constraints_check(&context->current.os_version, &context->os_info->version)) {
        module_info_clear(&context->current);
        return;
    }
    module_info_t *info = array_list_add(context->modules, -1);
    *info = context->current;
    memset(&context->current, 0, sizeof(context->current));
}

static void modules_parse_context_destroy(modules_parse_context *context) {
    (void) context;
}

static void module_info_clear(module_info_t *info) {
    if (info->id == NULL) {
        return;
    }
    free(info->id);
    if (info->group != NULL) {
        free(info->group);
    }
    if (info->name != NULL) {
        free(info->name);
    }
    str_list_clear(&info->conflicts);
    version_constraints_clear(&info->os_version);
    memset(info, 0, sizeof(module_info_t));
}

static int module_weight_compare_fn(const void *a, const void *b) {
    const module_info_t *m1 = a, *m2 = b;
    int weight_diff = m2->weight - m1->weight;
    if (weight_diff == 0) {
        return strcmp(module_info_get_group(m1), module_info_get_group(m2));
    }
    return weight_diff;
}

static const module_info_t *module_by_id(const array_list_t *list, const char *id) {
    for (int i = 0, j = array_list_size(list); i < j; ++i) {
        const module_info_t *info = array_list_get((array_list_t *) list, i);
        if (strcmp(id, info->id) == 0 || (info->group != NULL && strcmp(id, info->group) == 0)) {
            return info;
        }
    }
    return NULL;
}

static int str_list_parse(str_list_t *list, const char *value, const char *delim) {
    if (value[0] == 0) {
        return 0;
    }
    int n_delim = 0;
    const char *temp = value;
    do {
        const char *find = strpbrk(temp, delim);
        if (find != NULL) {
            find++;
            n_delim++;
        }
        temp = find;
    } while (temp != NULL);
    /* This array should contain at most n_delim + 1 strings */
    char **items = calloc(n_delim + 1, sizeof(char *));
    char *value_dup = strdup(value);
    char *rest = value_dup, *cur;
    int n_items = 0;
    while ((cur = strtok_r(rest, delim, &rest))) {
        items[n_items++] = cur;
    }
    if (n_items == 0) {
        free(value_dup);
        return 0;
    }
    list->count = n_items;
    list->elements = items;
    list->raw = value_dup;
    return n_items;
}


static void str_list_clear(str_list_t *list) {
    if (list->elements != NULL) {
        free(list->elements);
    }
    if (list->raw != NULL) {
        free(list->raw);
    }
    memset(list, 0, sizeof(str_list_t));
}

static bool preference_auto(const char *value) {
    return value == NULL || strcmp(value, MODULE_PREFERENCE_AUTO) == 0;
}