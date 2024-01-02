/*
 * Copyright (c) 2024 Ningyuan Li <https://github.com/mariotaku>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <assert.h>
#include <stdlib.h>

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

    addr = sockaddr_parse("1.1.1.1:aaa");
    assert(addr == NULL);

    addr = sockaddr_parse("");
    assert(addr == NULL);
}

int main() {
    test_parse();
    return 0;
}