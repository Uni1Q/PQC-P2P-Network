//
// Created by rokas on 24/09/2024.
//

#ifndef ROUTING_SERVER_H
#define ROUTING_SERVER_H

void *handle_client(void *arg);
void *command_handler(void *arg);
void broadcast_message(const char *message, int exclude_sock);
void remove_client_socket(int client_sock);

#endif // ROUTING_SERVER_H
