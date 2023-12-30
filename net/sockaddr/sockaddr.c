#include "sockaddr.h"

#include <stdlib.h>

struct sockaddr *sockaddr_new() {
    return calloc(1, sizeof(struct sockaddr_storage));
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