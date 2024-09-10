//
// Created by rokas on 02/09/2024.
//

#include "LinkedList.h"

#include <stdlib.h>
#include <stdio.h>


// Private member methods

struct Node * create_node_ll(void *data, unsigned long size);
void destroy_node_ll(struct Node *node_to_destroy);


// public member methods

struct Node * iterate_ll(struct LinkedList *linked_list, int index);
void insert_ll(struct LinkedList *linked_list, int index, void *data, unsigned long size);
void remove_node_ll(struct LinkedList *linked_list, int index);
void * retrieve_ll(struct LinkedList *linked_list, int index);
void bubble_sort_ll(struct LinkedList *linked_list, int (*compare)(void *a, void *b));
short binary_search_ll(struct LinkedList *linked_list, void *query, int (*compare)(void *a, void *b));

// constructor

struct LinkedList linked_list_constructor()
{
    struct LinkedList new_list;
    new_list.head = NULL;
    new_list.length = 0;

    new_list.insert = insert_ll;
    new_list.remove = remove_node_ll;
    new_list.retrieve = retrieve_ll;
    new_list.sort = bubble_sort_ll;
    new_list.search = binary_search_ll;

    return new_list;
}

void linked_list_destructor(struct LinkedList *linked_list)
{
    for (int i = 0; i < linked_list->length; i++)
    {
        linked_list->remove(linked_list, 0);
    }
}


// Private methods

struct Node * create_node_ll(void *data, unsigned long size)
{
    // Allocate space
    struct Node *new_node = (struct Node *)malloc(sizeof(struct Node));
    // Call the constructor
    *new_node = node_constructor(data, size);
    return new_node;
}

// removes node by deallocating its memory address
void destroy_node_ll(struct Node *node_to_destroy)
{
    node_destructor(node_to_destroy);
}

// Iterates through the list
struct Node * iterate_ll(struct LinkedList *linked_list, int index)
{
    // Confirm user has specified a valid index
    if (index < 0 || index >= linked_list->length)
    {
        return NULL;
    }
    // Create cursor node
    struct Node *cursor = linked_list->head;
    // step through the list until the desired index is reached
    for (int i = 0; i < index; i++)
    {
        cursor = cursor->next;
    }
    return cursor;
}


// Public methods

// Puts a new node in the chain
void insert_ll(struct LinkedList *linked_list, int index, void *data, unsigned long size)
{
    // Creates new node to be inserted
    struct Node *node_to_insert = create_node_ll(data, size);
    // Check if its the head of the list
    if (index == 0)
    {
        // Define the Node's next.
        node_to_insert->next = linked_list->head;
        // Redefine the lists head
        linked_list->head = node_to_insert;
    }
    else
    {
        // Find item in list before desired index
        struct Node *cursor = iterate_ll(linked_list, index - 1);
        // Set the Node's next.
        node_to_insert->next = cursor->next;
        // Set the cursor's next to the new node.
        cursor->next = node_to_insert;

    }
    // Increment list length
    linked_list->length += 1;
}

// The remove function removes a node from the linked list.
void remove_node_ll(struct LinkedList *linked_list, int index)
{
    // Check if the item being removed is the head.
    if (index == 0)
    {
        // Collect the node to be removed.
        struct Node *node_to_remove = linked_list->head;
        // Define the new head of the list.
        if (node_to_remove)
        {
            linked_list->head = node_to_remove->next;
            // Remove the desired node
            destroy_node_ll(node_to_remove);
        }
    }
    else
    {
        // Find the item in the list before the one that is going to be removed
        struct Node *cursor = iterate_ll(linked_list, index - 1);
        // Use the cursor to define the node to be removed
        struct Node *node_to_remove = cursor->next;
        // Update the cursor's next to skip the node to be removed
        cursor->next = node_to_remove->next;
        // Remove the node
        destroy_node_ll(node_to_remove);
    }
    // Decrement list length
    linked_list->length -= 1;
}

// The retrieve function is used to access data in the list
void * retrieve_ll(struct LinkedList *linked_list, int index)
{
    // Find the desired node and return its data
    struct Node *cursor = iterate_ll(linked_list, index);
    if (cursor)
    {
        return cursor->data;
    }
    else
    {
        return NULL;
    }
}

// Bubble sort for sorting data in the list
void bubble_sort_ll(struct LinkedList *linked_list, int (*compare)(void *a, void *b))
{
    for (struct Node *i = linked_list->retrieve(linked_list, 0); i; i = i->next)
    {
        for (struct Node *n = i->next; n; n = n->next)
        {
            if (compare(i->data, n->data) > 0)
            {
               // Swap them
                void *temporary = n->data;
                n->data = i->data;
                i->data = temporary;
            }
        }
    }
}

short binary_search_ll(struct LinkedList *linked_list, void *query, int (*compare)(void *a, void *b))
{
    int position = linked_list->length/2;
    int min_checked = 0;
    int max_checked = linked_list->length;
    while (max_checked > min_checked)
    {
        void *data = linked_list->retrieve(linked_list, position);
        if (compare(data, query) == 1)
        {
            max_checked = position;
            if (position != (min_checked + position) / 2)
            {
                position = (min_checked + position) / 2;
            }
            else
            {
                break;
            }
        }
        else if (compare(data, query) == -1)
        {
            min_checked = position;
            if (position != (max_checked + position) / 2)
            {
                position = (max_checked + position) / 2;
            }
            else
            {
                break;
            }
        }
        else
        {
            return 1;
        }
    }
    return 0;
}