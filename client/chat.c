//
// Created by rokas on 24/09/2024.
//

// chat.c
#include "chat.h"
#include "network.h"
#include "utils.h"
#include "peer_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

extern int in_chat;
extern int routing_server_sock;
extern char username_global[50];

void *handle_incoming_connections(void *arg) {
    int listen_port = *(int *)arg;
    int server_sock = bind_and_listen(listen_port);
    if (server_sock < 0) {
        pthread_exit(NULL);
    }

    safe_print("Listening for incoming connections on port %d...\n", listen_port);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("accept");
            continue;
        }

        int *new_sock = malloc(sizeof(int));
        if (new_sock == NULL) {
            perror("malloc");
            close(client_sock);
            continue;
        }
        *new_sock = client_sock;
        pthread_t peer_thread;
        if (pthread_create(&peer_thread, NULL, handle_peer_connection, (void *)new_sock) != 0) {
            perror("pthread_create");
            close(client_sock);
            free(new_sock);
            continue;
        }
        pthread_detach(peer_thread);
    }
    close(server_sock);
    pthread_exit(NULL);
}

void *handle_peer_connection(void *arg) {
    int peer_sock = *(int *)arg;
    free(arg);
    char buffer[BUFFER_SIZE];
    char peer_username[50];
    int connection_accepted = 0;

    // Read the connection request message
    ssize_t bytes_read = read(peer_sock, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        safe_print("Connection closed by peer before handshake.\n");
        close(peer_sock);
        pthread_exit(NULL);
    }
    buffer[bytes_read] = '\0';

    if (strncmp(buffer, "CONNECT_REQUEST", 15) == 0) {
        // Extract peer's username
        sscanf(buffer + 16, "%s", peer_username);

        safe_print("\nPeer '%s' wants to connect with you. Accept? (yes/no): ", peer_username);
        fflush(stdout);

        char response[10];
        fgets(response, sizeof(response), stdin);
        trim_newline(response);

        if (strcmp(response, "yes") == 0) {
            // Send ACCEPT response
            strcpy(buffer, "ACCEPT");
            write(peer_sock, buffer, strlen(buffer));

            // Remove from discoverable list
            char remove_msg[BUFFER_SIZE];
            snprintf(remove_msg, sizeof(remove_msg), "REMOVE %s", username_global);
            write(routing_server_sock, remove_msg, strlen(remove_msg));

            connection_accepted = 1;
            in_chat = 1; // Set in_chat flag
        } else {
            // Send DENY response
            strcpy(buffer, "DENY");
            write(peer_sock, buffer, strlen(buffer));
            safe_print("You denied the connection request from '%s'.\n", peer_username);
            close(peer_sock);
            pthread_exit(NULL);
        }
    } else {
        safe_print("Invalid connection request.\n");
        close(peer_sock);
        pthread_exit(NULL);
    }

    if (connection_accepted) {
        safe_print("Connection with '%s' established.\n", peer_username);

        // Start message exchange
        pthread_t send_thread;
        struct chat_info *chat = malloc(sizeof(struct chat_info));

        chat->sock = peer_sock;
        strcpy(chat->peer_username, peer_username);
        strcpy(chat->your_username, username_global);

        if (pthread_create(&send_thread, NULL, send_messages, (void *)chat) != 0) {
            perror("pthread_create");
            close(peer_sock);
            free(chat);
            pthread_exit(NULL);
        }

        // Receive messages
        char message[BUFFER_SIZE];
        ssize_t bytes_read;
        while ((bytes_read = read(peer_sock, message, sizeof(message) - 1)) > 0) {
            message[bytes_read] = '\0';
            safe_print("[%s]: %s", peer_username, message);
        }

        safe_print("Connection with '%s' closed.\n", peer_username);
        close(peer_sock);
        in_chat = 0; // Reset in_chat flag
        pthread_exit(NULL);
    }

    close(peer_sock);
    pthread_exit(NULL);
}

void *send_messages(void *arg) {
    struct chat_info *chat = (struct chat_info *)arg;
    char message[BUFFER_SIZE];

    while (1) {
        safe_print("[%s]: ", chat->your_username);
        fflush(stdout);
        if (fgets(message, sizeof(message), stdin) == NULL) {
            break;
        }
        if (write(chat->sock, message, strlen(message)) <= 0) {
            safe_print("Failed to send message. Connection may have been lost.\n");
            break;
        }
    }
    free(chat);
    pthread_exit(NULL);
}
