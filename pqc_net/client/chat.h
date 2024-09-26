//
// Created by rokas on 24/09/2024.
//

// client/chat.h

#ifndef CHAT_H
#define CHAT_H

#include "constants.h"
#include "pq_encryption.h"  // Ensure this path is correct

struct chat_info {
    int sock;
    encryption_context_t enc_ctx; // Post-quantum encryption context
    char peer_username[USERNAME_MAX_LENGTH];
    char your_username[USERNAME_MAX_LENGTH];
};

void *send_messages(void *arg);
void *receive_messages(void *arg);

#endif // CHAT_H

