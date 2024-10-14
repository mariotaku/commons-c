#pragma once

#include "sockaddr.h"

typedef struct hostport host_t;

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

void host_free(host_t *host) __attribute__((nonnull (1)));

const char *host_get_hostname(const host_t *host) __attribute__((nonnull (1)));

uint16_t host_get_port(const host_t *host) __attribute__((nonnull (1)));

int host_to_string(const host_t *host, char *buf, size_t len) __attribute__((nonnull (1, 2)));

int host_is_ip(const host_t *host) __attribute__((nonnull (1)));