#include <assert.h>
#include <string.h>

#include "host.h"

void test_host_parse() {
    host_t *host;

    host = host_parse("www.google.com:80");
    assert(host != NULL);
    assert(strcmp(host_get_hostname(host), "www.google.com") == 0);
    assert(host_get_port(host) == 80);
    host_free(host);

    assert(host_parse("www.google.com:") == NULL);
}

void test_host_new() {
    host_t *host = host_new("www.google.com", 80);
    assert(host != NULL);
    assert(strcmp(host_get_hostname(host), "www.google.com") == 0);
    host_free(host);
}

int main() {
    test_host_parse();
    test_host_new();
}