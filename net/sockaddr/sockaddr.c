#include "sockaddr.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct sockaddr *sockaddr_new() {
    return calloc(1, sizeof(struct sockaddr_storage));
}

struct sockaddr *sockaddr_parse(const char *address) {
    assert(address != NULL);
    char buf[128];
    strncpy(buf, address, 128);
    int af = AF_UNSPEC;
    char *addr_start = NULL, *addr_end = NULL, *port_start = NULL;
    if (buf[0] == '[') {
        af = AF_INET6;
        addr_start = buf + 1;
        addr_end = strchr(addr_start, ']');
        if (addr_end != NULL) {
            *addr_end = '\0';
            port_start = strchr(addr_end + 1, ':');
            if (port_start != NULL) {
                *port_start = '\0';
                port_start++;
            }
        }
    } else {
        af = AF_INET;
        addr_start = buf;
        addr_end = strchr(addr_start, ':');
        if (addr_end != NULL) {
            *addr_end = '\0';
            port_start = addr_end + 1;
        }
    }
    struct sockaddr_storage *addr = calloc(1, sizeof(struct sockaddr_storage));
    addr->ss_family = af;
    if (inet_pton(af, addr_start, &((struct sockaddr_in *) addr)->sin_addr) != 1) {
        free(addr);
        return NULL;
    }
    if (port_start != NULL) {
        char *endptr = NULL;
        uint16_t port = (uint16_t) strtol(port_start, &endptr, 10);
        if (endptr == NULL || *endptr != '\0') {
            free(addr);
            return NULL;
        }
        sockaddr_set_port((struct sockaddr *) addr, port);
    }
    return (struct sockaddr *) addr;
}

int sockaddr_set_address(struct sockaddr *addr, int family, const void *address) {
    switch (family) {
        case AF_INET: {
            struct sockaddr_in *addr4 = (struct sockaddr_in *) addr;
            addr4->sin_family = AF_INET;
            addr4->sin_addr = *(struct in_addr *) address;
            return 0;
        }
        case AF_INET6: {
            struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *) addr;
            addr6->sin6_family = AF_INET6;
            addr6->sin6_addr = *(struct in6_addr *) address;
            return 0;
        }
        default:
            return -1;
    }
}

int sockaddr_address_to_string(struct sockaddr *addr, char *dest, size_t len) {
    switch (addr->sa_family) {
        case AF_INET: {
            struct sockaddr_in *addr4 = (struct sockaddr_in *) addr;
            return inet_ntop(AF_INET, &addr4->sin_addr, dest, len) == NULL ? -1 : 0;
        }
        case AF_INET6: {
            struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *) addr;
            return inet_ntop(AF_INET6, &addr6->sin6_addr, dest, len) == NULL ? -1 : 0;
        }
        default:
            return -1;
    }
}

int sockaddr_set_port(struct sockaddr *addr, uint16_t port) {
    struct sockaddr_in *addr4 = (struct sockaddr_in *) addr;
    addr4->sin_port = htons(port);
    return 0;
}

uint16_t sockaddr_get_port(struct sockaddr *addr) {
    struct sockaddr_in *addr4 = (struct sockaddr_in *) addr;
    return ntohs(addr4->sin_port);
}