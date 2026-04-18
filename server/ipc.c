#include "ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

#define FIFO_PATH "data/auction_events.fifo"

static EventCallback listener_callback = NULL;
static pthread_t listener_thread;
static bool running = true;

bool ipc_init() {
    mkdir("data", 0755);
    if (mkfifo(FIFO_PATH, 0666) == -1) {
        if (errno != EEXIST) {
            perror("mkfifo failed");
            return false;
        }
    }
    return true;
}

bool ipc_send_event(AuctionEvent event) {
    // Open for write only; will block until a reader exists
    // To prevent blocking if no listener is active, we could use O_NONBLOCK
    // but the prompt implies a standard IPC flow.
    int fd = open(FIFO_PATH, O_WRONLY);
    if (fd == -1) {
        // If nobody is reading, we skip sending to avoid blocking forever
        if (errno == ENXIO) return false; 
        return false;
    }

    // Binary write of the struct is atomic if size <= PIPE_BUF
    ssize_t written = write(fd, &event, sizeof(AuctionEvent));
    close(fd);
    return written == sizeof(AuctionEvent);
}

void *ipc_listener_worker(void *arg) {
    (void)arg;
    while (running) {
        // Opening for O_RDONLY will block until a writer opens it
        int fd = open(FIFO_PATH, O_RDONLY);
        if (fd == -1) {
            usleep(100000);
            continue;
        }

        AuctionEvent event;
        while (read(fd, &event, sizeof(AuctionEvent)) > 0) {
            if (listener_callback) {
                listener_callback(event);
            }
        }
        close(fd);
    }
    return NULL;
}

void ipc_start_listener(EventCallback callback) {
    listener_callback = callback;
    pthread_create(&listener_thread, NULL, ipc_listener_worker, NULL);
}

void ipc_cleanup() {
    running = false;
    pthread_cancel(listener_thread);
    pthread_join(listener_thread, NULL);
    unlink(FIFO_PATH);
}
