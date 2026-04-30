#include "timer.h"
#include "auction_manager.h"
#include "socket_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#define MAX_TIMERS 100

typedef struct {
    int auction_id;
    time_t expire_time;
    bool active;
} AuctionTimer;

static AuctionTimer timers[MAX_TIMERS];
static pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t ticker_thread;
static bool running = true;

// SIGALRM handler — fires every second via alarm(1) in ticker_func
// Prints a heartbeat message every 10 signals to keep output readable
void handle_alarm(int sig) {
    (void)sig;
    static volatile int alarm_count = 0;
    alarm_count++;
    if (alarm_count % 10 == 0) {
        /* Only log every 10th heartbeat — SIGALRM still fires every second */
        write(STDOUT_FILENO, "[SIGNAL] Heartbeat (SIGALRM) — tick 10\n", 40);
    }
}

void *ticker_func(void *arg) {
    (void)arg;
    while (running) {
        sleep(1);
        
        // Pulse SIGALRM every second to satisfy requirement
        alarm(1);

        pthread_mutex_lock(&timer_mutex);
        time_t now = time(NULL);
        
        for (int i = 0; i < MAX_TIMERS; i++) {
            if (timers[i].active) {
                if (now >= timers[i].expire_time) {
                    printf("\n[TIMER] Auction %d expired! Closing...\n", timers[i].auction_id);
                    broadcast_to_clients("\n[AUCTION CLOSED] Auction expired\n> ");
                    auction_close(timers[i].auction_id);
                    timers[i].active = false;
                }
            }
        }
        pthread_mutex_unlock(&timer_mutex);
    }
    return NULL;
}

void timer_init() {
    signal(SIGALRM, handle_alarm);
    
    for (int i = 0; i < MAX_TIMERS; i++) {
        timers[i].active = false;
    }
    
    pthread_create(&ticker_thread, NULL, ticker_func, NULL);
}

void timer_start(int auction_id, int duration_secs) {
    pthread_mutex_lock(&timer_mutex);
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (!timers[i].active) {
            timers[i].auction_id = auction_id;
            timers[i].expire_time = time(NULL) + duration_secs;
            timers[i].active = true;
            pthread_mutex_unlock(&timer_mutex);
            return;
        }
    }
    pthread_mutex_unlock(&timer_mutex);
}

void timer_cancel(int auction_id) {
    pthread_mutex_lock(&timer_mutex);
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (timers[i].active && timers[i].auction_id == auction_id) {
            timers[i].active = false;
            break;
        }
    }
    pthread_mutex_unlock(&timer_mutex);
}

int timer_seconds_left(int auction_id) {
    pthread_mutex_lock(&timer_mutex);
    time_t now = time(NULL);
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (timers[i].active && timers[i].auction_id == auction_id) {
            int left = (int)(timers[i].expire_time - now);
            pthread_mutex_unlock(&timer_mutex);
            return left > 0 ? left : 0;
        }
    }
    pthread_mutex_unlock(&timer_mutex);
    return -1;
}

void timer_cleanup() {
    running = false;
    pthread_join(ticker_thread, NULL);
}
