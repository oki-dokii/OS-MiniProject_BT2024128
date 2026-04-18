#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H

/**
 * Starts the TCP server on the specified port.
 * This function blocks until the server is shut down.
 */
void server_start(int port);
void broadcast_to_clients(const char *msg);

#endif // SOCKET_SERVER_H
