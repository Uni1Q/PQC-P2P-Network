//
// Created by rokas on 24/09/2024.
//

// common/network.c
#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int create_socket() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
    }
    return sock;
}

int connect_to_server(const char *ip, int port) {
    int sock = create_socket();
    if (sock < 0) return -1;

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Convert IP address from string to binary form
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock);
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        return -1;
    }

    return sock;
}

int bind_and_listen(int port) {
    int server_sock = create_socket();
    if (server_sock < 0) return -1;

    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR,
                   &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server_sock);
        return -1;
                   }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY; // Accept connections on any interface

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_sock);
        return -1;
    }

    if (listen(server_sock, 10) < 0) {
        perror("listen");
        close(server_sock);
        return -1;
    }

    return server_sock;
}
