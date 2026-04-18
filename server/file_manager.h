#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <stddef.h>
#include <stdbool.h>

// File access modes
typedef enum {
    FM_READ,
    FM_WRITE, // Overwrite
    FM_APPEND
} FileMode;

/**
 * Safely reads the content of a file using a shared lock (F_RDLCK).
 * Returns true on success, false otherwise.
 */
bool fm_read_file(const char *path, char *buffer, size_t size);

/**
 * Safely writes/overwrites content to a file using an exclusive lock (F_WRLCK).
 * Returns true on success, false otherwise.
 */
bool fm_write_file(const char *path, const char *content);

/**
 * Safely appends content to a file using an exclusive lock (F_WRLCK).
 * Returns true on success, false otherwise.
 */
bool fm_append_file(const char *path, const char *content);

#endif // FILE_MANAGER_H
