#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 2048

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    printf("--- AUCTION SYSTEM CLIENT ---\n");
    printf("Commands: LOGIN, CREATE, LIST, BID, HISTORY, CLOSE, QUIT\n\n");

    while (1) {
        printf("> ");
        if (!fgets(buffer, BUFFER_SIZE, stdin)) break;
        
        send(sock, buffer, strlen(buffer), 0);
        
        if (strncmp(buffer, "QUIT", 4) == 0) break;

        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(sock, buffer, BUFFER_SIZE);
        if (valread <= 0) {
            printf("Server disconnected.\n");
            break;
        }
        printf("%s\n", buffer);
    }

    close(sock);
    return 0;
}

