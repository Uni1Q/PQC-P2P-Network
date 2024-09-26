//
// Created by rokas on 24/09/2024.
//

#ifndef PEER_REGISTRY_H
#define PEER_REGISTRY_H

#include <pthread.h>
#include <arpa/inet.h>

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

#define MAX_PEERS 100

struct Peer {
	char username[50];
	char ip[INET_ADDRSTRLEN];
	int port;
};

extern struct Peer peer_list[MAX_PEERS];
extern int peer_count;
extern pthread_mutex_t peer_list_mutex;

void add_peer(const char *username, const char *ip, int port);
void remove_peer(const char *username);
int username_exists(const char *username);
void get_peer_list(char *buffer, size_t size);

#endif // PEER_REGISTRY_H
