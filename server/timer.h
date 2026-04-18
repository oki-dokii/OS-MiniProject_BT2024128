#ifndef TIMER_H
#define TIMER_H

#include <stdbool.h>

/**
 * Initializes the timer system and starts the ticker thread.
 */
void timer_init();

/**
 * Starts a countdown for an auction.
 */
void timer_start(int auction_id, int duration_secs);

/**
 * Cancels a countdown for an auction.
 */
void timer_cancel(int auction_id);

/**
 * Returns seconds remaining for an auction, or -1 if no timer exists.
 */
int timer_seconds_left(int auction_id);

/**
 * Clean up timer resources (optional for this simple project).
 */
void timer_cleanup();

#endif // TIMER_H
