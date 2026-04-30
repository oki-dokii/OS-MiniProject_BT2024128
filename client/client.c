/**
 * Concurrent Auction System - Client
 * ===================================
 * A rich interactive CLI client that connects to the auction server over TCP.
 *
 * Usage:
 *   ./auction_client [host] [port]
 *   Defaults: host=127.0.0.1, port=8080
 *
 * Commands:
 *   LOGIN  <username> <password>   - Authenticate with the server
 *   LIST                           - List all active auctions
 *   SEARCH <keyword>               - Search auctions by item name
 *   CREATE <name> <price> <secs>   - Create a new auction (Admin only)
 *   BID    <auction_id> <amount>   - Place a bid on an auction
 *   HISTORY <auction_id>           - View the bid history of an auction
 *   CLOSE  <auction_id>            - Force-close an auction (Admin only)
 *   HELP                           - Show this help menu
 *   QUIT                           - Disconnect from server
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>

#define BUFFER_SIZE    4096
#define DEFAULT_HOST   "127.0.0.1"
#define DEFAULT_PORT   8080

/* ANSI color codes */
#define CLR_RESET   "\033[0m"
#define CLR_BOLD    "\033[1m"
#define CLR_RED     "\033[31m"
#define CLR_GREEN   "\033[32m"
#define CLR_YELLOW  "\033[33m"
#define CLR_CYAN    "\033[36m"
#define CLR_MAGENTA "\033[35m"
#define CLR_WHITE   "\033[97m"

static void print_banner() {
    printf(CLR_CYAN CLR_BOLD);
    printf("╔══════════════════════════════════════════╗\n");
    printf("║       CONCURRENT AUCTION SYSTEM          ║\n");
    printf("║       Interactive Client v1.0            ║\n");
    printf("╚══════════════════════════════════════════╝\n");
    printf(CLR_RESET);
}

static void print_help() {
    printf(CLR_YELLOW CLR_BOLD "\n  === COMMAND REFERENCE ===\n" CLR_RESET);
    printf(CLR_CYAN "  %-30s" CLR_RESET "%s\n", "LOGIN <user> <pass>",    "Authenticate your session");
    printf(CLR_CYAN "  %-30s" CLR_RESET "%s\n", "LIST",                   "List all auctions");
    printf(CLR_CYAN "  %-30s" CLR_RESET "%s\n", "SEARCH <keyword>",       "Search auctions by item name");
    printf(CLR_CYAN "  %-30s" CLR_RESET "%s\n", "CREATE <name> <p> <t>",  "Create auction (Admin only)");
    printf(CLR_CYAN "  %-30s" CLR_RESET "%s\n", "BID <id> <amount>",      "Place a bid on an auction");
    printf(CLR_CYAN "  %-30s" CLR_RESET "%s\n", "HISTORY <id>",           "View bid history for an auction");
    printf(CLR_CYAN "  %-30s" CLR_RESET "%s\n", "CLOSE <id>",             "Force-close auction (Admin only)");
    printf(CLR_CYAN "  %-30s" CLR_RESET "%s\n", "HELP",                   "Show this help");
    printf(CLR_CYAN "  %-30s" CLR_RESET "%s\n", "QUIT",                   "Disconnect from server");
    printf("\n");
}

static void print_response(const char *resp) {
    if (strncmp(resp, "SUCCESS", 7) == 0 || strncmp(resp, "✓", 3) == 0) {
        printf(CLR_GREEN "%s" CLR_RESET, resp);
    } else if (strncmp(resp, "ERROR", 5) == 0) {
        printf(CLR_RED "%s" CLR_RESET, resp);
    } else if (strncmp(resp, "[BROADCAST]", 11) == 0 || strncmp(resp, "[AUCTION", 8) == 0) {
        printf(CLR_MAGENTA CLR_BOLD "%s" CLR_RESET, resp);
    } else {
        printf("%s", resp);
    }
}

int main(int argc, char *argv[]) {
    const char *host = (argc > 1) ? argv[1] : DEFAULT_HOST;
    int         port = (argc > 2) ? atoi(argv[2]) : DEFAULT_PORT;

    int sock;
    struct sockaddr_in serv_addr;
    char input[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    print_banner();

    /* --- Create TCP socket --- */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, CLR_RED "[ERROR] Socket creation failed: %s\n" CLR_RESET, strerror(errno));
        return EXIT_FAILURE;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons((uint16_t)port);

    if (inet_pton(AF_INET, host, &serv_addr.sin_addr) <= 0) {
        fprintf(stderr, CLR_RED "[ERROR] Invalid address: %s\n" CLR_RESET, host);
        close(sock);
        return EXIT_FAILURE;
    }

    /* --- Connect to server --- */
    printf(CLR_WHITE "  Connecting to %s:%d ...\n" CLR_RESET, host, port);
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, CLR_RED "[ERROR] Connection failed: %s\n"
                        "        Is the server running? Try: ./auction_server\n" CLR_RESET,
                strerror(errno));
        close(sock);
        return EXIT_FAILURE;
    }
    printf(CLR_GREEN "  Connected successfully!\n" CLR_RESET);
    print_help();

    /* --- Main REPL loop --- */
    while (1) {
        printf(CLR_BOLD CLR_YELLOW "auction> " CLR_RESET);
        fflush(stdout);

        if (!fgets(input, BUFFER_SIZE, stdin)) {
            /* EOF (Ctrl+D) */
            break;
        }

        /* Trim trailing newline */
        input[strcspn(input, "\r\n")] = '\0';

        /* Skip blank lines */
        if (strlen(input) == 0) continue;

        /* Local HELP command — no network round-trip needed */
        if (strcasecmp(input, "HELP") == 0) {
            print_help();
            continue;
        }

        /* Send command to server */
        char send_buf[BUFFER_SIZE];
        snprintf(send_buf, sizeof(send_buf), "%s\n", input);
        if (send(sock, send_buf, strlen(send_buf), 0) < 0) {
            fprintf(stderr, CLR_RED "[ERROR] Send failed: %s\n" CLR_RESET, strerror(errno));
            break;
        }

        /* QUIT: send and exit cleanly */
        if (strncasecmp(input, "QUIT", 4) == 0) {
            memset(response, 0, BUFFER_SIZE);
            recv(sock, response, BUFFER_SIZE - 1, 0);
            printf(CLR_CYAN "%s\n" CLR_RESET, response);
            break;
        }

        /* Read server response */
        memset(response, 0, BUFFER_SIZE);
        ssize_t bytes = recv(sock, response, BUFFER_SIZE - 1, 0);
        if (bytes <= 0) {
            printf(CLR_RED "\n[INFO] Server closed the connection.\n" CLR_RESET);
            break;
        }
        response[bytes] = '\0';

        printf("\n");
        print_response(response);
        printf("\n");
    }

    close(sock);
    printf(CLR_CYAN "[INFO] Disconnected. Goodbye!\n" CLR_RESET);
    return EXIT_SUCCESS;
}
