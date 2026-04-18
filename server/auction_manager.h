#ifndef AUCTION_MANAGER_H
#define AUCTION_MANAGER_H

#include <time.h>
#include <stdbool.h>

typedef enum {
    AUCTION_OPEN,
    AUCTION_CLOSED
} AuctionStatus;

typedef struct {
    int id;
    char item_name[100];
    double start_price;
    double current_price;
    char highest_bidder[50];
    AuctionStatus status;
    int duration_secs;
    time_t start_time;
} Auction;

/**
 * Creates a new auction and saves it to a file.
 * Returns the auction ID on success, -1 on failure.
 */
int auction_create(const char *item_name, double start_price, int duration_secs);

/**
 * Retrieves an auction by ID.
 * Returns true if found and loaded, false otherwise.
 */
bool auction_get(int id, Auction *auction);

/**
 * Lists all auction IDs currently in the system.
 * 'ids' is an array provided by caller, 'max_count' is its size.
 * Returns the number of auctions found.
 */
int auction_list_all(int *ids, int max_count);

/**
 * Marks an auction as CLOSED.
 * Returns true on success.
 */
bool auction_close(int id);

#endif // AUCTION_MANAGER_H
