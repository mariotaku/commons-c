#pragma once

typedef struct cec_support_t cec_support_ctx_t;

cec_support_ctx_t *cec_support_create();

void cec_support_destroy(cec_support_ctx_t *ctx);