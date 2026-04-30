# EGC 301P — Operating Systems Lab
# Mini Project Report

## Project Title: Concurrent Auction System

---

## 1. Problem Statement

Design and implement a multi-user, real-time auction system using C that demonstrates core Operating Systems concepts. The system must support concurrent clients who can create auctions, place bids, and view results through a client-server architecture. The implementation must exhibit role-based access control, file locking for data safety, concurrency synchronization, inter-process communication, and TCP socket-based networking.

---

## 2. System Overview

The Concurrent Auction System is a TCP-based client-server application where:
- An **Auction Server** handles multiple simultaneous client connections
- Each client runs an interactive **CLI** to issue commands
- Auctions are **timed** and auto-close when their duration expires
- All events (bids, creations, closures) are **broadcast in real-time** to all connected clients

### Default User Accounts

| Username | Password  | Role   |
|----------|-----------|--------|
| admin    | admin123  | ADMIN  |
| alice    | alice123  | BIDDER |
| bob      | bob456    | BIDDER |
| guest1   | guest123  | VIEWER |

---

## 3. Implementation of Required OS Concepts

---

### 3.1 Role-Based Authorization

**Files:** `server/auth.h`, `server/auth.c`

Three roles are defined using a C `enum`:

```c
typedef enum { ROLE_ADMIN, ROLE_BIDDER, ROLE_VIEWER, ROLE_NONE } Role;
```

A `Session` struct tracks the authenticated user:

```c
typedef struct {
    char username[50];
    Role role;
    bool authenticated;
} Session;
```

Three permission gates are called before any sensitive operation:

```c
bool auth_can_bid(const Session *session);    // ADMIN + BIDDER
bool auth_can_create(const Session *session); // ADMIN only
bool auth_can_close(const Session *session);  // ADMIN only
```

**How it works:**  
On login, `auth_login()` reads `data/users.txt` (format: `username:password:ROLE`), matches credentials, and populates the session. Every command handler in `socket_server.c` calls the appropriate gate before execution. A VIEWER attempting to bid receives `ERROR: Unauthorized`.

---

### 3.2 File Locking

**File:** `server/file_manager.c`

All file I/O is routed through a single gateway module using POSIX `fcntl()` advisory locks:

```c
// Shared (read) lock — multiple concurrent readers allowed
struct flock fl = { .l_type = F_RDLCK, .l_whence = SEEK_SET,
                    .l_start = 0, .l_len = 0 };
fcntl(fd, F_SETLKW, &fl);   // F_SETLKW = blocking wait

// Exclusive (write) lock — blocks all other readers/writers
fl.l_type = F_WRLCK;
fcntl(fd, F_SETLKW, &fl);
```

Three public functions form the API:
- `fm_read_file()` — acquires `F_RDLCK`, reads, releases
- `fm_write_file()` — acquires `F_WRLCK`, overwrites, releases
- `fm_append_file()` — acquires `F_WRLCK`, appends, releases

**Verification:** The file manager concurrency test spawns 5 forked processes simultaneously, each appending 100 lines. Final line count is always exactly 500 — proving zero data corruption under concurrent access.

---

### 3.3 Concurrency Control

**Files:** `server/bid_processor.c`, `server/socket_server.c`

#### Mutex (pthread_mutex_t)

The `bid_place()` function uses a global mutex to make the entire bid operation atomic:

```c
static pthread_mutex_t bid_mutex = PTHREAD_MUTEX_INITIALIZER;

BidResult bid_place(const Session *session, int auction_id, double amount) {
    if (!auth_can_bid(session)) return BID_UNAUTHORIZED;
    
    pthread_mutex_lock(&bid_mutex);
    // Critical Section:
    // 1. Read current auction state from disk
    // 2. Validate: amount must be > current_price
    // 3. Append bid to bid log
    // 4. Write updated auction state back to disk
    // 5. Fire IPC event
    pthread_mutex_unlock(&bid_mutex);
    
    return BID_SUCCESS;
}
```

This prevents the **race condition** where two threads both read the same `current_price` and both decide they win.

#### Named Semaphore (sem_open)

The server uses a counting semaphore to limit concurrent connections:

```c
connection_limit = sem_open("/auction_conn_limit", O_CREAT|O_EXCL, 0644, 10);

// Before spawning each client thread:
sem_wait(connection_limit);   // decrements; blocks if count == 0
pthread_create(&thread_id, NULL, client_handler, new_sock);

// When client disconnects:
sem_post(connection_limit);   // increments; unblocks next waiting client
```

A named semaphore (`sem_open`) is used instead of the deprecated `sem_init` for full POSIX portability across Linux and macOS.

