#include "socket_server.h"
#include "auth.h"
#include "auction_manager.h"
#include "bid_processor.h"
#include "timer.h"
#include "ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>

#define BUFFER_SIZE 2048
#define MAX_CONCURRENT_CLIENTS 10
#define MAX_CLIENTS 100

static sem_t connection_limit;
static int client_sockets[MAX_CLIENTS];
static pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void broadcast_to_clients(const char *msg) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != -1) {
            send(client_sockets[i], msg, strlen(msg), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

static void register_client(int sock) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == -1) {
            client_sockets[i] = sock;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

static void unregister_client(int sock) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == sock) {
            client_sockets[i] = -1;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *client_handler(void *socket_desc) {
    int sock = *(int *)socket_desc;
    free(socket_desc);
    
    register_client(sock);
    
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    Session session = {0};
    session.authenticated = false;

    printf("[SERVER] New client connected on socket %d\n", sock);

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int read_size = recv(sock, buffer, BUFFER_SIZE, 0);
        if (read_size <= 0) break;

        // Strip trailing newline/cr
        buffer[strcspn(buffer, "\r\n")] = 0;

        char cmd[20], arg1[100], arg2[100], arg3[100];
        sscanf(buffer, "%s %s %s %s", cmd, arg1, arg2, arg3);

        if (strcmp(cmd, "LOGIN") == 0) {
            if (auth_login(arg1, arg2, &session)) {
                sprintf(response, "SUCCESS: Logged in as %s (%d)\n", session.username, session.role);
            } else {
                sprintf(response, "ERROR: Invalid credentials\n");
            }
        } 
        else if (strcmp(cmd, "LIST") == 0) {
            int ids[50];
            int count = auction_list_all(ids, 50);
            sprintf(response, "Auctions (%d found):\n", count);
            for (int i = 0; i < count; i++) {
                Auction a;
                if (auction_get(ids[i], &a)) {
                    char line[200];
                    sprintf(line, " - [%d] %s | Price: %.2f | Status: %s\n", 
                            a.id, a.item_name, a.current_price, a.status == AUCTION_OPEN ? "OPEN" : "CLOSED");
                    strcat(response, line);
                }
            }
        }
        else if (strcmp(cmd, "SEARCH") == 0) {
            int ids[50];
            int count = auction_search(arg1, ids, 50);
            sprintf(response, "Search results for '%s' (%d found):\n", arg1, count);
            for (int i = 0; i < count; i++) {
                Auction a;
                if (auction_get(ids[i], &a)) {
                    char line[200];
                    sprintf(line, " - [%d] %s | Price: %.2f | Status: %s\n", 
                            a.id, a.item_name, a.current_price, a.status == AUCTION_OPEN ? "OPEN" : "CLOSED");
                    strcat(response, line);
                }
            }
        }
        else if (strcmp(cmd, "CREATE") == 0) {
            if (!auth_can_create(&session)) {
                sprintf(response, "ERROR: Unauthorized (Admin only)\n");
            } else {
                int aid = auction_create(arg1, atof(arg2), atoi(arg3));
                if (aid != -1) {
                    timer_start(aid, atoi(arg3));
                    sprintf(response, "SUCCESS: Created auction %d\n", aid);
                } else {
                    sprintf(response, "ERROR: Failed to create auction\n");
                }
            }
        }
        else if (strcmp(cmd, "BID") == 0) {
            BidResult res = bid_place(&session, atoi(arg1), atof(arg2));
            switch (res) {
                case BID_SUCCESS: sprintf(response, "SUCCESS: Bid placed\n"); break;
                case BID_OUTBID:  sprintf(response, "ERROR: Outbid\n"); break;
                case BID_CLOSED:  sprintf(response, "ERROR: Auction closed\n"); break;
                case BID_UNAUTHORIZED: sprintf(response, "ERROR: Unauthorized\n"); break;
                default: sprintf(response, "ERROR: Unknown error\n");
            }
        }
        else if (strcmp(cmd, "HISTORY") == 0) {
            char history[BUFFER_SIZE - 200];
            if (bid_get_history(atoi(arg1), history, sizeof(history))) {
                sprintf(response, "History for Auction %s:\n%s", arg1, history);
            } else {
                sprintf(response, "ERROR: No history found\n");
            }
        }
        else if (strcmp(cmd, "CLOSE") == 0) {
             if (!auth_can_close(&session)) {
                sprintf(response, "ERROR: Unauthorized (Admin only)\n");
            } else {
                if (auction_close(atoi(arg1))) {
                    timer_cancel(atoi(arg1));
                    sprintf(response, "SUCCESS: Auction %s closed\n", arg1);
                } else {
                    sprintf(response, "ERROR: Failed to close\n");
                }
            }
        }
        else if (strcmp(cmd, "QUIT") == 0) {
            sprintf(response, "Goodbye!\n");
            send(sock, response, strlen(response), 0);
            break;
        }
        else {
            sprintf(response, "ERROR: Unknown command\n");
        }

        send(sock, response, strlen(response), 0);
    }

    printf("[SERVER] Client on socket %d disconnected\n", sock);
    unregister_client(sock);
    close(sock);
    sem_post(&connection_limit); // Release semaphore slot
    return NULL;
}

void server_start(int port) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    // Initialize semaphore for connection limiting (Counting Semaphore)
    sem_init(&connection_limit, 0, MAX_CONCURRENT_CLIENTS);

    for (int i = 0; i < MAX_CLIENTS; i++) client_sockets[i] = -1;

    printf("[SERVER] Listening on port %d (Max clients: %d, Semaphore slots: %d)\n", 
           port, MAX_CONCURRENT_CLIENTS, MAX_CONCURRENT_CLIENTS);

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }

        // Wait for a slot to become available (Semaphore)
        sem_wait(&connection_limit);

        int *new_sock = malloc(sizeof(int));
        *new_sock = new_socket;
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, client_handler, (void *)new_sock) < 0) {
            perror("Could not create thread");
            free(new_sock);
            close(new_socket);
        }
        pthread_detach(thread_id); // Ensure thread resources are reclaimed on exit
    }
}
