#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include "bid_processor.h"
#include "auction_manager.h"
#include "auth.h"

#define NUM_THREADS 10
#define TARGET_AUCTION 1

typedef struct {
    int thread_id;
    Session session;
} ThreadData;

void* bid_worker(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    
    // All threads try to bid exactly 100.0 on an auction starting at 50.0
    // Only one should succeed; others should get outbid/error
    BidResult res = bid_place(&data->session, TARGET_AUCTION, 100.0);
    
    const char* res_str;
    switch(res) {
        case BID_SUCCESS: res_str = "SUCCESS"; break;
        case BID_OUTBID:  res_str = "OUTBID"; break;
        case BID_CLOSED:  res_str = "CLOSED"; break;
        case BID_UNAUTHORIZED: res_str = "UNAUTHORIZED"; break;
        default: res_str = "ERROR";
    }
    
    printf("Thread %d (%s): %s\n", data->thread_id, data->session.username, res_str);
    
    return NULL;
}

int main() {
    printf("--- BID PROCESSOR CONCURRENCY TEST ---\n\n");

    // 1. Setup environnement
    system("rm -rf data");
    mkdir("data", 0755);
    
    // Create an auction
    int aid = auction_create("Test Item", 50.0, 3600);
    printf("Created auction %d starting at 50.0\n", aid);

    // 2. Prepare threads
    pthread_t threads[NUM_THREADS];
    ThreadData tdata[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        tdata[i].thread_id = i + 1;
        sprintf(tdata[i].session.username, "bidder_%d", i + 1);
        tdata[i].session.role = ROLE_BIDDER;
        tdata[i].session.authenticated = true;
    }

    printf("Spawning %d threads all bidding 100.0 concurrently...\n", NUM_THREADS);

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, bid_worker, &tdata[i]);
    }

    // 3. Join threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // 4. Verify auction state
    Auction a;
    auction_get(TARGET_AUCTION, &a);
    printf("\nFinal Auction State:\n");
    printf("  Price: %.2f\n", a.current_price);
    printf("  Winner: %s\n", a.highest_bidder);

    // 5. Verify bid history
    char history[1024];
    if (bid_get_history(TARGET_AUCTION, history, sizeof(history))) {
        printf("\nBid History:\n%s", history);
    }

    return 0;
}
