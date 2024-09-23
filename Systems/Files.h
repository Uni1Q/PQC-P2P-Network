/**
 * @file files.h
 * @brief Header file for basic file handling operations.
 * 
 * This header provides utility functions for reading from, writing to, and appending
 * data to files, as well as retrieving file sizes. These functions are intended for 
 * simple file management tasks.
 * 
 * @date 02/09/2024
 * @version 1.0
 * @author rokas
 */

#ifndef FILES_H
#define FILES_H

/**
 * @brief Retrieves the size of a file in bytes.
 * 
 * This function opens the file specified by the given path, determines its size in bytes, 
 * and returns the size as an unsigned long.
 * 
 * @param path The path to the file.
 * @return The size of the file in bytes.
 */
unsigned long get_file_size(char *path);

/**
 * @brief Writes data to a file, overwriting its contents.
 * 
 * This function writes the provided data to the file at the specified path. 
 * If the file already exists, its contents will be overwritten.
 * 
 * @param path The path to the file.
 * @param data A pointer to the data to be written.
 * @param size The size of the data to be written in bytes.
 */
void write_file(char *path, void *data, unsigned long size);

/**
 * @brief Appends data to the end of a file.
 * 
 * This function appends the provided data to the end of the file at the specified path. 
 * If the file does not exist, it will be created.
 * 
 * @param path The path to the file.
 * @param data A pointer to the data to be appended.
 * @param size The size of the data to be appended in bytes.
 */
void append_file(char *path, void *data, unsigned long size);

/**
 * @brief Reads the entire content of a file into memory.
 * 
 * This function reads the content of the file at the specified path and returns 
 * a pointer to the data. The data is allocated dynamically, so the caller is responsible 
 * for freeing the memory.
 * 
 * @param path The path to the file.
 * @return A pointer to the file's content, or NULL if the file could not be read.
 */
void *read_file(char *path);

#endif // FILES_H
