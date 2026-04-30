#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/select.h>
#include <stdbool.h>

#define DEFAULT_PORT 8080
#define DEFAULT_HOST "127.0.0.1"
#define BUFFER_SIZE 1024

/* ANSI Color Codes for premium UI */
#define CLR_RESET   "\x1b[0m"
#define CLR_BOLD    "\x1b[1m"
#define CLR_RED     "\x1b[31m"
#define CLR_GREEN   "\x1b[32m"
#define CLR_YELLOW  "\x1b[33m"
#define CLR_BLUE    "\x1b[34m"
#define CLR_MAGENTA "\x1b[35m"
#define CLR_CYAN    "\x1b[36m"

void print_help() {
    printf(CLR_BOLD CLR_CYAN "  === COMMAND REFERENCE ===\n" CLR_RESET);
    printf("  LOGIN <user> <pass>           Authenticate your session\n");
    printf("  LIST                          List all auctions\n");
    printf("  SEARCH <keyword>              Search auctions by item name\n");
    printf("  CREATE <name> <p> <t>         Create auction (Admin only)\n");
    printf("  BID <id> <amount>             Place a bid on an auction\n");
    printf("  HISTORY <id>                  View bid history for an auction\n");
    printf("  CLOSE <id>                    Force-close auction (Admin only)\n");
    printf("  HELP                          Show this help\n");
    printf("  QUIT                          Disconnect from server\n\n");
}

int main(int argc, char *argv[]) {
    const char *host = (argc > 1) ? argv[1] : DEFAULT_HOST;
    int port = (argc > 2) ? atoi(argv[2]) : DEFAULT_PORT;

    int sock;
    struct sockaddr_in serv_addr;

    printf(CLR_BOLD CLR_BLUE "╔══════════════════════════════════════════╗\n");
    printf("║       CONCURRENT AUCTION SYSTEM          ║\n");
    printf("║       Interactive Client v1.1            ║\n");
    printf("╚══════════════════════════════════════════╝\n" CLR_RESET);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf(CLR_RED "\n Socket creation error \n" CLR_RESET);
        return EXIT_FAILURE;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host, &serv_addr.sin_addr) <= 0) {
        printf(CLR_RED "\n Invalid address/ Address not supported \n" CLR_RESET);
        return EXIT_FAILURE;
    }

    printf("  Connecting to %s:%d ...\n", host, port);
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf(CLR_RED "  Connection Failed: %s\n" CLR_RESET, strerror(errno));
        close(sock);
        return EXIT_FAILURE;
    }
    printf(CLR_GREEN "  Connected successfully!\n\n" CLR_RESET);
    print_help();

    fd_set read_fds;
    char input[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    while (1) {
        printf(CLR_BOLD CLR_YELLOW "auction> " CLR_RESET);
        fflush(stdout);

        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(sock, &read_fds);

        int max_fd = (sock > STDIN_FILENO) ? sock : STDIN_FILENO;

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select failed");
            break;
        }

        /* Server sent a message (Broadcast or Command Response) */
        if (FD_ISSET(sock, &read_fds)) {
            memset(response, 0, BUFFER_SIZE);
            int bytes = recv(sock, response, BUFFER_SIZE - 1, 0);
            if (bytes <= 0) {
                printf("\n" CLR_RED "[SYSTEM] Server disconnected." CLR_RESET "\n");
                break;
            }
            /* Overwrite current prompt line with server message */
            printf("\r%s\n", response);
        }

        /* User typed a command */
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            if (!fgets(input, BUFFER_SIZE, stdin)) break;
            input[strcspn(input, "\r\n")] = '\0';
            if (strlen(input) == 0) continue;

            if (strcasecmp(input, "HELP") == 0) {
                print_help();
                continue;
            }

            if (strcasecmp(input, "QUIT") == 0) break;

            char send_buf[BUFFER_SIZE];
            snprintf(send_buf, sizeof(send_buf), "%s\n", input);
            if (send(sock, send_buf, strlen(send_buf), 0) < 0) {
                perror("send failed");
                break;
            }
        }
    }

    close(sock);
    printf(CLR_CYAN "Goodbye!\n" CLR_RESET);
    return 0;
}
