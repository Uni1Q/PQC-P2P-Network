// routing_server.c
#include "routing_server.h"
#include "peer_registry.h"
#include "network.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100

// Define client_mutex
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;

// List of connected client sockets
int client_sockets[MAX_CLIENTS];
int client_count = 0;

void *command_handler(void *arg) {
    (void)arg; // Suppress unused parameter warning
    char command[BUFFER_SIZE];
    while (1) {
        safe_print("Enter command: ");
        fflush(stdout);
        if (fgets(command, sizeof(command), stdin) != NULL) {
            // Remove newline character
            trim_newline(command);
            if (strcmp(command, "showpeer") == 0) {
                pthread_mutex_lock(&peer_list_mutex);
                safe_print("Discoverable peers:\n");
                for (int i = 0; i < peer_count; i++) {
                    safe_print("%s %s %d\n",
                               peer_list[i].username,
                               peer_list[i].ip,
                               peer_list[i].port);
                }
                pthread_mutex_unlock(&peer_list_mutex);
            } else {
                safe_print("Unknown command.\n");
            }
        }
    }
    return NULL;
}

void broadcast_message(const char *message, int exclude_sock) {
    pthread_mutex_lock(&client_mutex);
    for (int i = 0; i < client_count; i++) {
        int client_sock = client_sockets[i];
        if (client_sock != exclude_sock) {
            if (write(client_sock, message, strlen(message)) < 0) {
                perror("write");
            }
        }
    }
    pthread_mutex_unlock(&client_mutex);
}

void remove_client_socket(int client_sock) {
    pthread_mutex_lock(&client_mutex);
    for (int i = 0; i < client_count; i++) {
        if (client_sockets[i] == client_sock) {
            client_sockets[i] = client_sockets[client_count - 1];
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&client_mutex);
}

void *handle_client(void *arg) {
    int client_sock = *(int *)arg;
    free(arg);
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    getpeername(client_sock, (struct sockaddr *)&client_addr, &addr_len);
    char client_ip[IP_STR_LEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));

    safe_print("[%s] connected.\n", client_ip);

    char username[USERNAME_MAX_LENGTH] = {0};
    int registered = 0;

    // Add client socket to the list
    pthread_mutex_lock(&client_mutex);
    client_sockets[client_count++] = client_sock;
    pthread_mutex_unlock(&client_mutex);

    // Registration loop
    while (!registered) {
        // Read initial request from client
        bytes_read = read(client_sock, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) {
            safe_print("\n[%s] disconnected during registration.\n", client_ip);
            remove_client_socket(client_sock);
            close(client_sock);
            pthread_exit(NULL);
        }
        buffer[bytes_read] = '\0';

        if (strncmp(buffer, "REGISTER", 8) == 0) {
            // Client wants to become discoverable
            int peer_port;
            sscanf(buffer + 9, "%s %d", username, &peer_port);

            if (username_exists(username)) {
                // Username is taken
                char response[] = "USERNAME_TAKEN\n";
                write(client_sock, response, strlen(response));
                safe_print("\n[%s] attempted to register with taken username '%s'.\n", client_ip, username);
            } else {
                // Username is available
                add_peer(username, client_ip, peer_port);

                safe_print("[%s] [%s] registered.\n", client_ip, username);

                // Send list of discoverable peers to client
                pthread_mutex_lock(&peer_list_mutex);
                char peer_list_str[BUFFER_SIZE];
                peer_list_str[0] = '\0';
                for (int i = 0; i < peer_count; i++) {
                    if (strcmp(peer_list[i].username, username) != 0) {
                        strcat(peer_list_str, peer_list[i].username);
                        strcat(peer_list_str, " ");
                        strcat(peer_list_str, peer_list[i].ip);
                        strcat(peer_list_str, " ");
                        char port_str[6];
                        sprintf(port_str, "%d", peer_list[i].port);
                        strcat(peer_list_str, port_str);
                        strcat(peer_list_str, "\n");
                    }
                }
                pthread_mutex_unlock(&peer_list_mutex);

                // Prefix with "PEER_LIST\n"
                char message[BUFFER_SIZE * 2];
                snprintf(message, sizeof(message), "PEER_LIST\n%s", peer_list_str);

                write(client_sock, message, strlen(message));

                safe_print("[%s] [%s] received peer list.\n", client_ip, username);

                // Broadcast to other clients about new peer
                char notification[BUFFER_SIZE];
                snprintf(notification, sizeof(notification), "New peer connected: %s\n", username);
                broadcast_message(notification, client_sock);

                registered = 1; // Registration successful
            }
        } else {
            // Unknown command or not REGISTER
            char response[] = "INVALID_COMMAND\n";
            write(client_sock, response, strlen(response));
        }
    }

    // Main loop to handle further client requests
    while ((bytes_read = read(client_sock, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';

        if (strncmp(buffer, "GET_PEER_LIST", 13) == 0) {
            // Send the updated peer list to the client
            pthread_mutex_lock(&peer_list_mutex);
            char peer_list_str[BUFFER_SIZE];
            peer_list_str[0] = '\0';
            for (int i = 0; i < peer_count; i++) {
                if (strcmp(peer_list[i].username, username) != 0) {
                    strcat(peer_list_str, peer_list[i].username);
                    strcat(peer_list_str, " ");
                    strcat(peer_list_str, peer_list[i].ip);
                    strcat(peer_list_str, " ");
                    char port_str[6];
                    sprintf(port_str, "%d", peer_list[i].port);
                    strcat(peer_list_str, port_str);
                    strcat(peer_list_str, "\n");
                }
            }
            pthread_mutex_unlock(&peer_list_mutex);

            // Prefix with "PEER_LIST\n"
            char message[BUFFER_SIZE * 2];
            snprintf(message, sizeof(message), "PEER_LIST\n%s", peer_list_str);

            write(client_sock, message, strlen(message));
        } else if (strncmp(buffer, "REMOVE", 6) == 0) {
            // Client wants to be removed from discoverable list
            sscanf(buffer + 7, "%s", username);

            remove_peer(username);

            safe_print("[%s] [%s] disconnected.\n", client_ip, username);

            // Remove client socket from the list
            remove_client_socket(client_sock);
            break; // Exit loop and close connection
        } else if (strncmp(buffer, "PEER_DISCONNECTED", 17) == 0) {
            // Handle peer disconnection notification
            char disconnected_username[USERNAME_MAX_LENGTH];
            sscanf(buffer + 18, "%s", disconnected_username);

            remove_peer(disconnected_username);

            safe_print("\nPeer '%s' reported as disconnected by [%s].\n", disconnected_username, username);
        } else {
            // Unknown command
            char response[] = "INVALID_COMMAND\n";
            write(client_sock, response, strlen(response));
        }
    }

    // If the client disconnected unexpectedly
    if (bytes_read <= 0) {
        safe_print("\n[%s] [%s] disconnected unexpectedly.\n", client_ip, username);
    }

    // Remove client from peers list if registered
    if (registered) {
        remove_peer(username);
    }

    // Remove client socket from the list
    remove_client_socket(client_sock);

    safe_print("[%s] [%s] connection closed.\n", client_ip, username);
    close(client_sock);
    pthread_exit(NULL);
}

