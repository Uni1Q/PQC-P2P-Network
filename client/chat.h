//
// Created by rokas on 24/09/2024.
//

// chat.h
#ifndef CHAT_H
#define CHAT_H

void *handle_incoming_connections(void *arg);
void *handle_peer_connection(void *arg);
void *send_messages(void *arg);

struct chat_info {
    int sock;
    char peer_username[50];
    char your_username[50];
};

#endif // CHAT_H
