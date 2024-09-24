//
// Created by rokas on 24/09/2024.
//

// chat.h
#ifndef CHAT_H
#define CHAT_H

#define BUFFER_SIZE 1024

struct chat_info {
    int sock;
    char peer_username[50];
    char your_username[50];
};

void *send_messages(void *arg);
void *receive_messages(void *arg);

#endif // CHAT_H
