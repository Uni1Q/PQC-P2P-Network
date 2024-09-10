//
// Created by rokas on 02/09/2024.
//

#ifndef FILES_H
#define FILES_H

unsigned long get_file_size(char *path);
void write_file(char *path, void *data, unsigned long size);
void append_file(char *path, void *data, unsigned long size);
void * read_file(char *path);

#endif //FILES_H
