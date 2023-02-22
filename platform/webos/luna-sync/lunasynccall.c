#include "lunasynccall.h"

#include <libhelpers.h>

struct HContextSync {
    union {
        HContext ctx;
        __attribute__((unused)) unsigned char placeholder[128];
    } base;
    GMutex mutex;
    GCond cond;
    bool finished;
    char **output;
};

static bool callback(LSHandle *sh, LSMessage *reply, void *ctx);

bool HLunaServiceCallSync(const char *uri, const char *payload, bool public, char **output) {
    struct HContextSync context = {.base.ctx = {
            .multiple = 0,
            .public = public ? 1 : 0,
            .callback = callback,
    }};

    g_mutex_init(&context.mutex);
    g_cond_init(&context.cond);
    context.output = output;

    if (HLunaServiceCall(uri, payload, &context.base.ctx) != 0) {
        g_mutex_clear(&context.mutex);
        g_cond_clear(&context.cond);
        return false;
    }
    g_mutex_lock(&context.mutex);
    while (!context.finished) {
        g_cond_wait(&context.cond, &context.mutex);
    }
    g_mutex_unlock(&context.mutex);

    g_mutex_clear(&context.mutex);
    g_cond_clear(&context.cond);
    return true;
}

static bool callback(LSHandle *sh, LSMessage *reply, void *ctx) {
    (void) sh;
    struct HContextSync *context = (struct HContextSync *) ctx;
    context->finished = true;
    if (context->output) {
        *context->output = g_strdup(HLunaServiceMessage(reply));
    }
    g_cond_signal(&context->cond);
    return true;
}