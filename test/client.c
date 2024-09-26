//
// Created by rokas on 23/09/2024.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define ROUTING_SERVER_IP "159.89.248.152" // 127.0.0.1 for local, 159.89.248.152 to connect to hosted ip
#define ROUTING_SERVER_PORT 5453
#define BUFFER_SIZE 1024

int listen_port_global;
int routing_server_sock; // Global variable to access routing server socket
char peer_list[BUFFER_SIZE]; // Shared peer list
pthread_mutex_t peer_list_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t peer_list_updated = PTHREAD_COND_INITIALIZER;
int has_new_peer_list = 0; // Flag to indicate if a new peer list is available

char username_global[50]; // Global variable to store the user's username
int in_chat = 0; // Flag to indicate if the client is in a chat session

// Function prototypes
void *handle_incoming_connections(void *arg);
void *handle_peer_connection(void *arg);
void *send_messages(void *arg);
void *server_listener(void *arg);
void display_peer_list(const char *peer_list_str, const char *own_username);

struct chat_info {
	int sock;
	char peer_username[50];
	char your_username[50];
};

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
				printf("%s\n", peer_name); // Only display the peer's username
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

void *handle_peer_connection(void *arg) {
	int peer_sock = *(int *)arg;
	free(arg);
	char buffer[BUFFER_SIZE];
	char peer_username[50];
	int connection_accepted = 0;

	// Read the connection request message
	ssize_t bytes_read = read(peer_sock, buffer, sizeof(buffer) - 1);
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
		while ((bytes_read = read(peer_sock, buffer, sizeof(buffer) - 1)) > 0) {
			buffer[bytes_read] = '\0';
			pthread_mutex_lock(&print_mutex);
			printf("[%s]: %s", peer_username, buffer);
			pthread_mutex_unlock(&print_mutex);
		}

		pthread_mutex_lock(&print_mutex);
		printf("Connection with '%s' closed.\n", peer_username);
		pthread_mutex_unlock(&print_mutex);
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
		pthread_mutex_lock(&print_mutex);
		printf("[%s]: ", chat->your_username);
		pthread_mutex_unlock(&print_mutex);
		fflush(stdout);
		if (fgets(message, sizeof(message), stdin) == NULL) {
			break;
		}
		if (write(chat->sock, message, strlen(message)) <= 0) {
			pthread_mutex_lock(&print_mutex);
			printf("Failed to send message. Connection may have been lost.\n");
			pthread_mutex_unlock(&print_mutex);
			break;
		}
	}
	free(chat);
	pthread_exit(NULL);
}

void *handle_incoming_connections(void *arg) {
	int listen_port = *(int *)arg;
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
		pthread_detach(peer_thread);
	}
	close(server_sock);
	pthread_exit(NULL);
}

void *server_listener(void *arg) {
	char *username = (char *)arg;
	char buffer[BUFFER_SIZE];
	while (1) {
		ssize_t bytes_read = read(routing_server_sock, buffer, sizeof(buffer) - 1);
		if (bytes_read <= 0) {
			// Connection closed or error
			break;
		}
		buffer[bytes_read] = '\0';

		if (strcmp(buffer, "USERNAME_TAKEN") == 0) {
			// Username is taken
			continue;
		}

		if (strstr(buffer, "New peer connected") != NULL && !in_chat) {
			// Automatically refresh the peer list
			char request[BUFFER_SIZE];
			snprintf(request, sizeof(request), "REGISTER %s %d", username, listen_port_global);
			write(routing_server_sock, request, strlen(request));
			// Receive updated peer list
			bytes_read = read(routing_server_sock, buffer, sizeof(buffer) - 1);
			if (bytes_read > 0) {
				buffer[bytes_read] = '\0';
				pthread_mutex_lock(&peer_list_mutex);
				strcpy(peer_list, buffer);
				has_new_peer_list = 1;
				pthread_cond_signal(&peer_list_updated);
				pthread_mutex_unlock(&peer_list_mutex);
				pthread_mutex_lock(&print_mutex);
				printf("\nPeer list updated. New peers are available.\n");
				pthread_mutex_unlock(&print_mutex);
			}
		}
		// Handle other messages if needed
	}
	pthread_exit(NULL);
}

