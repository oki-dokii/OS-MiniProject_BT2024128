#include "socket_server.h"
#include "ipc.h"
#include "timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>

void ensure_environment() {
    if (mkdir("data", 0755) == -1 && errno != EEXIST) perror("mkdir data failed");
    if (mkdir("data/auctions", 0755) == -1 && errno != EEXIST) perror("mkdir auctions failed");
    if (mkdir("data/bids", 0755) == -1 && errno != EEXIST) perror("mkdir bids failed");
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
    printf("--- AUCTION SYSTEM SERVER INITIALIZING ---\n");
    
    ensure_environment();
    
    if (!ipc_init()) {
        fprintf(stderr, "Failed to initialize IPC\n");
        exit(1);
    }
    ipc_start_listener(system_event_handler);
    
    timer_init();
    
    printf("Modules loaded: Auth, FileMgr, AuctionMgr, BidProcessor, Timer, IPC\n");
    
    server_start(8080);
    
    return 0;
}
