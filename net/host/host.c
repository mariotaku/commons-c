#include "host.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

struct host {
    char hostname[254];
    uint16_t port;
    sockaddr_t *addr;
};

static const char *find_hostname_end(const char *s);

host_t *host_new(const char *hostname, uint16_t port) {
    host_t *host = host_parse(hostname);
    if (host == NULL) {
        return NULL;
    }
    host->port = port;
    return host;
}

host_t *host_parse(const char *s) {
    sockaddr_t *parse = sockaddr_parse(s);
    if (parse != NULL) {
        host_t *host = calloc(1, sizeof(host_t));
        strncpy(host->hostname, s, 254);
        host->port = sockaddr_get_port(parse);
        sockaddr_get_ip_str(parse, host->hostname, 253);
        host->addr = parse;
        return host;
    }
    const char *end = find_hostname_end(s);
    if (end == NULL) {
        return NULL;
    }
    uint16_t port = 0;
    if (*end == ':') {
        char *leftover = NULL;
        port = strtoul(end + 1, &leftover, 10);
        if (leftover == end + 1) {
            // Port number is invalid
            return NULL;
        }
    } else if (*end != '\0') {
        // Invalid character
        return NULL;
    }
    host_t *host = calloc(1, sizeof(host_t));
    strncpy(host->hostname, s, end - s);
    host->port = port;
    return host;
}

void host_free(host_t *host) {
    if (host->addr != NULL) {
        sockaddr_free(host->addr);
    }
    free(host);
}

const char *host_get_hostname(const host_t *host) {
    return host->hostname;
}

uint16_t host_get_port(const host_t *host) {
    return host->port;
}

const char *find_hostname_end(const char *s) {
    const char *p = s;
    // One segment can have at most 63 characters, and at most 253 characters in total.
    int seglen = 0;
    for (char c = *p; c != '\0' && p - s < 253; c = *(++p)) {
        if (c == '.') {
            if (seglen == 0) {
                return NULL;
            }
            seglen = 0;
        } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-') {
            seglen++;
            if (seglen > 63) {
                // A segment can't have more than 63 characters.
                return NULL;
            }
        } else {
            // Invalid character.
            break;
        }
    }
    return p;
}
