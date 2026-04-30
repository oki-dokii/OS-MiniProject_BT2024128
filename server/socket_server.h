#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H

#include <signal.h>

extern volatile sig_atomic_t g_shutdown;

/**
 * Starts the TCP server on the specified port.
 * Exits the accept loop when g_shutdown is set to 1.
 */
void server_start(int port);
void broadcast_to_clients(const char *msg);

#endif // SOCKET_SERVER_H
