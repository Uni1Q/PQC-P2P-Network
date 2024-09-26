//
// Created by rokas on 24/09/2024.
//

// client.c

#include "client.h"
#include "network.h"
#include "chat.h"
#include "peer_list.h"
#include "utils.h"
#include "constants.h"
#include "pq_encryption.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <openssl/rand.h>

// Define global variables
int server_sock;
char username_global[USERNAME_MAX_LENGTH];
int peer_port;
volatile int in_chat = 0;

// Implement request_peer_list
void request_peer_list(int sock) {
    const char *request = "GET_PEER_LIST\n";
    if (write(sock, request, strlen(request)) <= 0) {
        perror("Failed to request peer list");
    }
}

void *server_listener(void *arg) {
    int sock = *(int *)arg;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    while ((bytes_read = read(sock, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';

        // Check if the message is a notification about a new peer
        if (strncmp(buffer, "New peer connected", 18) == 0) {
            safe_print("Server notification: %s", buffer);

            // Send GET_PEER_LIST request to the server
            request_peer_list(sock);
        }
        // Check if the message is the updated peer list
        else if (strncmp(buffer, "PEER_LIST\n", 10) == 0) {
            char *peer_list_str = buffer + 10;
            update_peer_list(peer_list_str);
            safe_print("Peer list updated.\n");
        }
        else {
            safe_print("Server message: %s", buffer);
        }
    }

    // Connection closed
    safe_print("Disconnected from server.\n");
    close(sock);
    pthread_exit(NULL);
}

void *handle_incoming_peer(void *arg) {
    int peer_sock = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Receive connection request from peer
    bytes_read = read(peer_sock, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        perror("read");
        close(peer_sock);
        pthread_exit(NULL);
    }
    buffer[bytes_read] = '\0';
    trim_newline(buffer);

    // Check if it's a CONNECT_REQUEST
    if (strncmp(buffer, "CONNECT_REQUEST", 15) == 0) {
        char peer_username[USERNAME_MAX_LENGTH];
        sscanf(buffer + 16, "%s", peer_username);

        // Prompt user to accept or deny the connection
        safe_print("Received connection request from '%s'. Accept? (yes/no): ", peer_username);
        fgets(buffer, sizeof(buffer), stdin);
        trim_newline(buffer);

        if (strcmp(buffer, "yes") == 0) {
            // Send ACCEPT response
            snprintf(buffer, sizeof(buffer), "ACCEPT\n");
            write(peer_sock, buffer, strlen(buffer));

            // Remove from discoverable list
            snprintf(buffer, sizeof(buffer), "REMOVE %s\n", username_global);
            write(server_sock, buffer, strlen(buffer));

            safe_print("Connection accepted with '%s'.\n", peer_username);

            in_chat = 1; // Set in_chat flag

            // Start chat session
            struct chat_info *chat = malloc(sizeof(struct chat_info));
            if (chat == NULL) {
                perror("malloc");
                close(peer_sock);
                in_chat = 0;
                pthread_exit(NULL);
            }
            chat->sock = peer_sock;
            strcpy(chat->peer_username, peer_username);
            strcpy(chat->your_username, username_global);

            // Generate Kyber key pair
            unsigned char *public_key = NULL, *secret_key = NULL;
            size_t public_key_len = 0, secret_key_len = 0;
            if (pqcrypto_generate_keypair(&public_key, &public_key_len, &secret_key, &secret_key_len) != 0) {
                fprintf(stderr, "Failed to generate Kyber key pair\n");
                close(peer_sock);
                free(chat);
                in_chat = 0;
                pthread_exit(NULL);
            }

            // Receive peer's public key length and public key
            uint32_t peer_public_key_len;
            if (read(peer_sock, &peer_public_key_len, sizeof(peer_public_key_len)) <= 0) {
                perror("read");
                close(peer_sock);
                free(public_key);
                free(secret_key);
                free(chat);
                in_chat = 0;
                pthread_exit(NULL);
            }
            peer_public_key_len = ntohl(peer_public_key_len);

            unsigned char *peer_public_key = malloc(peer_public_key_len);
            if (peer_public_key == NULL) {
                perror("malloc");
                close(peer_sock);
                free(public_key);
                free(secret_key);
                free(chat);
                in_chat = 0;
                pthread_exit(NULL);
            }

            if (read(peer_sock, peer_public_key, peer_public_key_len) <= 0) {
                perror("read");
                close(peer_sock);
                free(public_key);
                free(secret_key);
                free(peer_public_key);
                free(chat);
                in_chat = 0;
                pthread_exit(NULL);
            }

            // Derive shared secret and initialize encryption context
            if (pqcrypto_derive_shared_secret(&chat->enc_ctx, peer_public_key, peer_public_key_len,
                                             secret_key, secret_key_len) != 0) {
                fprintf(stderr, "Failed to derive shared secret\n");
                close(peer_sock);
                free(public_key);
                free(secret_key);
                free(peer_public_key);
                free(chat);
                in_chat = 0;
                pthread_exit(NULL);
            }

            // Send own public key length and public key to peer
            uint32_t net_public_key_len = htonl(public_key_len);
            if (write(peer_sock, &net_public_key_len, sizeof(net_public_key_len)) <= 0) {
                perror("write");
                close(peer_sock);
                free(public_key);
                free(secret_key);
                free(peer_public_key);
                free(chat);
                in_chat = 0;
                pthread_exit(NULL);
            }
            if (write(peer_sock, public_key, public_key_len) <= 0) {
                perror("write");
                close(peer_sock);
                free(public_key);
                free(secret_key);
                free(peer_public_key);
                free(chat);
                in_chat = 0;
                pthread_exit(NULL);
            }

            // Send AES IV to peer
            if (write(peer_sock, chat->enc_ctx.iv, AES_GCM_IV_SIZE) <= 0) {
                perror("write");
                close(peer_sock);
                free(public_key);
                free(secret_key);
                free(peer_public_key);
                free(chat);
                in_chat = 0;
                pthread_exit(NULL);
            }

            // Clean up keys
            free(public_key);
            free(secret_key);
            free(peer_public_key);

            // Start chat threads
            pthread_t send_thread, receive_thread;
            if (pthread_create(&send_thread, NULL, send_messages, (void *)chat) != 0) {
                perror("pthread_create");
                close(peer_sock);
                free(chat);
                in_chat = 0;
                pthread_exit(NULL);
            }
            if (pthread_create(&receive_thread, NULL, receive_messages, (void *)chat) != 0) {
                perror("pthread_create");
                close(peer_sock);
                free(chat);
                in_chat = 0;
                pthread_exit(NULL);
            }

            // Wait for chat to end
            pthread_join(send_thread, NULL);
            pthread_join(receive_thread, NULL);

            safe_print("Chat with '%s' ended.\n", peer_username);
            close(peer_sock);
            in_chat = 0; // Reset in_chat flag

            // Re-register with the server
            snprintf(buffer, sizeof(buffer), "REGISTER %s %d\n", username_global, peer_port);
            write(server_sock, buffer, strlen(buffer));

            // Request updated peer list
            request_peer_list(server_sock);

            pthread_exit(NULL);

        } else {
            // Send DENY response
            snprintf(buffer, sizeof(buffer), "DENY\n");
            write(peer_sock, buffer, strlen(buffer));

            close(peer_sock);
            pthread_exit(NULL);
        }
    } else {
        // Unknown command
        safe_print("Received unknown command from peer.\n");
        close(peer_sock);
        pthread_exit(NULL);
    }
}

void *peer_listener(void *arg) {
    int port = *(int *)arg;

    int listen_sock = bind_and_listen(port);
    if (listen_sock < 0) {
        fprintf(stderr, "Failed to bind and listen on port %d.\n", port);
        pthread_exit(NULL);
    }

    printf("Listening for incoming peer connections on port %d...\n", port);

    while (1) {
        int *peer_sock = malloc(sizeof(int));
        if (peer_sock == NULL) {
            perror("malloc");
            continue;
        }

        struct sockaddr_in peer_addr;
        socklen_t addr_size = sizeof(peer_addr);
        *peer_sock = accept(listen_sock, (struct sockaddr *)&peer_addr, &addr_size);
        if (*peer_sock < 0) {
            perror("accept");
            free(peer_sock);
            continue;
        }

        // Handle the incoming peer connection in a new thread
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_incoming_peer, (void *)peer_sock) != 0) {
            perror("pthread_create");
            close(*peer_sock);
            free(peer_sock);
            continue;
        }
        pthread_detach(thread_id);
    }

    close(listen_sock);
    pthread_exit(NULL);
}
