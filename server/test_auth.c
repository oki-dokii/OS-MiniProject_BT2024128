#include <stdio.h>
#include "auth.h"

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
