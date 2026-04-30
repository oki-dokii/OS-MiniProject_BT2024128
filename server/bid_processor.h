#ifndef BID_PROCESSOR_H
#define BID_PROCESSOR_H

#include "auth.h"
#include <time.h>
#include <stdbool.h>

typedef struct {
    int auction_id;
    char bidder_username[50];
    double amount;
    time_t timestamp;
} Bid;

typedef enum {
    BID_SUCCESS,
    BID_OUTBID,
    BID_CLOSED,
    BID_UNAUTHORIZED,
    BID_SELF_BID,
    BID_ERROR
} BidResult;

/**
 * Places a bid on an auction.
 * Protected by a global mutex to ensure atomic validation and update.
 */
BidResult bid_place(const Session *session, int auction_id, double amount);

/**
 * Retrieves the current highest bid for an auction.
 */
bool bid_get_highest(int auction_id, Bid *highest_bid);

/**
 * Retrieves the full bid history for an auction as a string.
 */
bool bid_get_history(int auction_id, char *buf, size_t bufsize);

#endif // BID_PROCESSOR_H
