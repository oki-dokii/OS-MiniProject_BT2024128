#ifndef AUTH_H
#define AUTH_H

#include <stdbool.h>

typedef enum {
    ROLE_ADMIN,
    ROLE_BIDDER,
    ROLE_VIEWER,
    ROLE_NONE
} Role;

typedef struct {
    char username[50];
    Role role;
    bool authenticated;
} Session;

// Function prototypes
bool auth_login(const char *username, const char *password, Session *session);
bool auth_can_bid(const Session *session);
bool auth_can_create(const Session *session);
bool auth_can_close(const Session *session);

#endif // AUTH_H
