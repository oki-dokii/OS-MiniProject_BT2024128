#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "ipc.h"

void handle_event(AuctionEvent ev) {
    const char *type_str = "UNKNOWN";
    if (ev.type == EVENT_AUCTION_CREATED) type_str = "CREATED";
    if (ev.type == EVENT_BID_PLACED) type_str = "BID";
    if (ev.type == EVENT_AUCTION_CLOSED) type_str = "CLOSED";

    printf("[LISTENER] Received event: %s | ID: %d | User: %s | Amount: %.2f\n",
           type_str, ev.auction_id, ev.bidder_username, ev.amount);
}

int main() {
    printf("--- IPC named-pipe (FIFO) TEST ---\n\n");

    if (!ipc_init()) {
        exit(1);
    }

    // Start background listener
    ipc_start_listener(handle_event);
    
    // Fork a writer process
    pid_t pid = fork();
    if (pid == 0) {
        // Child: Writer
        printf("[WRITER] Sending 5 events to FIFO...\n");
        for (int i = 1; i <= 5; i++) {
            AuctionEvent ev;
            ev.type = (i % 3 == 0) ? EVENT_AUCTION_CLOSED : (i % 2 == 0 ? EVENT_BID_PLACED : EVENT_AUCTION_CREATED);
            ev.auction_id = 100 + i;
            sprintf(ev.bidder_username, "user_%d", i);
            ev.amount = 10.5 * i;
            ev.timestamp = time(NULL);
            
            if (ipc_send_event(ev)) {
                printf("[WRITER] Sent event %d\n", i);
            }
            usleep(100000); // Small delay
        }
        printf("[WRITER] Finished sending. Exiting...\n");
        exit(0);
    } else if (pid > 0) {
        // Parent: Listener
        // Wait for child to finish
        waitpid(pid, NULL, 0);
        
        // Give listener a bit more time to process all
        sleep(1);
        printf("\n[MAIN] Closing IPC test.\n");
        ipc_cleanup();
    } else {
        perror("Fork failed");
    }

    return 0;
}
