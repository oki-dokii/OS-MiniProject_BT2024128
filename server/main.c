#include "socket_server.h"
#include "ipc.h"
#include "timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

void server_cleanup(); /* forward declared in socket_server.h */

void ensure_environment() {
    if (mkdir("data", 0755) == -1 && errno != EEXIST) perror("mkdir data failed");
    if (mkdir("data/auctions", 0755) == -1 && errno != EEXIST) perror("mkdir auctions failed");
    if (mkdir("data/bids", 0755) == -1 && errno != EEXIST) perror("mkdir bids failed");
}

void signal_shutdown_handler(int sig) {
    (void)sig;
    static const char msg[] = "\n[SYSTEM] SIGINT received. Shutting down gracefully...\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    g_shutdown = 1; /* break the accept() loop in server_start() */
}

void system_event_handler(AuctionEvent ev) {
    const char *type = "EVENT";
    if (ev.type == EVENT_AUCTION_CREATED) type = "CREATE";
    if (ev.type == EVENT_BID_PLACED) type = "BID";
    if (ev.type == EVENT_AUCTION_CLOSED) type = "CLOSE";
    
    char msg[256];
    sprintf(msg, "\n[BROADCAST] %s: Auction %d | User: %s | Val: %.2f\n> ", 
           type, ev.auction_id, ev.bidder_username, ev.amount);
    
    printf("%s", msg);
    broadcast_to_clients(msg);
}

int main() {
    printf("==============================================\n");
    printf("   CONCURRENT AUCTION SYSTEM - SERVER v1.0   \n");
    printf("==============================================\n");
    printf("OS Concepts Demonstrated:\n");
    printf("  [1] Role-Based Access Control (auth.c)\n");
    printf("  [2] File Locking via fcntl (file_manager.c)\n");
    printf("  [3] Mutex-Protected Transactions (bid_processor.c)\n");
    printf("  [4] Named Semaphore for Concurrency (socket_server.c)\n");
    printf("  [5] TCP Socket Server (socket_server.c)\n");
    printf("  [6] Named Pipe IPC / FIFO (ipc.c)\n");
    printf("  [7] Signals: SIGALRM + SIGINT (timer.c / main.c)\n");
    printf("==============================================\n\n");

    ensure_environment();

    signal(SIGINT, signal_shutdown_handler); // Register graceful shutdown

    if (!ipc_init()) {
        fprintf(stderr, "[ERROR] Failed to initialize IPC (FIFO). Exiting.\n");
        exit(1);
    }
    ipc_start_listener(system_event_handler);

    timer_init();

    printf("[BOOT] Modules loaded: Auth, FileMgr, AuctionMgr, BidProcessor, Timer, IPC\n");
    printf("[BOOT] Starting TCP server on port 8080...\n\n");

    server_start(8080); /* blocks until g_shutdown is set by SIGINT */

    ipc_cleanup();
    return 0;
}
