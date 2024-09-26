//
// Created by rokas on 24/09/2024.
//

#include "peer_registry.h"
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

struct Peer peer_list[MAX_PEERS];
int peer_count = 0;
pthread_mutex_t peer_list_mutex = PTHREAD_MUTEX_INITIALIZER;

void add_peer(const char *username, const char *ip, int port) {
	pthread_mutex_lock(&peer_list_mutex);
	if (peer_count < MAX_PEERS) {
		strcpy(peer_list[peer_count].username, username);
		strcpy(peer_list[peer_count].ip, ip);
		peer_list[peer_count].port = port;
		peer_count++;
	}
	pthread_mutex_unlock(&peer_list_mutex);
}

void remove_peer(const char *username) {
	pthread_mutex_lock(&peer_list_mutex);
	for (int i = 0; i < peer_count; i++) {
		if (strcmp(peer_list[i].username, username) == 0) {
			// Remove this peer by shifting the array
			peer_list[i] = peer_list[peer_count - 1];
			peer_count--;
			break;
		}
	}
	pthread_mutex_unlock(&peer_list_mutex);
}

int username_exists(const char *username) {
	pthread_mutex_lock(&peer_list_mutex);
	for (int i = 0; i < peer_count; i++) {
		if (strcmp(peer_list[i].username, username) == 0) {
			pthread_mutex_unlock(&peer_list_mutex);
			return 1; // Username exists
		}
	}
	pthread_mutex_unlock(&peer_list_mutex);
	return 0; // Username does not exist
}

void get_peer_list(char *buffer, size_t size) {
	pthread_mutex_lock(&peer_list_mutex);
	buffer[0] = '\0';
	for (int i = 0; i < peer_count; i++) {
		char line[128];
		snprintf(line, sizeof(line), "%s %s %d\n",
				 peer_list[i].username,
				 peer_list[i].ip,
				 peer_list[i].port);
		strncat(buffer, line, size - strlen(buffer) - 1);
	}
	pthread_mutex_unlock(&peer_list_mutex);
}
