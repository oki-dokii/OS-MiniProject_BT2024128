#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include "auth.h"

/* Ensure the users database exists before running auth tests */
static void seed_users_db() {
    mkdir("data", 0755); /* OK if already exists */
    FILE *f = fopen("data/users.txt", "r");
    if (f) { fclose(f); return; } /* Already exists */
    f = fopen("data/users.txt", "w");
    if (!f) { perror("Cannot create data/users.txt"); exit(1); }
    fprintf(f, "admin:admin123:ADMIN\n");
    fprintf(f, "alice:alice123:BIDDER\n");
    fprintf(f, "bob:bob456:BIDDER\n");
    fprintf(f, "guest1:guest123:VIEWER\n");
    fclose(f);
    printf("[SETUP] Created data/users.txt for test.\n\n");
}

void test_user(const char *username, const char *password) {
    Session session = {0};
    printf("Testing login for user: %s\n", username);
    
    if (auth_login(username, password, &session)) {
        printf("  ✓ Login successful\n");
        printf("  - Can bid: %s\n", auth_can_bid(&session) ? "YES" : "NO");
        printf("  - Can create: %s\n", auth_can_create(&session) ? "YES" : "NO");
        printf("  - Can close: %s\n", auth_can_close(&session) ? "YES" : "NO");
    } else {
        printf("  ✗ Login failed\n");
    }
    printf("\n");
}

int main() {
    printf("--- AUCTION SYSTEM AUTH TEST ---\n\n");
    seed_users_db();

    // Case 1: Admin
    test_user("admin", "admin123");

    // Case 2: Bidder
    test_user("alice", "alice123");

    // Case 3: Viewer
    test_user("guest1", "guest123");

    // Case 4: Wrong password
    test_user("admin", "wrongpass");

    // Case 5: Nonexistent user
    test_user("nobody", "pass");

    return 0;
}