---

### 3.4 Data Consistency

**Files:** `server/file_manager.c`, `server/bid_processor.c`

The system uses **two complementary layers** of consistency protection:

| Layer | Mechanism | Protects Against |
|-------|-----------|-----------------|
| Application | `pthread_mutex_t` | Race conditions in bid validation logic |
| Filesystem | `fcntl F_WRLCK` | Concurrent file writes from multiple threads/processes |

**Problems prevented:**
- **Race condition:** Two bidders simultaneously reading `current_price = 1000` and both placing `1001` — only one wins due to mutex serialization.
- **Lost update:** Two threads simultaneously writing to the same auction file — fcntl exclusive lock ensures writes are serialized.
- **Dirty read:** A thread reading a half-written file — the shared read lock blocks until the writer releases its exclusive lock.

---

### 3.5 Socket Programming

**Files:** `server/socket_server.c`, `client/client.c`

Full TCP client-server implementation using the Berkeley Sockets API:

**Server lifecycle:**
```c
// 1. Create socket
server_fd = socket(AF_INET, SOCK_STREAM, 0);
setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

// 2. Bind to port
address.sin_family = AF_INET;
address.sin_addr.s_addr = INADDR_ANY;
address.sin_port = htons(8080);
bind(server_fd, (struct sockaddr *)&address, sizeof(address));

// 3. Listen and accept loop
listen(server_fd, 3);
while (1) {
    new_socket = accept(server_fd, &address, &addrlen);
    sem_wait(connection_limit);
    pthread_create(&thread_id, NULL, client_handler, &new_socket);
    pthread_detach(thread_id);
}
```

**Thread-per-client model:**  
Each accepted connection gets its own `pthread`. The thread maintains a private `Session` struct, enabling stateful authentication. All threads access shared data (auction files) through the mutex-protected bid processor and fcntl-locked file manager.

**Live broadcast:**  
All active client sockets are tracked in a global array protected by `clients_mutex`. When an IPC event fires (bid placed, auction closed), the server calls `broadcast_to_clients()` which sends the notification to every connected socket simultaneously.

---

### 3.6 Inter-Process Communication

**File:** `server/ipc.c` — Named Pipe (FIFO)  
**File:** `server/timer.c` + `server/main.c` — Signals

#### Named Pipe (FIFO)

A POSIX named pipe at `data/auction_events.fifo` serves as an event bus between subsystems:

```c
// Creation
mkfifo("data/auction_events.fifo", 0666);

// Writer (any module, e.g. bid_processor.c):
AuctionEvent ev = { .type = EVENT_BID_PLACED, .auction_id = id,
                    .amount = amount, ... };
int fd = open(FIFO_PATH, O_WRONLY);
write(fd, &ev, sizeof(AuctionEvent));   // atomic for small structs

// Reader (background listener thread in ipc.c):
void *ipc_listener_worker(void *arg) {
    while (running) {
        int fd = open(FIFO_PATH, O_RDONLY);
        AuctionEvent event;
        while (read(fd, &event, sizeof(AuctionEvent)) > 0)
            listener_callback(event);   // triggers broadcast_to_clients()
        close(fd);
    }
}
```

The IPC test verifies cross-process event delivery by forking a writer child and reading from the listener thread in the parent — confirming the FIFO works across process boundaries.

#### Signals

Two signals are used:

| Signal | Handler | Purpose |
|--------|---------|---------|
| `SIGALRM` | `handle_alarm()` in `timer.c` | Raises every 1 second from the ticker thread to drive auction expiry checks |
| `SIGINT` | `signal_shutdown_handler()` in `main.c` | Ctrl+C triggers graceful shutdown: broadcasts "Server shutting down" to all clients, cleans up FIFO and semaphore, then exits |

```c
// timer.c — SIGALRM registration + heartbeat
signal(SIGALRM, handle_alarm);
void *ticker_func(void *arg) {
    while (running) {
        sleep(1);
        alarm(1);  // schedule SIGALRM
        // check all timers for expiry...
    }
}

// main.c — SIGINT graceful shutdown
signal(SIGINT, signal_shutdown_handler);
void signal_shutdown_handler(int sig) {
    broadcast_to_clients("\n[SERVER] Server shutting down. Goodbye!\n");
    ipc_cleanup();
    exit(0);
}
```

---

## 4. Screenshots / Output

