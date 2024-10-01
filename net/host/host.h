#pragma once

#include "sockaddr.h"

typedef struct host host_t;

/**
 * Create a new host.
 * @param hostname Hostname.
 * @param port Port.
 * @return New host. NULL if the hostname isn't valid.
 */
host_t *host_new(const char *hostname, uint16_t port) __attribute__((nonnull (1)));

/**
 * Parse a host string.
 * @param s Host string. Format: hostname[:port]
 * @return Parsed host or NULL if the string is invalid.
 */
host_t *host_parse(const char *s) __attribute__((nonnull (1)));

void host_free(host_t *host);

const char *host_get_hostname(const host_t *host);

uint16_t host_get_port(const host_t *host);