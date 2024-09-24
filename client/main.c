//
// Created by rokas on 24/09/2024.
//

// main.c
#include "client.h"
#include "network.h"
#include "utils.h"
#include "peer_list.h"
#include "chat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define ROUTING_SERVER_IP "127.0.0.1" // 127.0.0.1 for local testing, 159.89.248.152 to connect to droplet
#define ROUTING_SERVER_PORT 5453
#define BUFFER_SIZE 1024

int main() {
    int sock;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    pthread_t listener_thread, server_listener_thread;

    // Connect to routing server
    sock = connect_to_server(ROUTING_SERVER_IP, ROUTING_SERVER_PORT);
    if (sock < 0) {
        safe_print("Could not connect to routing server.\n");
        exit(1);
    }
    routing_server_sock = sock; // Set global variable

    // Ask if user wants to become discoverable
    safe_print("Do you want to become discoverable? (yes/no): ");
    fgets(buffer, sizeof(buffer), stdin);
    trim_newline(buffer);
    if (strcmp(buffer, "yes") == 0) {
        // Ask which port the client would like to open for communication
        safe_print("Enter port to listen for incoming connections: ");
        fgets(buffer, sizeof(buffer), stdin);
        listen_port_global = atoi(buffer);

        // Start the listener thread for incoming connections
        if (pthread_create(&listener_thread, NULL, handle_incoming_connections, &listen_port_global) != 0) {
            perror("pthread_create");
            exit(1);
        }

        // Registration loop
        int registered = 0;
        while (!registered) {
            safe_print("Enter your preferred username: ");
            fgets(username_global, sizeof(username_global), stdin);
            trim_newline(username_global);

            // Register with routing server
            register_with_server(sock, username_global, listen_port_global);

            // Read server response
            bytes_read = read(sock, buffer, sizeof(buffer) - 1);
            if (bytes_read <= 0) {
                safe_print("Failed to receive response from server.\n");
                exit(1);
            }
            buffer[bytes_read] = '\0';

            if (strcmp(buffer, "USERNAME_TAKEN") == 0) {
                safe_print("Username '%s' is already taken. Please choose a different username.\n", username_global);
            } else {
                // Assume registration successful and buffer contains the peer list
                registered = 1;
            }
        }

        // Start server listener thread
        if (pthread_create(&server_listener_thread, NULL, server_listener, username_global) != 0) {
            perror("pthread_create");
            exit(1);
        }

        // Receive list of discoverable peers
        update_peer_list(buffer);

        while (1) {
            if (in_chat) {
                // If in chat, wait until chat is over
                sleep(1);
                continue;
            }

            display_peer_list(username_global);

            safe_print("Enter the username of the peer to connect to (or 'no' to skip): ");
            fgets(buffer, sizeof(buffer), stdin);
            trim_newline(buffer);
            if (strcmp(buffer, "no") == 0) {
                break;
            }

            char selected_username[50];
            strcpy(selected_username, buffer);

            if (strcmp(selected_username, username_global) == 0) {
                safe_print("You cannot connect to yourself. Please select a different peer.\n");
                continue;
            }

            // Parse the peer list to find the selected peer's IP and port
            pthread_mutex_lock(&peer_list_mutex);
            char peer_list_copy[BUFFER_SIZE];
            strncpy(peer_list_copy, peer_list, BUFFER_SIZE - 1);
            peer_list_copy[BUFFER_SIZE - 1] = '\0';
            pthread_mutex_unlock(&peer_list_mutex);

            char *line = strtok(peer_list_copy, "\n");
            char peer_ip[INET_ADDRSTRLEN];
            int peer_port;
            int peer_found = 0;
            while (line != NULL) {
                char peer_name[50], peer_address[INET_ADDRSTRLEN];
                int port;
                if (sscanf(line, "%s %s %d", peer_name, peer_address, &port) == 3) {
                    if (strcmp(peer_name, selected_username) == 0) {
                        strcpy(peer_ip, peer_address);
                        peer_port = port;
                        peer_found = 1;
                        break;
                    }
                }
                line = strtok(NULL, "\n");
            }

            if (!peer_found) {
                safe_print("Peer not found. Please try again.\n");
                continue;
            } else {
                // Start P2P connection
                int peer_sock = connect_to_server(peer_ip, peer_port);
                if (peer_sock < 0) {
                    safe_print("Could not connect to peer. The peer may no longer be connected.\n");
                    // Inform the server that the peer is no longer connected
                    snprintf(buffer, sizeof(buffer), "PEER_DISCONNECTED %s", selected_username);
                    write(sock, buffer, strlen(buffer));

                    // Receive updated peer list
                    bytes_read = read(sock, buffer, sizeof(buffer) - 1);
                    if (bytes_read > 0) {
                        buffer[bytes_read] = '\0';
                        update_peer_list(buffer);
                        safe_print("Updated list of discoverable peers:\n");
                        display_peer_list(username_global);
                    }
                    close(peer_sock);
                    continue;
                } else {
                    safe_print("Connected to peer at %s:%d\n", peer_ip, peer_port);

                    // Send connection request with your username
                    snprintf(buffer, sizeof(buffer), "CONNECT_REQUEST %s", username_global);
                    write(peer_sock, buffer, strlen(buffer));

                    // Wait for response
                    bytes_read = read(peer_sock, buffer, sizeof(buffer) - 1);
                    if (bytes_read <= 0) {
                        safe_print("No response from peer.\n");
                        close(peer_sock);
                        continue;
                    }
                    buffer[bytes_read] = '\0';

                    if (strcmp(buffer, "ACCEPT") == 0) {
                        // Remove from discoverable list
                        snprintf(buffer, sizeof(buffer), "REMOVE %s", username_global);
                        write(sock, buffer, strlen(buffer));

                        safe_print("Connection accepted by '%s'.\n", selected_username);

                        in_chat = 1; // Set in_chat flag

                        // Start message exchange
                        pthread_t receive_thread;
                        struct chat_info *chat = malloc(sizeof(struct chat_info));

                        chat->sock = peer_sock;
                        strcpy(chat->peer_username, selected_username);
                        strcpy(chat->your_username, username_global);

                        if (pthread_create(&receive_thread, NULL, send_messages, (void *)chat) != 0) {
                            perror("pthread_create");
                            close(peer_sock);
                            free(chat);
                            in_chat = 0;
                            continue;
                        }

                        // Receive messages
                        while ((bytes_read = read(peer_sock, buffer, sizeof(buffer) - 1)) > 0) {
                            buffer[bytes_read] = '\0';
                            safe_print("[%s]: %s", selected_username, buffer);
                        }

                        safe_print("Connection with '%s' closed.\n", selected_username);
                        close(peer_sock);
                        in_chat = 0; // Reset in_chat flag
                    } else if (strcmp(buffer, "DENY") == 0) {
                        safe_print("Connection request denied by '%s'.\n", selected_username);
                        close(peer_sock);
                        // Continue and remain discoverable
                    } else {
                        safe_print("Received unknown response from peer.\n");
                        close(peer_sock);
                        // Continue and remain discoverable
                    }
                }
            }
        }

        // Send a kill request to the routing server to remove from discoverable list
        snprintf(buffer, sizeof(buffer), "REMOVE %s", username_global);
        write(sock, buffer, strlen(buffer));

        pthread_join(listener_thread, NULL);
        pthread_join(server_listener_thread, NULL);
    } else {
        // Do not become discoverable
        safe_print("You chose not to become discoverable.\n");
    }

    // Close the routing server socket
    close(sock);
    return 0;
}
