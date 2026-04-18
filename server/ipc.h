#ifndef IPC_H
#define IPC_H

#include <time.h>
#include <stdbool.h>

typedef enum {
    EVENT_AUCTION_CREATED,
    EVENT_BID_PLACED,
    EVENT_AUCTION_CLOSED
} EventType;

typedef struct {
    EventType type;
    int auction_id;
    char bidder_username[50];
    double amount;
    time_t timestamp;
} AuctionEvent;

typedef void (*EventCallback)(AuctionEvent);

/**
 * Initializes the FIFO for IPC.
 */
bool ipc_init();

/**
 * Sends an event to the IPC channel.
 */
bool ipc_send_event(AuctionEvent event);

/**
 * Starts a background listener thread that processes events.
 */
void ipc_start_listener(EventCallback callback);

/**
 * Cleans up IPC resources.
 */
void ipc_cleanup();

#endif // IPC_H
