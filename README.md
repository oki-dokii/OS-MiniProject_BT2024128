# 🏛️ OS-Inspired Distributed Auction System

A robust, multi-user auction server built in C to demonstrate core Operating Systems concepts. The system features role-based authentication, disk-backed persistence with hardware-level file locking, atomic transaction processing via mutual exclusion, background automated task management using signals, and cross-subsystem event broadcasting through named pipes.

---

## 🏗️ System Architecture

| Module | File | OS Concept | Description |
| :--- | :--- | :--- | :--- |
| **Auth** | `auth.c` | RBAC | Role-based permission gating for Admin/Bidder/Viewer. |
| **File Mgr** | `file_manager.c` | Advisory Locking | `fcntl`-based read/write locks for safe concurrent file I/O. |
| **Auction Mgr** | `auction_manager.c` | Persistence | CRUD operations for auctions with disk serialization. |
| **Bid Proc** | `bid_processor.c` | Mutex | Atomic validation and record updates. |
| **Network** | `socket_server.c` | Sockets / Semaphore | TCP server with client connection limiting. |
| **Timer** | `timer.c` | Signal Heartbeat | `SIGALRM` driven automated cleanup. |
| **IPC** | `ipc.c` | Named Pipes (FIFO) | System-wide event broadcasting via POSIX FIFOs. |

---

## 🛠️ Mandatory OS Concepts

### 1. Role-Based Access Control (RBAC)
Implemented in `auth.c`. The system uses an `enum Role { ROLE_ADMIN, ROLE_BIDDER, ROLE_VIEWER }` to gate operations. Functions like `auth_can_create()` and `auth_can_bid()` are called before any core logic executes, ensuring that a "Guest" cannot create auctions and a "Bidder" cannot close them.

### 2. File Locking (Advisory Locks)
Implemented in `file_manager.c`. Instead of simple file I/O, every read/write operation applies `fcntl()` advisory locks.
- **Shared Locks (`F_RDLCK`)**: Allow multiple concurrent readers.
- **Exclusive Locks (`F_WRLCK`)**: Block all other readers and writers during record updates.
This prevents the "Lost Update" problem at the filesystem level.

### 3. Mutual Exclusion (Atoms)
Implemented in `bid_processor.c`. While the File Manager protects the *disk*, the `bid_mutex` (`pthread_mutex_t`) protects the *logic*. It ensures that the sequence "Read current price → Validate new bid → Write new record" is fully atomic, preventing race conditions where two users might tie for the same price.

### 4. Process Management & Signals
Implemented in `timer.c`. The system uses `signal(SIGALRM, ...)` and `alarm(1)` to create a recurring heartbeat. This pulse triggers a visible notification via the signal handler and allows a background thread to monitor all active auctions, automatically triggering `auction_close()` when time expires.

### 5. Semaphores (Resource Counting)
Implemented in `socket_server.c`. We use a counting semaphore (`sem_t connection_limit`) to limit the number of concurrent client connections (set to 10). This demonstrates OS-level resource management and thread blocking, as new clients will be held at `sem_wait()` if the server is at capacity.

### 5. Inter-Process Communication (IPC)
Implemented in `ipc.c`. A named pipe (FIFO) at `data/auction_events.fifo` serves as a broadcast channel. Modules like `bid_processor.c` write binary `AuctionEvent` structs to the pipe, which are picked up by a listener thread to provide real-time notification across the system.

### 6. Network Programming
Implemented in `socket_server.c`. Using the POSIX Sockets API (`socket`, `bind`, `listen`, `accept`), the server exposes a TCP port (8080). It uses a "One Thread Per Client" model to maintain stateful user sessions concurrently.

---

## 🚀 Getting Started

### Prerequisites
- GCC Compiler
- POSIX-compliant OS (Linux/macOS)

### Build Instructions
```bash
cd auction_system
make all      # Builds both server and client binaries
```

### How to Run
1. **Start the Server**:
   ```bash
   ./auction_server
   ```
