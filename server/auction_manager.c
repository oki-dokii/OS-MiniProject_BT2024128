#include "auction_manager.h"
#include "file_manager.h"
#include "ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

#define AUCTIONS_DIR "data/auctions"
#define NEXT_ID_FILE "data/auctions/next_id.txt"

static int get_next_id() {
    char buf[20];
    int id = 1;
    if (fm_read_file(NEXT_ID_FILE, buf, sizeof(buf))) {
        id = atoi(buf);
    }
    
    char next_buf[20];
    sprintf(next_buf, "%d", id + 1);
    fm_write_file(NEXT_ID_FILE, next_buf);
    
    return id;
}

static void serialize_auction(const Auction *a, char *buf) {
    sprintf(buf, 
        "id:%d\n"
        "item_name:%s\n"
        "start_price:%.2f\n"
        "current_price:%.2f\n"
        "highest_bidder:%s\n"
        "status:%d\n"
        "duration_secs:%d\n"
        "start_time:%ld\n",
        a->id, a->item_name, a->start_price, a->current_price, 
        a->highest_bidder, (int)a->status, a->duration_secs, (long)a->start_time);
}

static void deserialize_auction(const char *buf, Auction *a) {
    sscanf(buf, 
        "id:%d\n"
        "item_name:%[^\n]\n"
        "start_price:%lf\n"
        "current_price:%lf\n"
        "highest_bidder:%[^\n]\n"
        "status:%d\n"
        "duration_secs:%d\n"
        "start_time:%ld\n",
        &a->id, a->item_name, &a->start_price, &a->current_price, 
        a->highest_bidder, (int*)&a->status, &a->duration_secs, (long*)&a->start_time);
}

int auction_create(const char *item_name, double start_price, int duration_secs) {
    if (mkdir("data", 0755) == -1 && errno != EEXIST) {
        perror("Failed to create data directory");
    }
    if (mkdir(AUCTIONS_DIR, 0755) == -1 && errno != EEXIST) {
        perror("Failed to create auctions directory");
    }

    Auction a;
    a.id = get_next_id();
    strncpy(a.item_name, item_name, sizeof(a.item_name));
    a.start_price = start_price;
    a.current_price = start_price;
    strcpy(a.highest_bidder, "None");
    a.status = AUCTION_OPEN;
    a.duration_secs = duration_secs;
    a.start_time = time(NULL);

    char path[150];
    sprintf(path, "%s/%d.txt", AUCTIONS_DIR, a.id);
    
    char buf[1024];
    serialize_auction(&a, buf);
    
    if (fm_write_file(path, buf)) {
        AuctionEvent ev = {0};
        ev.type = EVENT_AUCTION_CREATED;
        ev.auction_id = a.id;
        strncpy(ev.bidder_username, "System", sizeof(ev.bidder_username));
        ev.amount = a.start_price;
        ev.timestamp = a.start_time;
        ipc_send_event(ev);
        return a.id;
    }
    return -1;
}

bool auction_get(int id, Auction *auction) {
    char path[150];
    sprintf(path, "%s/%d.txt", AUCTIONS_DIR, id);
    
    char buf[1024];
    if (fm_read_file(path, buf, sizeof(buf))) {
        deserialize_auction(buf, auction);
        return true;
    }
    return false;
}

int auction_list_all(int *ids, int max_count) {
    DIR *d = opendir(AUCTIONS_DIR);
    if (!d) {
        perror("Failed to open auctions directory for listing");
        return 0;
    }

    struct dirent *dir;
    int count = 0;
    while ((dir = readdir(d)) != NULL && count < max_count) {
        if (strstr(dir->d_name, ".txt") && strcmp(dir->d_name, "next_id.txt") != 0) {
            ids[count++] = atoi(dir->d_name);
        }
    }
    closedir(d);
    return count;
}

int auction_search(const char *keyword, int *ids, int max_count) {
    int all_ids[100];
    int all_count = auction_list_all(all_ids, 100);
    int match_count = 0;

    for (int i = 0; i < all_count && match_count < max_count; i++) {
        Auction a;
        if (auction_get(all_ids[i], &a)) {
            if (strcasestr(a.item_name, keyword)) {
                ids[match_count++] = all_ids[i];
            }
        }
    }
    return match_count;
}

bool auction_close(int id) {
    Auction a;
    if (!auction_get(id, &a)) {
        fprintf(stderr, "Failed to retrieve auction %d for closing\n", id);
        return false;
    }
    
    a.status = AUCTION_CLOSED;
    
    char path[150];
    sprintf(path, "%s/%d.txt", AUCTIONS_DIR, id);
    
    char buf[1024];
    serialize_auction(&a, buf);
    
    bool result = fm_write_file(path, buf);
    if (result) {
        AuctionEvent ev = {0};
        ev.type = EVENT_AUCTION_CLOSED;
        ev.auction_id = id;
        strncpy(ev.bidder_username, a.highest_bidder, sizeof(ev.bidder_username));
        ev.amount = a.current_price;
        ev.timestamp = time(NULL);
        ipc_send_event(ev);
    }
    return result;
}
