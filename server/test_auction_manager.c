#include <stdio.h>
#include <stdlib.h>
#include "auction_manager.h"

void print_auction(const Auction *a) {
    printf("Auction [%d]: %s\n", a->id, a->item_name);
    printf("  Price: %.2f | Bidder: %s\n", a->current_price, a->highest_bidder);
    printf("  Status: %s\n", a->status == AUCTION_OPEN ? "OPEN" : "CLOSED");
    printf("\n");
}

int main() {
    printf("--- AUCTION MANAGER TEST ---\n\n");

    // Clean up old tests if any
    system("rm -rf data/auctions/*.txt");

    // 1. Create 3 auctions
    printf("Creating 3 auctions...\n");
    int id1 = auction_create("Vintage Camera", 50.0, 3600);
    int id2 = auction_create("Gaming Laptop", 1200.0, 7200);
    int id3 = auction_create("Rare Stamp", 200.0, 1800);

    if (id1 != -1 && id2 != -1 && id3 != -1) {
        printf("Successfully created auctions with IDs: %d, %d, %d\n\n", id1, id2, id3);
    }

    // 2. List all
    printf("Listing all auctions:\n");
    int ids[10];
    int count = auction_list_all(ids, 10);
    for (int i = 0; i < count; i++) {
        Auction a;
        if (auction_get(ids[i], &a)) {
            print_auction(&a);
        }
    }

    // 3. Read one back specifically
    printf("Reading back auction %d specifically...\n", id2);
    Auction a2;
    if (auction_get(id2, &a2)) {
        print_auction(&a2);
    }

    // 4. Close one
    printf("Closing auction %d...\n", id1);
    if (auction_close(id1)) {
        Auction a1;
        auction_get(id1, &a1);
        print_auction(&a1);
    }

    return 0;
}
