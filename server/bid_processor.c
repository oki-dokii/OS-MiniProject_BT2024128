#include "bid_processor.h"
#include "auction_manager.h"
#include "file_manager.h"
#include "ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <errno.h>

#define BIDS_DIR "data/bids"

static pthread_mutex_t bid_mutex = PTHREAD_MUTEX_INITIALIZER;

static void serialize_bid(const Bid *b, char *buf) {
    sprintf(buf, "%ld|%s|%.2f\n", (long)b->timestamp, b->bidder_username, b->amount);
}

BidResult bid_place(const Session *session, int auction_id, double amount) {
    // 1. Authorization check (outside mutex is fine for initial check)
    if (!auth_can_bid(session)) {
        return BID_UNAUTHORIZED;
    }

    pthread_mutex_lock(&bid_mutex);

    // 2. Read current auction state
    Auction a;
    if (!auction_get(auction_id, &a)) {
        pthread_mutex_unlock(&bid_mutex);
        return BID_ERROR;
    }

    // 3. Validate
    if (a.status == AUCTION_CLOSED) {
        pthread_mutex_unlock(&bid_mutex);
        return BID_CLOSED;
    }

    if (amount <= a.current_price) {
        pthread_mutex_unlock(&bid_mutex);
        return BID_OUTBID;
    }

    // 4. Record the bid
    if (mkdir(BIDS_DIR, 0755) == -1 && errno != EEXIST) {
        perror("Failed to create bids directory");
    }
    char bid_path[150];
    sprintf(bid_path, "%s/%d.txt", BIDS_DIR, auction_id);

    Bid b;
    b.auction_id = auction_id;
    strncpy(b.bidder_username, session->username, sizeof(b.bidder_username));
    b.amount = amount;
    b.timestamp = time(NULL);

    char bid_buf[256];
    serialize_bid(&b, bid_buf);
    
    if (!fm_append_file(bid_path, bid_buf)) {
        pthread_mutex_unlock(&bid_mutex);
        return BID_ERROR;
    }

    // 5. Update auction file
    a.current_price = amount;
    strncpy(a.highest_bidder, session->username, sizeof(a.highest_bidder));

    char auc_path[150];
    sprintf(auc_path, "data/auctions/%d.txt", auction_id);
    
    // Serialize updated auction state back to disk
    char auc_buf[1024];
    sprintf(auc_buf, 
        "id:%d\n"
        "item_name:%s\n"
        "start_price:%.2f\n"
        "current_price:%.2f\n"
        "highest_bidder:%s\n"
        "status:%d\n"
        "duration_secs:%d\n"
        "start_time:%ld\n",
        a.id, a.item_name, a.start_price, a.current_price, 
        a.highest_bidder, (int)a.status, a.duration_secs, (long)a.start_time);
    
    if (!fm_write_file(auc_path, auc_buf)) {
        pthread_mutex_unlock(&bid_mutex);
        return BID_ERROR;
    }

    AuctionEvent ev = {0};
    ev.type = EVENT_BID_PLACED;
    ev.auction_id = auction_id;
    strncpy(ev.bidder_username, session->username, sizeof(ev.bidder_username));
    ev.amount = amount;
    ev.timestamp = b.timestamp;
    ipc_send_event(ev);

    pthread_mutex_unlock(&bid_mutex);
    return BID_SUCCESS;
}

bool bid_get_highest(int auction_id, Bid *highest_bid) {
    Auction a;
    if (auction_get(auction_id, &a)) {
        highest_bid->auction_id = auction_id;
        strncpy(highest_bid->bidder_username, a.highest_bidder, sizeof(highest_bid->bidder_username));
        highest_bid->amount = a.current_price;
        highest_bid->timestamp = 0; // Not strictly tracked in the auction main file
        return true;
    }
    return false;
}

bool bid_get_history(int auction_id, char *buf, size_t bufsize) {
    char path[150];
    sprintf(path, "%s/%d.txt", BIDS_DIR, auction_id);
    return fm_read_file(path, buf, bufsize);
}
