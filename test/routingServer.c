//
// Created by rokas on 23/09/2024.
//

// routingServer.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>

#define SERVER_PORT 5453
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024

typedef struct {
    char username[50];
    char ip[INET_ADDRSTRLEN];
    int port;
} PeerInfo;

PeerInfo peers[MAX_CLIENTS];
int peer_count = 0;
pthread_mutex_t peer_mutex;

// List of connected client sockets
int client_sockets[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t client_mutex;

void *command_handler(void *arg) {
    (void)arg; // Suppress unused parameter warning

    char command[BUFFER_SIZE];
    while (1) {
        printf("Enter command: ");
        fflush(stdout);
        if (fgets(command, sizeof(command), stdin) != NULL) {
            // Remove newline character
            size_t len = strlen(command);
            if (len > 0 && command[len - 1] == '\n') {
                command[len - 1] = '\0';
            }
            if (strcmp(command, "showpeer") == 0) {
                pthread_mutex_lock(&peer_mutex);
                printf("Discoverable peers:\n");
                for (int i = 0; i < peer_count; i++) {
                    printf("%s %s %d\n", peers[i].username, peers[i].ip, peers[i].port);
                }
                pthread_mutex_unlock(&peer_mutex);
            } else {
                printf("Unknown command.\n");
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

int username_exists(const char *username) {
    pthread_mutex_lock(&peer_mutex);
    for (int i = 0; i < peer_count; i++) {
        if (strcmp(peers[i].username, username) == 0) {
            pthread_mutex_unlock(&peer_mutex);
            return 1; // Username exists
        }
    }
    pthread_mutex_unlock(&peer_mutex);
    return 0; // Username does not exist
}


void *handle_client(void *arg) {
    int client_sock = *(int *)arg;
    free(arg);
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    getpeername(client_sock, (struct sockaddr *)&client_addr, &addr_len);
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    printf("[%s] connected.\n", client_ip);

    char username[50] = {0};
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
            printf("\n[%s] disconnected during registration.\n", client_ip);
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
                char response[] = "USERNAME_TAKEN";
                write(client_sock, response, strlen(response));
                printf("\n[%s] attempted to register with taken username '%s'.\n", client_ip, username);
            } else {
                // Username is available
                pthread_mutex_lock(&peer_mutex);
                strcpy(peers[peer_count].username, username);
                strcpy(peers[peer_count].ip, client_ip);
                peers[peer_count].port = peer_port;
                peer_count++;
                pthread_mutex_unlock(&peer_mutex);

                printf("[%s] [%s] connected.\n", client_ip, username);

                // Send list of discoverable peers to client
                pthread_mutex_lock(&peer_mutex);
                buffer[0] = '\0';
                for (int i = 0; i < peer_count; i++) {
                    strcat(buffer, peers[i].username);
                    strcat(buffer, " ");
                    strcat(buffer, peers[i].ip);
                    strcat(buffer, " ");
                    char port_str[6];
                    sprintf(port_str, "%d", peers[i].port);
                    strcat(buffer, port_str);
                    strcat(buffer, "\n");
                }
                pthread_mutex_unlock(&peer_mutex);
                write(client_sock, buffer, strlen(buffer));

                printf("[%s] [%s] requested List of peers.\n", client_ip, username);

                // Broadcast to other clients about new peer
                char notification[BUFFER_SIZE];
                snprintf(notification, sizeof(notification), "New peer connected, refresh list (yes/no)?\n");
                broadcast_message(notification, client_sock);

                registered = 1; // Registration successful
            }
        } else {
            // Unknown command or not REGISTER
            char response[] = "INVALID_COMMAND";
            write(client_sock, response, strlen(response));
        }
    }

    // Main loop to handle further client requests
    while ((bytes_read = read(client_sock, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';

        if (strncmp(buffer, "REMOVE", 6) == 0) {
            // Client wants to be removed from discoverable list
            sscanf(buffer + 7, "%s", username);

            pthread_mutex_lock(&peer_mutex);
            for (int i = 0; i < peer_count; i++) {
                if (strcmp(peers[i].username, username) == 0) {
                    // Remove this peer
                    peers[i] = peers[peer_count - 1];
                    peer_count--;
                    break;
                }
            }
            pthread_mutex_unlock(&peer_mutex);

            printf("[%s] [%s] disconnected.\n", client_ip, username);

            // Remove client socket from the list
            remove_client_socket(client_sock);
            break; // Exit loop and close connection
        } else if (strncmp(buffer, "PEER_DISCONNECTED", 17) == 0) {
            // Handle peer disconnection notification
            char disconnected_username[50];
            sscanf(buffer + 18, "%s", disconnected_username);

            pthread_mutex_lock(&peer_mutex);
            for (int i = 0; i < peer_count; i++) {
                if (strcmp(peers[i].username, disconnected_username) == 0) {
                    // Remove this peer
                    peers[i] = peers[peer_count - 1];
                    peer_count--;
                    break;
                }
            }
            pthread_mutex_unlock(&peer_mutex);

            printf("\nPeer '%s' reported as disconnected by [%s].\n", disconnected_username, username);
        }
    }

    // If the client disconnected unexpectedly
    if (bytes_read <= 0) {
        printf("\n[%s] [%s] disconnected unexpectedly.\n", client_ip, username);
    }

    // Remove client from peers list if registered
    if (registered) {
        pthread_mutex_lock(&peer_mutex);
        for (int i = 0; i < peer_count; i++) {
            if (strcmp(peers[i].username, username) == 0) {
                // Remove this peer
                peers[i] = peers[peer_count - 1];
                peer_count--;
                break;
            }
        }
        pthread_mutex_unlock(&peer_mutex);
    }

    // Remove client socket from the list
    remove_client_socket(client_sock);

    printf("[%s] [%s] connection closed.\n", client_ip, username);
    close(client_sock);
    pthread_exit(NULL);
}

int main() {
    int server_sock, *new_sock;
    struct sockaddr_in server_addr, client_addr;
    pthread_t tid, cmd_tid;
    socklen_t client_len = sizeof(client_addr);

    pthread_mutex_init(&peer_mutex, NULL);
    pthread_mutex_init(&client_mutex, NULL);

    // Create socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket");
        exit(1);
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR,
                   &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(1);
    }

    // Bind
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY; // Accept connections on any interface

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    // Listen
    if (listen(server_sock, 10) < 0) {
        perror("listen");
        exit(1);
    }

    printf("Routing server listening on port %d\n", SERVER_PORT);

    // Start command handler thread
    if (pthread_create(&cmd_tid, NULL, command_handler, NULL) != 0) {
        perror("pthread_create");
        exit(1);
    }

    while (1) {
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("accept");
            continue;
        }

        new_sock = malloc(sizeof(int));
        if (new_sock == NULL) {
            perror("malloc");
            close(client_sock);
            continue;
        }
        *new_sock = client_sock;
        if (pthread_create(&tid, NULL, handle_client, (void *)new_sock) != 0) {
            perror("pthread_create");
            close(client_sock);
            free(new_sock);
        } else {
            pthread_detach(tid);
        }
    }

    close(server_sock);
    pthread_mutex_destroy(&peer_mutex);
    pthread_mutex_destroy(&client_mutex);
    return 0;
}
