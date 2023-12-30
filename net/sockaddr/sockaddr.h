#pragma once

#if __WIN32__
#include <winsock2.h>
#else

#include <arpa/inet.h>

#endif

struct sockaddr *sockaddr_new();

int sockaddr_set_address(struct sockaddr *addr, int family, const void *address);

int sockaddr_address_to_string(struct sockaddr *addr, char *dest, size_t len);

int sockaddr_set_port(struct sockaddr *addr, uint16_t port);

uint16_t sockaddr_get_port(struct sockaddr *addr);