2. **Connect as Admin** (Open a new terminal):
   ```bash
   ./auction_client
   > LOGIN admin admin123
   > CREATE "Vintage Rolex" 5000 60
   ```
3. **Connect as Bidder** (Open another terminal):
   ```bash
   ./auction_client
   > LOGIN alice alice123
   > LIST
   > BID 1 5500
   ```

---

## 📝 Command Reference

| Command | Role | Description | Example |
| :--- | :--- | :--- | :--- |
| `LOGIN <u:p>` | Anyone | Authenticate session | `LOGIN admin admin123` |
| `CREATE <n:p:t>` | Admin | Start a new auction | `CREATE Laptop 1200 60` |
| `LIST` | Anyone | List all open auctions | `LIST` |
| `BID <id:val>` | Bidder | Place a higher bid | `BID 1 1300` |
| `HISTORY <id>` | Anyone | Show bid history log | `HISTORY 1` |
| `CLOSE <id>` | Admin | Force-close an auction | `CLOSE 1` |
| `QUIT` | Anyone | Terminate connection | `QUIT` |

---

## 📁 Project Structure

```text
auction_system/
├── Makefile                # Multi-target build system
├── README.md               # Documentation
├── auction_server          # Server binary (generated)
├── auction_client          # Client binary (generated)
├── data/                   # Persistent storage
│   ├── users.txt           # User DB
│   ├── auctions/           # Auction records (.txt)
│   └── bids/               # Bid logs (.txt)
├── client/
│   └── client.c            # CLI client implementation
└── server/
    ├── main.c              # Server entry point
    ├── auth.h/c            # Security logic
    ├── file_manager.h/c    # fcntl locking
    ├── auction_manager.h/c # Auction state logic
    ├── bid_processor.h/c   # Mutex-protected bidding
    ├── timer.h/c           # Signal-based countdowns
    ├── ipc.h/c             # Named pipe events
    └── socket_server.h/c   # TCP networking
```

---

## 🧠 Design Decisions

- **Mutex vs Semaphore**: Both are used in this project. **Mutexes** are used in the Bid Processor for exclusive ownership of a transaction. **Semaphores** are used in the Socket Server as a "counting" mechanism to limit concurrent socket sessions, demonstrating their utility in resource throttling.
- **Named Pipe over Shared Memory**: Chosen for IPC because it provides a simplified, file-like interface for broadcasting events. Unlike shared memory, FIFOs handle synchronization internally (via kernel-level byte-streams), making them easier to debug for event logs.
- **fcntl over flock**: `fcntl` was chosen for its flexibility. It supports byte-range locking and is POSIX standard, whereas `flock` can have inconsistent behavior on some filesystems (like NFS) and lacks the granularity needed for complex OS credit.
- **Thread-per-Client over Select/Poll**: For an OS mini-project, the thread-per-client model is much more "OS credible" as it demonstrates process/thread management and context switching. While `select/poll` is more scalable for thousands of clients, pthreads are simpler for maintaining stateful authentication sessions.

---

## 💡 Innovation & Advanced Features

- **Live Event Broadcasting**: Unlike basic request-response systems, this server maintains a registry of all active client sockets. When an event occurs (New Bid, Auction Closed), the server uses IPC triggers to broadcast real-time notifications to all connected clients simultaneously.
- **Concurrent Session Management**: The server supports multiple independent auctions running with their own dedicated timers, all managed by a single background thread and synchronized via global mutexes and semaphores.
- **Real-Time Heartbeat**: The system provides a visual signal pulse (`SIGALRM`), demonstrating low-level kernel interaction and automated resource aging.

---

## 🛠️ Technologies
- **Language**: C (C99 Standard)
- **Concurrency**: POSIX Threads (`pthreads`)
- **Locking**: `fcntl` Advisory Locks
- **Signals**: `SIGALRM`, `signal()`/`alarm()`
- **Networking**: TCP Sockets (Berkeley Sockets)
- **IPC**: POSIX FIFOs (Named Pipes)
