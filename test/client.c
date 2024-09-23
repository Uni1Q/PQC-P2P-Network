// client.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define ROUTING_SERVER_IP "127.0.0.1" // Replace with your routing server's IP
#define ROUTING_SERVER_PORT 5453
#define BUFFER_SIZE 1024

int routing_server_sock; // Global variable to access routing server socket
char peer_list[BUFFER_SIZE]; // Shared peer list
pthread_mutex_t peer_list_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t peer_list_updated = PTHREAD_COND_INITIALIZER;

char username_global[50]; // Global variable to store the user's username
int in_chat = 0; // Flag to indicate if the client is in a chat session

// Function prototypes
void *handle_incoming_connections(void *arg);
void *handle_peer_connection(void *arg);
void *send_messages(void *arg);
void *server_listener(void *arg); // Function prototype
void display_peer_list(const char *peer_list_str, const char *own_username);
void request_peer_list(int sock);

// Structure to hold chat information
struct chat_info {
    int sock;
    char peer_username[50];
    char your_username[50];
};

// Function to request the latest peer list from the routing server
void request_peer_list(int sock) {
    char buffer[BUFFER_SIZE];
    // Send a request to the server to get the latest peer list
    snprintf(buffer, sizeof(buffer), "GET_PEER_LIST\n"); // Added newline for clarity
    ssize_t bytes_sent = write(sock, buffer, strlen(buffer));
    if (bytes_sent <= 0) {
        pthread_mutex_lock(&print_mutex);
        printf("Failed to send GET_PEER_LIST request to server.\n");
        pthread_mutex_unlock(&print_mutex);
        return;
    }
    pthread_mutex_lock(&print_mutex);
    printf("Sent GET_PEER_LIST request to server.\n");
    pthread_mutex_unlock(&print_mutex);

    // Read the server's response
    ssize_t bytes_read = read(sock, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        pthread_mutex_lock(&print_mutex);
        printf("Failed to receive peer list from server.\n");
        pthread_mutex_unlock(&print_mutex);
        return;
    }
    buffer[bytes_read] = '\0';

    pthread_mutex_lock(&print_mutex);
    printf("Received response from server:\n%s\n", buffer);
    pthread_mutex_unlock(&print_mutex);

    // Update the peer list
    pthread_mutex_lock(&peer_list_mutex);
    strncpy(peer_list, buffer, BUFFER_SIZE - 1);
    peer_list[BUFFER_SIZE - 1] = '\0';
    pthread_mutex_unlock(&peer_list_mutex);
}

// Function to display the list of peers
void display_peer_list(const char *peer_list_str, const char *own_username) {
    pthread_mutex_lock(&print_mutex);
    printf("List of discoverable peers:\n");
    char peer_list_copy[BUFFER_SIZE];
    strncpy(peer_list_copy, peer_list_str, BUFFER_SIZE - 1);
    peer_list_copy[BUFFER_SIZE - 1] = '\0'; // Ensure null-termination
    char *line = strtok(peer_list_copy, "\n");
    int count = 0;
    while (line != NULL) {
        char peer_name[50], peer_address[INET_ADDRSTRLEN];
        int port;
        if (sscanf(line, "%s %s %d", peer_name, peer_address, &port) == 3) {
            if (strcmp(peer_name, own_username) != 0) {
                printf("Username: %s, IP: %s, Port: %d\n", peer_name, peer_address, port); // Detailed display
                count++;
            }
        }
        line = strtok(NULL, "\n");
    }
    if (count == 0) {
        printf("No available peers.\n");
    }
    pthread_mutex_unlock(&print_mutex);
}

