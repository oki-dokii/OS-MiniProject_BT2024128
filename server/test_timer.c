#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include "timer.h"
#include "auction_manager.h"

void broadcast_to_clients(const char *msg) { (void)msg; }

int main() {
    printf("--- TIMER MODULE TEST ---\n\n");

    // Clean start
    system("rm -rf data");
    mkdir("data", 0755);
    
    timer_init();

    printf("Starting 3 auctions with timers:\n");
    printf("- ID 1: 3 seconds\n");
    printf("- ID 2: 5 seconds\n");
    printf("- ID 3: 7 seconds\n\n");

    auction_create("Short Auction", 10.0, 3);
    timer_start(1, 3);
    
    auction_create("Medium Auction", 50.0, 5);
    timer_start(2, 5);
    
    auction_create("Long Auction", 100.0, 7);
    timer_start(3, 7);

    // Monitor for 10 seconds
    for (int i = 0; i < 10; i++) {
        printf("T+%ds | IDs left: ", i);
        bool found = false;
        for (int id = 1; id <= 3; id++) {
            int left = timer_seconds_left(id);
            if (left >= 0) {
                printf("[%d: %ds left] ", id, left);
                found = true;
            }
        }
        if (!found) printf("None");
        printf("\n");
        sleep(1);
    }

    printf("\nTest finished. Verifying final states...\n");
    for (int id = 1; id <= 3; id++) {
        Auction a;
        if (auction_get(id, &a)) {
            printf("Auction %d status: %s\n", id, a.status == AUCTION_CLOSED ? "CLOSED ✓" : "OPEN ✗");
        }
    }

    return 0;
}
