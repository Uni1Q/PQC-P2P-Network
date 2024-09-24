//
// Created by rokas on 24/09/2024.
//
#include "client.h"
#include "chat.h"
#include "utils.h"
#include "network.h"
#include "peer_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1" // 127.0.0.1 for local testing, 159.89.248.152 to connect to droplet
#define SERVER_PORT 5453
#define BUFFER_SIZE 1024

extern int server_sock;
extern char username_global[USERNAME_MAX_LENGTH];
extern int peer_port;
extern volatile int in_chat;

int main() {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    pthread_t listener_thread, peer_listener_thread;

    // Connect to the server
    server_sock = connect_to_server(SERVER_IP, SERVER_PORT);
    if (server_sock < 0) {
        safe_print("Failed to connect to the server.\n");
        exit(1);
    }

    // Registration process
    safe_print("Do you want to become discoverable? (yes/no): ");
    fgets(buffer, sizeof(buffer), stdin);
    trim_newline(buffer);

if (strcmp(buffer, "yes") == 0) {
    safe_print("Enter port to listen for incoming connections: ");
    fgets(buffer, sizeof(buffer), stdin);
    trim_newline(buffer);
    peer_port = atoi(buffer);

    // Registration loop
    while (1) {
        safe_print("Enter your preferred username: ");
        fgets(username_global, sizeof(username_global), stdin);
        trim_newline(username_global);

        // Send registration request to the server
        snprintf(buffer, sizeof(buffer), "REGISTER %s %d", username_global, peer_port);
        safe_print("Sending to server: '%s'\n", buffer); // Debugging statement
        if (write(server_sock, buffer, strlen(buffer)) < 0) {
            perror("write");
            close(server_sock);
            exit(1);
        }

        // Read the server's response
        bytes_read = read(server_sock, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) {
            perror("read");
            close(server_sock);
            exit(1);
        }
        buffer[bytes_read] = '\0';

        if (strncmp(buffer, "USERNAME_TAKEN", 14) == 0) {
            safe_print("Username '%s' is already taken. Please choose a different username.\n", username_global);
        } else if (strncmp(buffer, "INVALID_COMMAND", 15) == 0) {
            safe_print("Invalid registration command.\n");
            close(server_sock);
            exit(1);
        } else if (strncmp(buffer, "PEER_LIST\n", 10) == 0) {
            // Registration successful
            // Update the local peer list
            char *peer_list_str = buffer + 10;
            update_peer_list(peer_list_str);
            safe_print("Successfully registered with the server.\n");
            break;
        } else {
            safe_print("Unexpected response from server: %s\n", buffer);
        }
    }

        // Start the peer listener thread after successful registration
        if (pthread_create(&peer_listener_thread, NULL, peer_listener, &peer_port) != 0) {
            perror("pthread_create");
            close(server_sock);
            exit(1);
        }

        // Start the server listener thread
        if (pthread_create(&listener_thread, NULL, server_listener, &server_sock) != 0) {
            perror("pthread_create");
            close(server_sock);
            exit(1);
        }

        // Request the initial peer list from the server
        request_peer_list(server_sock);

        // Start listening for incoming peer connections
        if (pthread_create(&peer_listener_thread, NULL, peer_listener, &peer_port) != 0) {
            perror("pthread_create");
            close(server_sock);
            exit(1);
        }

        // Main loop for user input
        while (1) {
            if (in_chat) {
                // If in chat, wait until chat is over
                sleep(1);
                continue;
            }

            display_peer_list();

            safe_print("Enter the username of the peer to connect to (or 'exit' to quit): ");
            fgets(buffer, sizeof(buffer), stdin);
            trim_newline(buffer);
            if (strcmp(buffer, "exit") == 0) {
                // Send REMOVE command to the server
                snprintf(buffer, sizeof(buffer), "REMOVE %s\n", username_global);
                if (write(server_sock, buffer, strlen(buffer)) < 0) {
                    perror("write");
                }
                break;
            }

            char selected_username[USERNAME_MAX_LENGTH];
            strncpy(selected_username, buffer, USERNAME_MAX_LENGTH - 1);
            selected_username[USERNAME_MAX_LENGTH - 1] = '\0';

            if (strcmp(selected_username, username_global) == 0) {
                safe_print("You cannot connect to yourself. Please select a different peer.\n");
                continue;
            }

            // Check if the selected peer is in the peer list
            pthread_mutex_lock(&peer_list_mutex);
            int peer_found = 0;
            char peer_ip[IP_STR_LEN];
            int selected_peer_port;
            for (int i = 0; i < peer_count; i++) {
                if (strcmp(peer_list[i].username, selected_username) == 0) {
                    strncpy(peer_ip, peer_list[i].ip, IP_STR_LEN);
                    selected_peer_port = peer_list[i].port;
                    peer_found = 1;
                    break;
                }
            }
            pthread_mutex_unlock(&peer_list_mutex);

            if (!peer_found) {
                safe_print("Peer not found. Please try again.\n");
                continue;
            } else {
                // Start P2P connection
                int peer_sock = connect_to_server(peer_ip, selected_peer_port);
                if (peer_sock < 0) {
                    safe_print("Could not connect to peer. The peer may no longer be connected.\n");
                    // Inform the server that the peer is no longer connected
                    snprintf(buffer, sizeof(buffer), "PEER_DISCONNECTED %s\n", selected_username);
                    write(server_sock, buffer, strlen(buffer));

                    // Request updated peer list
                    request_peer_list(server_sock);
                    continue;
                } else {
                    safe_print("Connected to peer at %s:%d\n", peer_ip, selected_peer_port);

                    // Send connection request with your username
                    snprintf(buffer, sizeof(buffer), "CONNECT_REQUEST %s\n", username_global);
                    write(peer_sock, buffer, strlen(buffer));

                    // Wait for response
                    bytes_read = read(peer_sock, buffer, sizeof(buffer) - 1);
                    if (bytes_read <= 0) {
                        safe_print("No response from peer.\n");
                        close(peer_sock);
                        continue;
                    }
                    buffer[bytes_read] = '\0';
                    trim_newline(buffer);

                    if (strcmp(buffer, "ACCEPT") == 0) {
                        // Remove from discoverable list
                        snprintf(buffer, sizeof(buffer), "REMOVE %s\n", username_global);
                        write(server_sock, buffer, strlen(buffer));

                        safe_print("Connection accepted by '%s'.\n", selected_username);

                        in_chat = 1; // Set in_chat flag

                        // Start chat session
                        struct chat_info *chat = malloc(sizeof(struct chat_info));
                        chat->sock = peer_sock;
                        strcpy(chat->peer_username, selected_username);
                        strcpy(chat->your_username, username_global);

                        // Start chat threads
                        pthread_t send_thread, receive_thread;
                        if (pthread_create(&send_thread, NULL, send_messages, (void *)chat) != 0) {
                            perror("pthread_create");
                            close(peer_sock);
                            free(chat);
                            in_chat = 0;
                            continue;
                        }
                        if (pthread_create(&receive_thread, NULL, receive_messages, (void *)chat) != 0) {
                            perror("pthread_create");
                            close(peer_sock);
                            free(chat);
                            in_chat = 0;
                            continue;
                        }

                        // Wait for chat to end
                        pthread_join(send_thread, NULL);
                        pthread_join(receive_thread, NULL);

                        safe_print("Chat with '%s' ended.\n", selected_username);
                        close(peer_sock);
                        in_chat = 0; // Reset in_chat flag

                        // Re-register with the server
                        snprintf(buffer, sizeof(buffer), "REGISTER %s %d\n", username_global, peer_port);
                        write(server_sock, buffer, strlen(buffer));

                        // Request updated peer list
                        request_peer_list(server_sock);
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

        // Clean up threads
        pthread_cancel(listener_thread);
        pthread_cancel(peer_listener_thread);
        pthread_join(listener_thread, NULL);
        pthread_join(peer_listener_thread, NULL);

        // Close the routing server socket
        close(server_sock);
    } else {
        // Do not become discoverable
        safe_print("You chose not to become discoverable.\n");

        // Main loop for non-discoverable clients
        while (1) {
            safe_print("Enter the IP and port of the peer to connect to (or 'exit' to quit): ");
            fgets(buffer, sizeof(buffer), stdin);
            trim_newline(buffer);

            if (strcmp(buffer, "exit") == 0) {
                break;
            }

            // Parse IP and port
            char peer_ip[INET_ADDRSTRLEN];
            int selected_peer_port;
            if (sscanf(buffer, "%s %d", peer_ip, &selected_peer_port) != 2) {
                safe_print("Invalid input. Please enter IP and port.\n");
                continue;
            }

            // Start P2P connection
            int peer_sock = connect_to_server(peer_ip, selected_peer_port);
            if (peer_sock < 0) {
                safe_print("Could not connect to peer.\n");
                continue;
            }

            safe_print("Connected to peer at %s:%d\n", peer_ip, selected_peer_port);

            // Send connection request with your username
            snprintf(buffer, sizeof(buffer), "CONNECT_REQUEST %s", "Anonymous");
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
                in_chat = 1; // Set in_chat flag

                // Start chat session
                struct chat_info *chat = malloc(sizeof(struct chat_info));
                chat->sock = peer_sock;
                strcpy(chat->peer_username, "Unknown");
                strcpy(chat->your_username, "Anonymous");

                // Start chat threads
                pthread_t send_thread, receive_thread;
                if (pthread_create(&send_thread, NULL, send_messages, (void *)chat) != 0) {
                    perror("pthread_create");
                    close(peer_sock);
                    free(chat);
                    in_chat = 0;
                    continue;
                }
                if (pthread_create(&receive_thread, NULL, receive_messages, (void *)chat) != 0) {
                    perror("pthread_create");
                    close(peer_sock);
                    free(chat);
                    in_chat = 0;
                    continue;
                }

                // Wait for chat to end
                pthread_join(send_thread, NULL);
                pthread_join(receive_thread, NULL);

                safe_print("Connection closed.\n");
                close(peer_sock);
                in_chat = 0; // Reset in_chat flag
            } else if (strcmp(buffer, "DENY") == 0) {
                safe_print("Connection request denied.\n");
                close(peer_sock);
                // Continue
            } else {
                safe_print("Received unknown response from peer.\n");
                close(peer_sock);
                // Continue
            }
        }

        // Close the routing server socket
        close(server_sock);
    }

    return 0;
}