// Function to handle incoming connections from peers
void *handle_peer_connection(void *arg) {
    int peer_sock = *(int *)arg;
    free(arg);
    char buffer[BUFFER_SIZE];
    char peer_username[50];
    int connection_accepted = 0;
    ssize_t bytes_read; // Single declaration to prevent shadowing

    // Read the connection request message
    bytes_read = read(peer_sock, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        pthread_mutex_lock(&print_mutex);
        printf("Connection closed by peer before handshake.\n");
        pthread_mutex_unlock(&print_mutex);
        close(peer_sock);
        pthread_exit(NULL);
    }
    buffer[bytes_read] = '\0';

    if (strncmp(buffer, "CONNECT_REQUEST", 15) == 0) {
        // Extract peer's username
        sscanf(buffer + 16, "%s", peer_username);

        pthread_mutex_lock(&print_mutex);
        printf("\nPeer '%s' wants to connect with you. Accept? (yes/no): ", peer_username);
        pthread_mutex_unlock(&print_mutex);
        fflush(stdout);

        char response[10];
        fgets(response, sizeof(response), stdin);
        response[strcspn(response, "\n")] = '\0'; // Remove newline

        if (strcmp(response, "yes") == 0) {
            // Send ACCEPT response
            strcpy(buffer, "ACCEPT\n"); // Added newline for consistency
            ssize_t bytes_sent = write(peer_sock, buffer, strlen(buffer));
            if (bytes_sent <= 0) {
                pthread_mutex_lock(&print_mutex);
                printf("Failed to send ACCEPT response to peer '%s'.\n", peer_username);
                pthread_mutex_unlock(&print_mutex);
                close(peer_sock);
                pthread_exit(NULL);
            }

            // Remove from discoverable list
            char remove_msg[BUFFER_SIZE];
            snprintf(remove_msg, sizeof(remove_msg), "REMOVE %s\n", username_global); // Added newline
            ssize_t remove_sent = write(routing_server_sock, remove_msg, strlen(remove_msg));
            if (remove_sent <= 0) {
                pthread_mutex_lock(&print_mutex);
                printf("Failed to send REMOVE request to server.\n");
                pthread_mutex_unlock(&print_mutex);
            }

            connection_accepted = 1;
            in_chat = 1; // Set in_chat flag
        } else {
            // Send DENY response
            strcpy(buffer, "DENY\n"); // Added newline for consistency
            ssize_t bytes_sent = write(peer_sock, buffer, strlen(buffer));
            if (bytes_sent <= 0) {
                pthread_mutex_lock(&print_mutex);
                printf("Failed to send DENY response to peer '%s'.\n", peer_username);
                pthread_mutex_unlock(&print_mutex);
            }
            pthread_mutex_lock(&print_mutex);
            printf("You denied the connection request from '%s'.\n", peer_username);
            pthread_mutex_unlock(&print_mutex);
            close(peer_sock);
            pthread_exit(NULL);
        }
    } else {
        pthread_mutex_lock(&print_mutex);
        printf("Invalid connection request.\n");
        pthread_mutex_unlock(&print_mutex);
        close(peer_sock);
        pthread_exit(NULL);
    }

    if (connection_accepted) {
        pthread_mutex_lock(&print_mutex);
        printf("Connection with '%s' established.\n", peer_username);
        pthread_mutex_unlock(&print_mutex);

        // Start message exchange
        pthread_t send_thread;
        struct chat_info *chat = malloc(sizeof(struct chat_info));

        if (chat == NULL) {
            pthread_mutex_lock(&print_mutex);
            printf("Failed to allocate memory for chat_info.\n");
            pthread_mutex_unlock(&print_mutex);
            close(peer_sock);
            pthread_exit(NULL);
        }

        chat->sock = peer_sock;
        strcpy(chat->peer_username, peer_username);
        strcpy(chat->your_username, username_global);

        if (pthread_create(&send_thread, NULL, send_messages, (void *)chat) != 0) {
            perror("pthread_create");
            close(peer_sock);
            free(chat);
            pthread_exit(NULL);
        }

        // Detach the thread as we don't need to join it
        pthread_detach(send_thread);

        // Receive messages
        while ((bytes_read = read(peer_sock, buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_read] = '\0';
            pthread_mutex_lock(&print_mutex);
            printf("[%s]: %s", peer_username, buffer);
            pthread_mutex_unlock(&print_mutex);
        }

        if (bytes_read == 0) {
            pthread_mutex_lock(&print_mutex);
            printf("Connection with '%s' closed by peer.\n", peer_username);
            pthread_mutex_unlock(&print_mutex);
        } else if (bytes_read < 0) {
            pthread_mutex_lock(&print_mutex);
            perror("read");
            printf("Error reading from peer '%s'.\n", peer_username);
            pthread_mutex_unlock(&print_mutex);
        }

        close(peer_sock);
        in_chat = 0; // Reset in_chat flag
        pthread_exit(NULL);
    }

    close(peer_sock);
    pthread_exit(NULL);
}

// Function to send messages to the connected peer
void *send_messages(void *arg) {
    struct chat_info *chat = (struct chat_info *)arg;
    char message[BUFFER_SIZE];
    ssize_t bytes_sent;

    while (1) {
        pthread_mutex_lock(&print_mutex);
        printf("[%s]: ", chat->your_username);
        pthread_mutex_unlock(&print_mutex);
        fflush(stdout);

        if (fgets(message, sizeof(message), stdin) == NULL) {
            // User closed input (e.g., pressed Ctrl+D)
            break;
        }

        bytes_sent = write(chat->sock, message, strlen(message));
        if (bytes_sent <= 0) {
            pthread_mutex_lock(&print_mutex);
            printf("Failed to send message. Connection may have been lost.\n");
            pthread_mutex_unlock(&print_mutex);
            break;
        }
    }

    close(chat->sock);
    free(chat);
    in_chat = 0; // Reset in_chat flag
    pthread_exit(NULL);
}

// Function to handle incoming connections from peers
void *handle_incoming_connections(void *arg) {
    int listen_port = *(int *)arg;
    free(arg);
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Create socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket");
        pthread_exit(NULL);
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR,
                   &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server_sock);
        pthread_exit(NULL);
    }

    // Bind
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(listen_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_sock);
        pthread_exit(NULL);
    }

    // Listen
    if (listen(server_sock, 5) < 0) {
        perror("listen");
        close(server_sock);
        pthread_exit(NULL);
    }

    pthread_mutex_lock(&print_mutex);
    printf("Listening for incoming connections on port %d...\n", listen_port);
    pthread_mutex_unlock(&print_mutex);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("accept");
            continue;
        }
        // Start a new thread to handle the connection
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
        pthread_detach(peer_thread); // Detach the thread as we don't need to join it
    }
    close(server_sock);
    pthread_exit(NULL);
}