int main() {
	int sock;
	struct sockaddr_in server_addr;
	char buffer[BUFFER_SIZE];
	ssize_t bytes_read;
	int server_available = 1;
	int listen_port;
	pthread_t listener_thread, server_listener_thread;

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
		printf("Routing server not available.\n");
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
		listen_port = atoi(buffer);
		listen_port_global = listen_port; // Store globally for use in thread

		// Start the listener thread for incoming connections
		if (pthread_create(&listener_thread, NULL, handle_incoming_connections, &listen_port_global) != 0) {
			perror("pthread_create");
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

			// Register with routing server
			snprintf(buffer, sizeof(buffer), "REGISTER %s %d", username_global, listen_port);
			write(sock, buffer, strlen(buffer));

			// Read server response
			bytes_read = read(sock, buffer, sizeof(buffer) - 1);
			if (bytes_read <= 0) {
				printf("Failed to receive response from server.\n");
				exit(1);
			}
			buffer[bytes_read] = '\0';

			if (strcmp(buffer, "USERNAME_TAKEN") == 0) {
				pthread_mutex_lock(&print_mutex);
				printf("Username '%s' is already taken. Please choose a different username.\n", username_global);
				pthread_mutex_unlock(&print_mutex);
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
		pthread_mutex_lock(&peer_list_mutex);
		strcpy(peer_list, buffer);
		pthread_mutex_unlock(&peer_list_mutex);

		while (1) {
			if (in_chat) {
				// If in chat, wait until chat is over
				sleep(1);
				continue;
			}

			pthread_mutex_lock(&peer_list_mutex);
			display_peer_list(peer_list, username_global);
			pthread_mutex_unlock(&peer_list_mutex);

			pthread_mutex_lock(&print_mutex);
			printf("Enter the username of the peer to connect to (or 'no' to skip): ");
			pthread_mutex_unlock(&print_mutex);
			fgets(buffer, sizeof(buffer), stdin);
			buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline
			if (strcmp(buffer, "no") == 0) {
				break;
			}

			char selected_username[50];
			strcpy(selected_username, buffer);

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
						strcpy(peer_ip, peer_address);
						peer_port = port;
						peer_found = 1;
						break;
					}
				}
				line = strtok(NULL, "\n");
			}

			if (!peer_found) {
				pthread_mutex_lock(&print_mutex);
				printf("Peer not found. Please try again.\n");
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
				inet_pton(AF_INET, peer_ip, &peer_addr.sin_addr);

				if (connect(peer_sock, (struct sockaddr *)&peer_addr, sizeof(peer_addr)) < 0) {
					pthread_mutex_lock(&print_mutex);
					printf("Could not connect to peer. The peer may no longer be connected.\n");
					pthread_mutex_unlock(&print_mutex);
					// Inform the server that the peer is no longer connected
					snprintf(buffer, sizeof(buffer), "PEER_DISCONNECTED %s", selected_username);
					write(sock, buffer, strlen(buffer));

					// Receive updated peer list
					bytes_read = read(sock, buffer, sizeof(buffer) - 1);
					if (bytes_read > 0) {
						buffer[bytes_read] = '\0';
						pthread_mutex_lock(&peer_list_mutex);
						strcpy(peer_list, buffer);
						has_new_peer_list = 1;
						pthread_mutex_unlock(&peer_list_mutex);
						pthread_mutex_lock(&print_mutex);
						printf("Updated list of discoverable peers:\n");
						display_peer_list(peer_list, username_global);
						pthread_mutex_unlock(&print_mutex);
					}
					close(peer_sock);
					continue;
				} else {
					pthread_mutex_lock(&print_mutex);
					printf("Connected to peer at %s:%d\n", peer_ip, peer_port);
					pthread_mutex_unlock(&print_mutex);

					// Send connection request with your username
					snprintf(buffer, sizeof(buffer), "CONNECT_REQUEST %s", username_global);
					write(peer_sock, buffer, strlen(buffer));

					// Wait for response
					bytes_read = read(peer_sock, buffer, sizeof(buffer) - 1);
					if (bytes_read <= 0) {
						pthread_mutex_lock(&print_mutex);
						printf("No response from peer.\n");
						pthread_mutex_unlock(&print_mutex);
						close(peer_sock);
						continue;
					}
					buffer[bytes_read] = '\0';

					if (strcmp(buffer, "ACCEPT") == 0) {
						// Remove from discoverable list
						snprintf(buffer, sizeof(buffer), "REMOVE %s", username_global);
						write(sock, buffer, strlen(buffer));

						pthread_mutex_lock(&print_mutex);
						printf("Connection accepted by '%s'.\n", selected_username);
						pthread_mutex_unlock(&print_mutex);

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
							pthread_mutex_lock(&print_mutex);
							printf("[%s]: %s", selected_username, buffer);
							pthread_mutex_unlock(&print_mutex);
						}

						pthread_mutex_lock(&print_mutex);
						printf("Connection with '%s' closed.\n", selected_username);
						pthread_mutex_unlock(&print_mutex);
						close(peer_sock);
						in_chat = 0; // Reset in_chat flag
					} else if (strcmp(buffer, "DENY") == 0) {
						pthread_mutex_lock(&print_mutex);
						printf("Connection request denied by '%s'.\n", selected_username);
						pthread_mutex_unlock(&print_mutex);
						close(peer_sock);
						// Continue and remain discoverable
					} else {
						pthread_mutex_lock(&print_mutex);
						printf("Received unknown response from peer.\n");
						pthread_mutex_unlock(&print_mutex);
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
		pthread_mutex_lock(&print_mutex);
		printf("You chose not to become discoverable.\n");
		pthread_mutex_unlock(&print_mutex);
	}

	// Close the routing server socket
	close(sock);
	return 0;
}
