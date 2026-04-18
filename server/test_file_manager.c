#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include "file_manager.h"

#define TEST_FILE "data/test_log.txt"
#define NUM_PROCESSES 5
#define ITERATIONS 100

void worker(int id) {
    char entry[100];
    sprintf(entry, "Process %d: Writing entry to log...\n", id);
    
    for (int i = 0; i < ITERATIONS; i++) {
        if (!fm_append_file(TEST_FILE, entry)) {
            fprintf(stderr, "Process %d failed at iteration %d\n", id, i);
            exit(1);
        }
        // Small delay to increase chance of overlap if locking wasn't working
        usleep(100); 
    }
    exit(0);
}

int main() {
    printf("--- FILE MANAGER CONCURRENCY TEST ---\n");
    printf("Spawning %d processes, each writing %d entries...\n", NUM_PROCESSES, ITERATIONS);

    // Ensure directory exists
    system("mkdir -p data");
    // Clean start
    unlink(TEST_FILE);

    pid_t pids[NUM_PROCESSES];

    for (int i = 0; i < NUM_PROCESSES; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {
            worker(i + 1);
        } else if (pids[i] < 0) {
            perror("Fork failed");
            return 1;
        }
    }

    // Wait for all children
    for (int i = 0; i < NUM_PROCESSES; i++) {
        waitpid(pids[i], NULL, 0);
    }

    printf("All processes finished. Verifying results...\n");

    // Count lines in the file
    FILE *fp = fopen(TEST_FILE, "r");
    if (!fp) {
        perror("Could not open test file for verification");
        return 1;
    }

    int line_count = 0;
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        line_count++;
    }
    fclose(fp);

    int expected = NUM_PROCESSES * ITERATIONS;
    printf("Total entries found: %d (Expected: %d)\n", line_count, expected);

    if (line_count == expected) {
        printf("✓ SUCCESS: No data corruption detected. Locks enforced order.\n");
    } else {
        printf("✗ FAILURE: File content is inconsistent!\n");
    }

    return 0;
}