// Function to listen to messages from the routing server
void *server_listener(void *arg) {
    (void)arg; // Suppress unused parameter warning

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    while (1) {
        bytes_read = read(routing_server_sock, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) {
            pthread_mutex_lock(&print_mutex);
            if (bytes_read == 0) {
                printf("Disconnected from routing server.\n");
            } else {
                perror("read");
                printf("Error reading from routing server.\n");
            }
            pthread_mutex_unlock(&print_mutex);
            break;
        }
        buffer[bytes_read] = '\0';

        pthread_mutex_lock(&print_mutex);
        printf("\nReceived from server: %s\n", buffer);
        pthread_mutex_unlock(&print_mutex);

        // Example: If server notifies about a new peer, refresh the list
        if (strstr(buffer, "NEW_PEER") != NULL && !in_chat) {
            pthread_mutex_lock(&print_mutex);
            printf("New peer available. Refreshing peer list...\n");
            pthread_mutex_unlock(&print_mutex);
            request_peer_list(routing_server_sock);
        }
        // Handle other server messages if needed
    }
    pthread_exit(NULL);
}

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    int server_available = 1;
    pthread_t listener_thread, server_listener_thread;

    // Initialize mutexes and condition variables
    pthread_mutex_init(&peer_list_mutex, NULL);
    pthread_mutex_init(&print_mutex, NULL);
    pthread_cond_init(&peer_list_updated, NULL);

    // Connect to routing server
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    routing_server_sock = sock; // Set global variable

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(ROUTING_SERVER_PORT);
    if (inet_pton(AF_INET, ROUTING_SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(1);
    }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Could not connect to routing server.\n");
        server_available = 0;
    }

    if (!server_available) {
        // Handle direct connection (not covered here)
        printf("Routing server not available.\n");
        close(sock);
        exit(1);
    }

    // Connection to routing server successful
    // Ask if user wants to become discoverable
    pthread_mutex_lock(&print_mutex);
    printf("Do you want to become discoverable? (yes/no): ");
    pthread_mutex_unlock(&print_mutex);
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline
    if (strcmp(buffer, "yes") == 0) {
        // Ask which port the client would like to open for communication
        pthread_mutex_lock(&print_mutex);
        printf("Enter port to listen for incoming connections: ");
        pthread_mutex_unlock(&print_mutex);
        fgets(buffer, sizeof(buffer), stdin);
        int listen_port = atoi(buffer);

        if (listen_port <= 0 || listen_port > 65535) {
            pthread_mutex_lock(&print_mutex);
            printf("Invalid port number. Please enter a port between 1 and 65535.\n");
            pthread_mutex_unlock(&print_mutex);
            close(sock);
            exit(1);
        }

        // Registration loop
        int registered = 0;
        while (!registered) {
            pthread_mutex_lock(&print_mutex);
            printf("Enter your preferred username: ");
            pthread_mutex_unlock(&print_mutex);
            fgets(username_global, sizeof(username_global), stdin);
            username_global[strcspn(username_global, "\n")] = '\0'; // Remove newline

            if (strlen(username_global) == 0) {
                pthread_mutex_lock(&print_mutex);
                printf("Username cannot be empty. Please try again.\n");
                pthread_mutex_unlock(&print_mutex);
                continue;
            }

            // Register with routing server
            snprintf(buffer, sizeof(buffer), "REGISTER %s %d\n", username_global, listen_port); // Added newline
            ssize_t bytes_sent = write(sock, buffer, strlen(buffer));
            if (bytes_sent <= 0) {
                pthread_mutex_lock(&print_mutex);
                printf("Failed to send REGISTER request to server.\n");
                pthread_mutex_unlock(&print_mutex);
                close(sock);
                exit(1);
            }
            pthread_mutex_lock(&print_mutex);
            printf("Sent REGISTER request to server.\n");
            pthread_mutex_unlock(&print_mutex);

            // Read server response
            bytes_read = read(sock, buffer, sizeof(buffer) - 1);
            if (bytes_read <= 0) {
                pthread_mutex_lock(&print_mutex);
                printf("Failed to receive response from server.\n");
                pthread_mutex_unlock(&print_mutex);
                close(sock);
                exit(1);
            }
            buffer[bytes_read] = '\0';

            pthread_mutex_lock(&print_mutex);
            printf("Received response from server:\n%s\n", buffer);
            pthread_mutex_unlock(&print_mutex);

            if (strcmp(buffer, "USERNAME_TAKEN\n") == 0) { // Adjusted to include newline
                pthread_mutex_lock(&print_mutex);
                printf("Username '%s' is already taken. Please choose a different username.\n", username_global);
                pthread_mutex_unlock(&print_mutex);
            } else if (strncmp(buffer, "REGISTERED", 10) == 0) { // Assuming server sends "REGISTERED" on success
                // Assume registration successful and buffer contains the peer list
                // Extract peer list from the response
                // Example server response: "REGISTERED\npeer1 IP1 PORT1\npeer2 IP2 PORT2\n"
                // We'll skip the first line ("REGISTERED") and store the rest as peer_list

                // Find the first newline
                char *peer_list_start = strchr(buffer, '\n');
                if (peer_list_start != NULL) {
                    peer_list_start++; // Move past the newline
                    pthread_mutex_lock(&peer_list_mutex);
                    strncpy(peer_list, peer_list_start, BUFFER_SIZE - 1);
                    peer_list[BUFFER_SIZE - 1] = '\0';
                    pthread_mutex_unlock(&peer_list_mutex);
                } else {
                    // No peers available
                    pthread_mutex_lock(&peer_list_mutex);
                    peer_list[0] = '\0';
                    pthread_mutex_unlock(&peer_list_mutex);
                }

                registered = 1;
            } else {
                // Handle other server responses if needed
                pthread_mutex_lock(&print_mutex);
                printf("Unexpected response from server. Please try again.\n");
                pthread_mutex_unlock(&print_mutex);
            }
        }

        // Start the listener thread for incoming connections AFTER registration
        int *listen_port_ptr = malloc(sizeof(int));
        if (listen_port_ptr == NULL) {
            perror("malloc");
            close(sock);
            exit(1);
        }
        *listen_port_ptr = listen_port;

        if (pthread_create(&listener_thread, NULL, handle_incoming_connections, listen_port_ptr) != 0) {
            perror("pthread_create");
            free(listen_port_ptr);
            close(sock);
            exit(1);
        }

        // Start server listener thread
        if (pthread_create(&server_listener_thread, NULL, server_listener, NULL) != 0) {
            perror("pthread_create");
            close(sock);
            exit(1);
        }

        // Display the initial peer list
        pthread_mutex_lock(&peer_list_mutex);
        if (strlen(peer_list) > 0) {
            pthread_mutex_lock(&print_mutex);
            printf("Initial list of peers:\n%s\n", peer_list);
            pthread_mutex_unlock(&print_mutex);
        } else {
            pthread_mutex_lock(&print_mutex);
            printf("No available peers at the moment.\n");
            pthread_mutex_unlock(&print_mutex);
        }
        pthread_mutex_unlock(&peer_list_mutex);

        // Main loop to interact with the user
        while (1) {
            if (in_chat) {
                // If in chat, wait until chat is over
                sleep(1);
                continue;
            }

            // Request the latest peer list
            request_peer_list(sock);

            // Display the peer list
            pthread_mutex_lock(&peer_list_mutex);
            display_peer_list(peer_list, username_global);
            pthread_mutex_unlock(&peer_list_mutex);

            // Prompt user to connect to a peer
            pthread_mutex_lock(&print_mutex);
            printf("Enter the username of the peer to connect to (or 'no' to skip): ");
            pthread_mutex_unlock(&print_mutex);
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline
            if (strcmp(buffer, "no") == 0) {
                break;
            }

            char selected_username[50];
            strncpy(selected_username, buffer, sizeof(selected_username) - 1);
            selected_username[sizeof(selected_username) - 1] = '\0'; // Ensure null-termination

            if (strcmp(selected_username, username_global) == 0) {
                pthread_mutex_lock(&print_mutex);
                printf("You cannot connect to yourself. Please select a different peer.\n");
                pthread_mutex_unlock(&print_mutex);
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
                        strncpy(peer_ip, peer_address, sizeof(peer_ip) - 1);
                        peer_ip[sizeof(peer_ip) - 1] = '\0';
                        peer_port = port;
                        peer_found = 1;
                        break;
                    }
                }
                line = strtok(NULL, "\n");
            }

            if (!peer_found) {
                pthread_mutex_lock(&print_mutex);
                printf("Peer '%s' not found. Please try again.\n", selected_username);
                pthread_mutex_unlock(&print_mutex);
                continue;
            } else {
                // Start P2P connection
                int peer_sock = socket(AF_INET, SOCK_STREAM, 0);
                if (peer_sock < 0) {
                    perror("socket");
                    exit(1);
                }
                struct sockaddr_in peer_addr;
                peer_addr.sin_family = AF_INET;
                peer_addr.sin_port = htons(peer_port);
                if (inet_pton(AF_INET, peer_ip, &peer_addr.sin_addr) <= 0) {
                    pthread_mutex_lock(&print_mutex);
                    printf("Invalid IP address for peer '%s'.\n", selected_username);
                    pthread_mutex_unlock(&print_mutex);
                    close(peer_sock);
                    continue;
                }

                if (connect(peer_sock, (struct sockaddr *)&peer_addr, sizeof(peer_addr)) < 0) {
                    pthread_mutex_lock(&print_mutex);
                    printf("Could not connect to peer '%s' at %s:%d.\n", selected_username, peer_ip, peer_port);
                    pthread_mutex_unlock(&print_mutex);
                    // Inform the server that the peer is no longer connected (if needed)
                    snprintf(buffer, sizeof(buffer), "PEER_DISCONNECTED %s\n", selected_username);
                    write(sock, buffer, strlen(buffer));
                    close(peer_sock);
                    continue;
                } else {
                    pthread_mutex_lock(&print_mutex);
                    printf("Connected to peer '%s' at %s:%d.\n", selected_username, peer_ip, peer_port);
                    pthread_mutex_unlock(&print_mutex);

                    // Send connection request with your username
                    snprintf(buffer, sizeof(buffer), "CONNECT_REQUEST %s\n", username_global); // Added newline
                    ssize_t bytes_sent = write(peer_sock, buffer, strlen(buffer));
                    if (bytes_sent <= 0) {
                        pthread_mutex_lock(&print_mutex);
                        printf("Failed to send CONNECT_REQUEST to peer '%s'.\n", selected_username);
                        pthread_mutex_unlock(&print_mutex);
                        close(peer_sock);
                        continue;
                    }

                    // Wait for response
                    bytes_read = read(peer_sock, buffer, sizeof(buffer) - 1);
                    if (bytes_read <= 0) {
                        pthread_mutex_lock(&print_mutex);
                        printf("No response from peer '%s'.\n", selected_username);
                        pthread_mutex_unlock(&print_mutex);
                        close(peer_sock);
                        continue;
                    }
                    buffer[bytes_read] = '\0';

                    if (strcmp(buffer, "ACCEPT\n") == 0) { // Adjusted to include newline
                        // Remove from discoverable list
                        snprintf(buffer, sizeof(buffer), "REMOVE %s\n", username_global); // Added newline
                        write(sock, buffer, strlen(buffer));

                        pthread_mutex_lock(&print_mutex);
                        printf("Connection accepted by '%s'.\n", selected_username);
                        printf("Successfully connected to user '%s'.\n", selected_username);
                        pthread_mutex_unlock(&print_mutex);

                        in_chat = 1; // Set in_chat flag

                        // Start message exchange
                        pthread_t receive_thread;
                        struct chat_info *chat = malloc(sizeof(struct chat_info));

                        if (chat == NULL) {
                            pthread_mutex_lock(&print_mutex);
                            printf("Failed to allocate memory for chat_info.\n");
                            pthread_mutex_unlock(&print_mutex);
                            close(peer_sock);
                            in_chat = 0;
                            continue;
                        }

                        chat->sock = peer_sock;
                        strncpy(chat->peer_username, selected_username, sizeof(chat->peer_username) - 1);
                        chat->peer_username[sizeof(chat->peer_username) - 1] = '\0';
                        strncpy(chat->your_username, username_global, sizeof(chat->your_username) - 1);
                        chat->your_username[sizeof(chat->your_username) - 1] = '\0';

                        if (pthread_create(&receive_thread, NULL, send_messages, (void *)chat) != 0) {
                            perror("pthread_create");
                            pthread_mutex_lock(&print_mutex);
                            printf("Failed to create thread for sending messages.\n");
                            pthread_mutex_unlock(&print_mutex);
                            close(peer_sock);
                            free(chat);
                            in_chat = 0;
                            continue;
                        }

                        pthread_detach(receive_thread); // Detach the thread

                        // Receive messages
                        while ((bytes_read = read(peer_sock, buffer, sizeof(buffer) - 1)) > 0) {
                            buffer[bytes_read] = '\0';
                            pthread_mutex_lock(&print_mutex);
                            printf("[%s]: %s", selected_username, buffer);
                            pthread_mutex_unlock(&print_mutex);
                        }

                        if (bytes_read == 0) {
                            pthread_mutex_lock(&print_mutex);
                            printf("Connection with '%s' closed by peer.\n", selected_username);
                            pthread_mutex_unlock(&print_mutex);
                        } else if (bytes_read < 0) {
                            pthread_mutex_lock(&print_mutex);
                            perror("read");
                            printf("Error reading from peer '%s'.\n", selected_username);
                            pthread_mutex_unlock(&print_mutex);
                        }

                        close(peer_sock);
                        in_chat = 0; // Reset in_chat flag
                    } else if (strcmp(buffer, "DENY\n") == 0) { // Adjusted to include newline
                        pthread_mutex_lock(&print_mutex);
                        printf("Connection request denied by '%s'.\n", selected_username);
                        pthread_mutex_unlock(&print_mutex);
                        close(peer_sock);
                        // Continue and remain discoverable
                    } else {
                        pthread_mutex_lock(&print_mutex);
                        printf("Received unknown response from peer '%s'.\n", selected_username);
                        pthread_mutex_unlock(&print_mutex);
                        close(peer_sock);
                        // Continue and remain discoverable
                    }
                }
            }
        }

        // Send a kill request to the routing server to remove from discoverable list
        snprintf(buffer, sizeof(buffer), "REMOVE %s\n", username_global); // Added newline
        write(sock, buffer, strlen(buffer));

        // Optionally, join threads or clean up resources here
        // pthread_join(listener_thread, NULL);
        // pthread_join(server_listener_thread, NULL);

    } else {
        // Do not become discoverable
        pthread_mutex_lock(&print_mutex);
        printf("You chose not to become discoverable.\n");
        pthread_mutex_unlock(&print_mutex);
    }

    // Close the routing server socket
    close(sock);
    return 0;
}
