CC = gcc
CFLAGS = -Wall -Wextra -Iserver
LDFLAGS = -pthread

SRV_OBJS = server/auth.c server/file_manager.c server/auction_manager.c \
           server/bid_processor.c server/timer.c server/ipc.c \
           server/socket_server.c server/main.c

.PHONY: all clean tests

all: auction_server auction_client

auction_server: $(SRV_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

auction_client: client/client.c
	$(CC) $(CFLAGS) $^ -o $@

tests: test_auth test_file_manager test_auction_manager test_bid_processor test_timer test_ipc

test_auth: server/auth.c server/test_auth.c
	$(CC) $(CFLAGS) $^ -o $@

test_file_manager: server/file_manager.c server/test_file_manager.c
	$(CC) $(CFLAGS) $^ -o $@

test_auction_manager: server/file_manager.c server/ipc.c server/auction_manager.c server/test_auction_manager.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

test_bid_processor: server/auth.c server/file_manager.c server/ipc.c server/auction_manager.c server/bid_processor.c server/test_bid_processor.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

test_timer: server/file_manager.c server/ipc.c server/auction_manager.c server/timer.c server/test_timer.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

test_ipc: server/ipc.c server/test_ipc.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

clean:
	rm -f auction_server auction_client test_auth test_file_manager test_auction_manager test_bid_processor test_timer test_ipc
