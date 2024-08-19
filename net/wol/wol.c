#include "wol.h"
#include "logging.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#if __WIN32__

#include <winsock2.h>
#define INVSOCKET ((SOCKET) NULL)
typedef char SOCKOPT;

#else

#include <arpa/inet.h>
#include <unistd.h>

typedef int SOCKET;
typedef int SOCKOPT;
#define closesocket(s) close(s)
#define INVSOCKET (-1)

#endif

static bool wol_build_packet(const char *macstr, char *packet);

int wol_broadcast(const char *mac) {
    char packet[102];
    if (!wol_build_packet(mac, packet)) {
        return -1;
    }
    int ret = 0;
    SOCKOPT broadcast = 1;
    SOCKET sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast)) {
        commons_log_error("WoL", "setsockopt() error: %d %s", errno, strerror(errno));
        ret = -1;
        goto cleanup;
    }

    struct sockaddr_in client, server;
    client.sin_family = AF_INET;
    client.sin_addr.s_addr = INADDR_ANY;
    client.sin_port = 0;
    // Bind socket
    if (bind(sockfd, (struct sockaddr *) &client, sizeof(client)) != 0) {
        ret = errno;
        commons_log_error("WoL", "bind() error: %d %s", errno, strerror(errno));
        goto cleanup;
    }

    // Set server endpoint (broadcast)
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("255.255.255.255");
    server.sin_port = htons(9);

    if (sendto(sockfd, packet, sizeof(packet), 0, (struct sockaddr *) &server, sizeof(server)) == -1) {
        ret = errno;
        commons_log_error("WoL", "sendto() error: %d %s", errno, strerror(errno));
        goto cleanup;
    }
    cleanup:
    if (sockfd != INVSOCKET) {
        closesocket(sockfd);
    }
    return ret;
}

static bool wol_build_packet(const char *macstr, char *packet) {
    unsigned int values[6];
    if (sscanf(macstr, "%x:%x:%x:%x:%x:%x%*c", &values[0], &values[1], &values[2], &values[3], &values[4],
               &values[5]) != 6) {
        return false;
    }
    uint8_t mac[6];
    for (int i = 0; i < 6; i++) {
        mac[i] = (uint8_t) values[i];
    }
    memset(packet, 0xFF, 6);
    for (int i = 0; i < 16; i++) {
        memcpy(&packet[6 + i * 6], mac, 6);
    }
    return true;
}