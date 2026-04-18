#include "auth.h"
#include <stdio.h>
#include <string.h>

#define USERS_DB "data/users.txt"

Role string_to_role(const char *role_str) {
    if (strcmp(role_str, "ADMIN") == 0) return ROLE_ADMIN;
    if (strcmp(role_str, "BIDDER") == 0) return ROLE_BIDDER;
    if (strcmp(role_str, "VIEWER") == 0) return ROLE_VIEWER;
    return ROLE_NONE;
}

bool auth_login(const char *username, const char *password, Session *session) {
    FILE *file = fopen(USERS_DB, "r");
    if (!file) {
        perror("Failed to open users database");
        return false;
    }

    char line[150];
    char db_user[50], db_pass[50], db_role[20];
    bool found = false;

    while (fgets(line, sizeof(line), file)) {
        // Format: username:password:role
        if (sscanf(line, "%[^:]:%[^:]:%s", db_user, db_pass, db_role) == 3) {
            if (strcmp(username, db_user) == 0 && strcmp(password, db_pass) == 0) {
                strncpy(session->username, db_user, sizeof(session->username));
                session->role = string_to_role(db_role);
                session->authenticated = true;
                found = true;
                break;
            }
        }
    }

    fclose(file);
    return found;
}

bool auth_can_bid(const Session *session) {
    if (!session || !session->authenticated) return false;
    return session->role == ROLE_ADMIN || session->role == ROLE_BIDDER;
}

bool auth_can_create(const Session *session) {
    if (!session || !session->authenticated) return false;
    return session->role == ROLE_ADMIN;
}

bool auth_can_close(const Session *session) {
    if (!session || !session->authenticated) return false;
    return session->role == ROLE_ADMIN;
}
