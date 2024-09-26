//
// Created by rokas on 24/09/2024.
//

#include "chat.h"
#include "utils.h"
#include "network.h"
#include "pq_encryption.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

// Declare external variables
extern int server_sock;
extern char username_global[50];
extern int peer_port;
extern volatile int in_chat;

void *send_messages(void *arg) {
	struct chat_info *chat = (struct chat_info *)arg;
	char message[BUFFER_SIZE];
	unsigned char ciphertext[BUFFER_SIZE + AES_GCM_TAG_SIZE];
	size_t ciphertext_len;

	while (1) {
		printf("[%s]: ", chat->your_username);
		fflush(stdout);
		safe_fgets(message, sizeof(message), stdin);
		trim_newline(message);

		if (strcmp(message, "/quit") == 0) {
			break;
		}

		size_t plaintext_len = strlen(message);

		// Encrypt the message
		if (pqcrypto_encrypt(&chat->enc_ctx, (unsigned char *)message, plaintext_len,
							 ciphertext, &ciphertext_len) != 0) {
			printf("Encryption failed.\n");
			break;
							 }

		// Send the ciphertext length
		uint32_t net_ciphertext_len = htonl(ciphertext_len);
		if (write(chat->sock, &net_ciphertext_len, sizeof(net_ciphertext_len)) <= 0) {
			printf("Failed to send message length. Connection may have been lost.\n");
			break;
		}

		// Send the ciphertext
		if (write(chat->sock, ciphertext, ciphertext_len) <= 0) {
			printf("Failed to send message. Connection may have been lost.\n");
			break;
		}
	}

	close(chat->sock);
	free(chat);
	in_chat = 0;
	pthread_exit(NULL);
}

void *receive_messages(void *arg) {
	struct chat_info *chat = (struct chat_info *)arg;
	unsigned char buffer[BUFFER_SIZE + AES_GCM_TAG_SIZE];
	unsigned char plaintext[BUFFER_SIZE];
	ssize_t bytes_read;

	while (1) {
		// Read the ciphertext length
		uint32_t net_ciphertext_len;
		bytes_read = read(chat->sock, &net_ciphertext_len, sizeof(net_ciphertext_len));
		if (bytes_read <= 0) {
			break;
		}
		size_t ciphertext_len = ntohl(net_ciphertext_len);

		// Read the ciphertext
		bytes_read = read(chat->sock, buffer, ciphertext_len);
		if (bytes_read <= 0) {
			break;
		}

		// Decrypt the message
		size_t plaintext_len;
		if (pqcrypto_decrypt(&chat->enc_ctx, buffer, ciphertext_len,
							 plaintext, &plaintext_len) != 0) {
			printf("Decryption failed.\n");
			break;
							 }
		plaintext[plaintext_len] = '\0';

		safe_print("[%s]: %s\n", chat->peer_username, plaintext);
	}

	// Connection closed or error
	safe_print("Connection with '%s' closed.\n", chat->peer_username);
	close(chat->sock);
	free(chat);
	in_chat = 0;
	pthread_exit(NULL);
}


