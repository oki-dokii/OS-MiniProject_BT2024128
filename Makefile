CC      = gcc
CFLAGS  = -Wall -Wextra -Iserver -std=c99
LDFLAGS = -pthread

SRV_SRCS = server/auth.c server/file_manager.c server/auction_manager.c \
           server/bid_processor.c server/timer.c server/ipc.c \
           server/socket_server.c server/main.c

.PHONY: all clean tests setup run run-client help

# ──────────────────────────────────────────
#  Primary Targets
# ──────────────────────────────────────────

all: auction_server auction_client
	@echo "✓ Build complete: auction_server  auction_client"

auction_server: $(SRV_SRCS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

auction_client: client/client.c
	$(CC) $(CFLAGS) $^ -o $@

# ──────────────────────────────────────────
#  Environment Setup
# ──────────────────────────────────────────

## setup: Creates required data directories and seeds the user database.
##        Run this ONCE before starting the server for the first time.
setup:
	@echo "Setting up runtime environment..."
	@mkdir -p data/auctions data/bids
	@if [ ! -f data/users.txt ]; then \
		echo "admin:admin123:ADMIN"    >  data/users.txt; \
		echo "alice:alice123:BIDDER"   >> data/users.txt; \
		echo "bob:bob456:BIDDER"       >> data/users.txt; \
		echo "guest1:guest123:VIEWER"  >> data/users.txt; \
		echo "✓ Created data/users.txt with default accounts."; \
	else \
		echo "✓ data/users.txt already exists (skipped)."; \
	fi
	@echo "✓ Setup complete."

# ──────────────────────────────────────────
#  Run Targets (convenience)
# ──────────────────────────────────────────

run: auction_server
	@echo "[LAUNCH] Starting auction server on port 8080..."
	./auction_server

run-client: auction_client
	@echo "[LAUNCH] Connecting client to 127.0.0.1:8080..."
	./auction_client

# ──────────────────────────────────────────
#  Test Suite
# ──────────────────────────────────────────

tests: test_auth test_file_manager test_auction_manager test_bid_processor test_timer test_ipc
	@echo "✓ All test binaries compiled."

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

## test-run: Builds and runs all unit tests sequentially.
test-run: setup tests
	@echo ""
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo "  RUNNING ALL MODULE TESTS"
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo ""
	@echo "[ 1/6 ] Auth Test"
	@echo "────────────────────────────────────────────"
	@./test_auth
	@echo ""
	@echo "[ 2/6 ] File Manager Concurrency Test"
	@echo "────────────────────────────────────────────"
	@./test_file_manager
	@echo ""
	@echo "[ 3/6 ] Auction Manager Test"
	@echo "────────────────────────────────────────────"
	@./test_auction_manager
	@echo ""
	@echo "[ 4/6 ] Bid Processor Concurrency Test"
	@echo "────────────────────────────────────────────"
	@./test_bid_processor
	@echo ""
	@echo "[ 5/6 ] IPC Named-Pipe Test"
	@echo "────────────────────────────────────────────"
	@./test_ipc
	@echo ""
	@echo "[ 6/6 ] Timer + Signal Test (10 seconds)"
	@echo "────────────────────────────────────────────"
	@./test_timer
	@echo ""
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo "  ALL TESTS COMPLETE"
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# ──────────────────────────────────────────
#  Help
# ──────────────────────────────────────────

help:
	@echo "Auction System — Available Make Targets:"
	@echo "  make all         Build server and client"
	@echo "  make setup       Create data dirs and seed users.txt"
	@echo "  make run         Start the server"
	@echo "  make run-client  Launch the interactive client"
	@echo "  make tests       Compile all unit test binaries"
	@echo "  make test-run    Run all unit tests sequentially"
	@echo "  make clean       Remove all compiled binaries"

# ──────────────────────────────────────────
#  Cleanup
# ──────────────────────────────────────────

clean:
	rm -f auction_server auction_client \
	      test_auth test_file_manager test_auction_manager \
	      test_bid_processor test_timer test_ipc
	@echo "✓ Cleaned all binaries."
