#include "file_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

static bool apply_lock(int fd, int type) {
    struct flock fl;
    fl.l_type = type;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0; // Lock entire file

    if (fcntl(fd, F_SETLKW, &fl) == -1) {
        perror("fcntl lock failed");
        return false;
    }
    return true;
}

static bool release_lock(int fd) {
    struct flock fl;
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    if (fcntl(fd, F_SETLK, &fl) == -1) {
        perror("fcntl unlock failed");
        return false;
    }
    return true;
}

bool fm_read_file(const char *path, char *buffer, size_t size) {
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        if (errno != ENOENT) perror("Error opening file for read");
        return false;
    }

    if (!apply_lock(fd, F_RDLCK)) {
        close(fd);
        return false;
    }

    ssize_t bytes_read = read(fd, buffer, size - 1);
    if (bytes_read >= 0) {
        buffer[bytes_read] = '\0';
    }

    release_lock(fd);
    close(fd);
    return bytes_read >= 0;
}

bool fm_write_file(const char *path, const char *content) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Error opening file for write");
        return false;
    }

    if (!apply_lock(fd, F_WRLCK)) {
        close(fd);
        return false;
    }

    ssize_t bytes_written = write(fd, content, strlen(content));
    
    release_lock(fd);
    close(fd);
    return bytes_written != -1;
}

bool fm_append_file(const char *path, const char *content) {
    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("Error opening file for append");
        return false;
    }

    if (!apply_lock(fd, F_WRLCK)) {
        close(fd);
        return false;
    }

    ssize_t bytes_written = write(fd, content, strlen(content));
    
    release_lock(fd);
    close(fd);
    return bytes_written != -1;
}
