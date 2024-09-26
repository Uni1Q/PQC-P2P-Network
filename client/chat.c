//
// Created by rokas on 24/09/2024.
//

#include "chat.h"
#include "utils.h"
#include "network.h"
#include "constants.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

// Declare external variables
extern int server_sock;
extern char username_global[USERNAME_MAX_LENGTH];
extern int peer_port;
extern volatile int in_chat;

// Function to send messages to the peer
void *send_messages(void *arg) {
	struct chat_info *chat = (struct chat_info *)arg;
	char message[BUFFER_SIZE]; // Declare 'message'

	while (1) {
		printf("[%s]: ", chat->your_username);
		fflush(stdout);
		safe_fgets(message, sizeof(message), stdin);
		trim_newline(message);

		if (strcmp(message, "/quit") == 0) {
			break;
		}

		strcat(message, "\n");
		if (write(chat->sock, message, strlen(message)) <= 0) {
			printf("Failed to send message. Connection may have been lost.\n");
			break;
		}
	}

	close(chat->sock);
	free(chat);
	in_chat = 0;
	pthread_exit(NULL);
}

// Function to receive messages from the peer
void *receive_messages(void *arg) {
	struct chat_info *chat = (struct chat_info *)arg;
	char buffer[BUFFER_SIZE];
	ssize_t bytes_read;

	while ((bytes_read = read(chat->sock, buffer, sizeof(buffer) - 1)) > 0) {
		buffer[bytes_read] = '\0';
		safe_print("[%s]: %s\n", chat->peer_username, buffer);
	}

	// Connection closed or error
	safe_print("Connection with '%s' closed.\n", chat->peer_username);
	close(chat->sock);
	free(chat);
	in_chat = 0;
	pthread_exit(NULL);
}

