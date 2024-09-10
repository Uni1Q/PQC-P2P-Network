//
// Created by rokas on 02/09/2024.
//

#ifndef QUEUE_H
#define QUEUE_H

#include "LinkedList.h"


// Queues are used to access a linked list in a controlled manner
struct Queue
{
    // A reference to the embedded LinkedList
    struct LinkedList list;
    // The push function adds a node to the end of the chain
    void (*push)(struct Queue *queue, void *data, unsigned long size);
    // The peek function retrieves the data from the front of the list
    void * (*peek)(struct Queue *queue);
    // The pop function removes the first item in the list
    void (*pop)(struct Queue *queue);
};


// The constructor should be used to create new Queue instances
struct Queue queue_constructor(void);
// The destructor should be used to delete a Queue instance
void queue_destructor(struct Queue *queue);

#endif //QUEUE_H
