#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "sockaddr.h"

void test_parse() {
    struct sockaddr *addr;

    addr = sockaddr_parse("192.168.1.1:80");
    assert(addr != NULL);
    assert(addr->sa_family == AF_INET);
    assert(sockaddr_get_port(addr) == 80);
    free(addr);

    addr = sockaddr_parse("127.0.0.1");
    assert(addr != NULL);
    assert(addr->sa_family == AF_INET);
    assert(sockaddr_get_port(addr) == 0);
    free(addr);

    addr = sockaddr_parse("[::1]:80");
    assert(addr != NULL);
    assert(addr->sa_family == AF_INET6);
    assert(sockaddr_get_port(addr) == 80);
    free(addr);

    addr = sockaddr_parse("[::1]");
    assert(addr != NULL);
    assert(addr->sa_family == AF_INET6);
    assert(sockaddr_get_port(addr) == 0);
    free(addr);

    addr = sockaddr_parse("[]");
    assert(addr == NULL);

    addr = sockaddr_parse("[111");
    assert(addr == NULL);

    addr = sockaddr_parse("[111]:aaa");
    assert(addr == NULL);

    addr = sockaddr_parse("[111");
    assert(addr == NULL);

    addr = sockaddr_parse("[111:aaa");
    assert(addr == NULL);

    addr = sockaddr_parse("[111:aaa::");
    assert(addr == NULL);

    addr = sockaddr_parse("1.1.1.1:aaa");
    assert(addr == NULL);

    addr = sockaddr_parse("");
    assert(addr == NULL);
}

void test_to_string() {
    struct sockaddr *addr;
    char buf[64];

    addr = sockaddr_parse("127.0.0.1:80");
    sockaddr_to_string(addr, buf, 64);
    assert(strcmp(buf, "127.0.0.1:80") == 0);
    free(addr);

    addr = sockaddr_parse("127.0.0.1");
    sockaddr_to_string(addr, buf, 64);
    assert(strcmp(buf, "127.0.0.1") == 0);
    free(addr);

    addr = sockaddr_parse("[::1]:80");
    sockaddr_to_string(addr, buf, 64);
    assert(strcmp(buf, "[::1]:80") == 0);
    free(addr);
}

int main() {
    test_parse();
    test_to_string();
    return 0;
}