### Server Startup
```
==============================================
   CONCURRENT AUCTION SYSTEM - SERVER v1.0
==============================================
OS Concepts Demonstrated:
  [1] Role-Based Access Control (auth.c)
  [2] File Locking via fcntl (file_manager.c)
  [3] Mutex-Protected Transactions (bid_processor.c)
  [4] Named Semaphore for Concurrency (socket_server.c)
  [5] TCP Socket Server (socket_server.c)
  [6] Named Pipe IPC / FIFO (ipc.c)
  [7] Signals: SIGALRM + SIGINT (timer.c / main.c)
==============================================

[BOOT] Modules loaded: Auth, FileMgr, AuctionMgr, BidProcessor, Timer, IPC
[BOOT] Starting TCP server on port 8080...

[SERVER] Listening on port 8080 (Max clients: 10, Semaphore slots: 10)
[SERVER] New client connected on socket 4
[SERVER] New client connected on socket 5

[BROADCAST] CREATE: Auction 1 | User: System     | Val: 80000.00
[BROADCAST] BID:    Auction 1 | User: alice       | Val: 85000.00
[SIGNAL] Heartbeat (SIGALRM)
[TIMER] Auction 1 expired! Closing...
[BROADCAST] CLOSE:  Auction 1 | User: alice       | Val: 85000.00
```

### Client Session (Admin)
```
╔══════════════════════════════════════════╗
║       CONCURRENT AUCTION SYSTEM          ║
║       Interactive Client v1.0            ║
╚══════════════════════════════════════════╝
  Connecting to 127.0.0.1:8080 ...
  Connected successfully!

auction> LOGIN admin admin123
SUCCESS: Logged in as admin [Role: ADMIN]

auction> CREATE MacBook 80000 120
SUCCESS: Created auction 1

auction> LIST
Auctions (1 found):
 - [1] MacBook | Price: 80000.00 | Status: OPEN

auction> CLOSE 1
SUCCESS: Auction 1 closed
```

### Client Session (Bidder)
```
auction> LOGIN alice alice123
SUCCESS: Logged in as alice [Role: BIDDER]

auction> BID 1 85000
SUCCESS: Bid placed

auction> HISTORY 1
History for Auction 1:
<timestamp>|alice|85000.00

auction> CREATE Laptop 1000 60
ERROR: Unauthorized (Admin only)
```

---

## 5. Challenges Faced and Solutions

| Challenge | Solution |
|-----------|----------|
| `sem_init` deprecated on macOS | Switched to POSIX named semaphores using `sem_open`/`sem_close`/`sem_unlink`. Named semaphores are fully supported on both Linux and macOS. |
| Race condition in bid validation | Wrapped the entire read-validate-write sequence in a `pthread_mutex_lock` / `pthread_mutex_unlock` critical section. |
| FIFO blocking on `open()` | `open(FIFO_PATH, O_WRONLY)` blocks until a reader exists. Used `ENXIO` errno check to detect and gracefully skip sends when no listener is active. |
| Timer expiry after auction already closed | Timer checks `timers[i].active` flag under mutex before calling `auction_close()`, preventing double-close. |
| Data directory not existing on fresh clone | Added `ensure_environment()` in `main.c` and `make setup` target to create `data/auctions/` and `data/bids/` automatically. |
| Client not receiving broadcasts mid-session | Maintained a `client_sockets[]` array protected by `clients_mutex`. The IPC listener callback calls `broadcast_to_clients()` which iterates the array and sends to all active sockets. |

---

## 6. Conclusion

The Concurrent Auction System successfully demonstrates all six mandatory OS concepts required by the EGC 301P project specification:

| Requirement | Implementation | Status |
|-------------|---------------|--------|
| Role-Based Authorization | `auth.c` — ADMIN/BIDDER/VIEWER enum + permission gates | ✅ |
| File Locking | `file_manager.c` — `fcntl` F_RDLCK/F_WRLCK advisory locks | ✅ |
| Concurrency Control (Mutex) | `bid_processor.c` — `pthread_mutex_t bid_mutex` | ✅ |
| Concurrency Control (Semaphore) | `socket_server.c` — `sem_open` counting semaphore | ✅ |
| Data Consistency | Two-layer: mutex + fcntl; verified by concurrent stress tests | ✅ |
| Socket Programming | `socket_server.c` + `client.c` — full TCP client-server | ✅ |
| IPC — Named Pipe | `ipc.c` — POSIX FIFO event bus | ✅ |
| IPC — Signals | `timer.c` / `main.c` — SIGALRM heartbeat + SIGINT shutdown | ✅ |

All modules compile cleanly with `-Wall -Wextra -std=c99` and all 6 unit tests pass.

---

*Language: C (C99) | Platform: POSIX (Linux/macOS) | Build: GNU Make*
