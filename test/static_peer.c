//
// Created by rokas on 24/09/2024.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "../common/constants.h"
#include "../common/network.h"
#include "../common/utils.h"

#define ROUTING_SERVER_IP "159.89.248.152" // 127.0.0.1 for local testing, 159.89.248.152 to connect to droplet
#define ROUTING_SERVER_PORT 5453
#define STATIC_PEER_USERNAME "test"
#define STATIC_PEER_PORT 6000
#define BUFFER_SIZE 1024

void *handle_p2p_connection(void *arg) {
	int peer_sock = *(int *)arg;
	free(arg);
	char buffer[BUFFER_SIZE];
	ssize_t bytes_read;

	while ((bytes_read = read(peer_sock, buffer, sizeof(buffer) - 1)) > 0) {
		buffer[bytes_read] = '\0';
		// For testing, echo the message back
		write(peer_sock, buffer, bytes_read);
	}

	close(peer_sock);
	pthread_exit(NULL);
}

int main() {
	// Connect to routing server
	int server_sock = connect_to_server(ROUTING_SERVER_IP, ROUTING_SERVER_PORT);
	if (server_sock < 0) {
		fprintf(stderr, "Failed to connect to routing server.\n");
		exit(EXIT_FAILURE);
	}

	// Send REGISTER command
	char register_command[BUFFER_SIZE];
	snprintf(register_command, sizeof(register_command), "REGISTER %s %d\n", STATIC_PEER_USERNAME, STATIC_PEER_PORT);
	if (write(server_sock, register_command, strlen(register_command)) <= 0) {
		perror("write");
		close(server_sock);
		exit(EXIT_FAILURE);
	}

	// Read server response
	char response[BUFFER_SIZE];
	ssize_t bytes_read = read(server_sock, response, sizeof(response) - 1);
	if (bytes_read <= 0) {
		perror("read");
		close(server_sock);
		exit(EXIT_FAILURE);
	}
	response[bytes_read] = '\0';

	if (strncmp(response, "USERNAME_TAKEN", 14) == 0) {
		fprintf(stderr, "Username '%s' is already taken.\n", STATIC_PEER_USERNAME);
		close(server_sock);
		exit(EXIT_FAILURE);
	} else if (strncmp(response, "INVALID_COMMAND", 15) == 0) {
		fprintf(stderr, "Invalid REGISTER command format.\n");
		close(server_sock);
		exit(EXIT_FAILURE);
	}

	// Registration successful, proceed to listen for P2P connections
	printf("Static peer '%s' registered with routing server.\n", STATIC_PEER_USERNAME);

	// Start P2P listener
	int listen_sock = bind_and_listen(STATIC_PEER_PORT);
	if (listen_sock < 0) {
		fprintf(stderr, "Failed to bind and listen on port %d.\n", STATIC_PEER_PORT);
		close(server_sock);
		exit(EXIT_FAILURE);
	}
	printf("Static peer listening for P2P connections on port %d...\n", STATIC_PEER_PORT);

	while (1) {
		struct sockaddr_in peer_addr;
		socklen_t addr_size = sizeof(peer_addr);
		int peer_sock = accept(listen_sock, (struct sockaddr *)&peer_addr, &addr_size);
		if (peer_sock < 0) {
			perror("accept");
			continue;
		}

		// Handle P2P connection in a new thread
		pthread_t thread_id;
		int *new_sock = malloc(sizeof(int));
		if (new_sock == NULL) {
			perror("malloc");
			close(peer_sock);
			continue;
		}
		*new_sock = peer_sock;
		if (pthread_create(&thread_id, NULL, handle_p2p_connection, (void *)new_sock) != 0) {
			perror("pthread_create");
			close(peer_sock);
			free(new_sock);
			continue;
		}
		pthread_detach(thread_id);
	}

	close(listen_sock);
	close(server_sock);
	return 0;
}