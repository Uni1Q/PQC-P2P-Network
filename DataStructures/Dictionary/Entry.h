//
// Created by rokas on 02/09/2024.
//

#ifndef ENTRY_H
#define ENTRY_H

struct Entry
{
    void *key;
    void *value;
};

struct Entry entry_constructor(void *key, unsigned long key_size, void *value, unsigned long value_size);
void entry_destructor(struct Entry *entry);

#endif //ENTRY_H
