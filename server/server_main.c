//
// Created by rokas on 24/09/2024.
//

#include "routing_server.h"
#include "peer_registry.h"
#include "network.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define SERVER_PORT 5453

int server_sock;

int main() {
	int *new_sock;
	struct sockaddr_in client_addr;
	pthread_t tid, cmd_tid;
	socklen_t client_len = sizeof(client_addr);

	// Create socket
	server_sock = bind_and_listen(SERVER_PORT);
	if (server_sock < 0) {
		safe_print("Failed to start server.\n");
		exit(1);
	}
	safe_print("Routing server listening on port %d\n", SERVER_PORT);

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
	return 0;
}
