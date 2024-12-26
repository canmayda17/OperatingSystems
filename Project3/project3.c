#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <semaphore.h>
#include <stdint.h>

#define MAX_LINES 100
#define MAX_LINE_LENGTH 1024

// Shared data structures
char lines[MAX_LINES][MAX_LINE_LENGTH];
int line_count = 0;
int upper_done[MAX_LINES] = {0};
int replace_done[MAX_LINES] = {0};

pthread_mutex_t line_mutex[MAX_LINES];
sem_t upper_sem[MAX_LINES];
sem_t replace_sem[MAX_LINES];

// Function declarations
void *read_thread(void *arg);
void *upper_thread(void *arg);
void *replace_thread(void *arg);
void *write_thread(void *arg);

// Helper function to convert string to uppercase
void to_uppercase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = toupper((unsigned char)str[i]);
    }
}

// Helper function to replace spaces with underscores
void replace_spaces(char *str) {
    for (int i = 0; str[i]; i++) {
        if (str[i] == ' ') {
            str[i] = '_';
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s -d <filename> -n <read> <upper> <replace> <write>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Parse arguments
    char *filename = argv[2];
    int read_count = atoi(argv[4]);
    int upper_count = atoi(argv[5]);
    int replace_count = atoi(argv[6]);
    int write_count = atoi(argv[7]);

    // Initialize mutexes and semaphores
    for (int i = 0; i < MAX_LINES; i++) {
        pthread_mutex_init(&line_mutex[i], NULL);
        sem_init(&upper_sem[i], 0, 1); // Start with upper thread allowed
        sem_init(&replace_sem[i], 0, 0); // Replace thread waits
    }

    // Read file and count lines
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        return EXIT_FAILURE;
    }
    while (fgets(lines[line_count], MAX_LINE_LENGTH, file)) {
        lines[line_count][strcspn(lines[line_count], "\n")] = '\0'; // Remove newline
        line_count++;
    }
    fclose(file);

    // Create threads
    pthread_t read_threads[read_count], upper_threads[upper_count],
        replace_threads[replace_count], write_threads[write_count];

    for (int i = 0; i < read_count; i++) {
        pthread_create(&read_threads[i], NULL, read_thread, (void *)(intptr_t)i);
    }
    for (int i = 0; i < upper_count; i++) {
        pthread_create(&upper_threads[i], NULL, upper_thread, (void *)(intptr_t)i);
    }
    for (int i = 0; i < replace_count; i++) {
        pthread_create(&replace_threads[i], NULL, replace_thread, (void *)(intptr_t)i);
    }
    for (int i = 0; i < write_count; i++) {
        pthread_create(&write_threads[i], NULL, write_thread, (void *)(intptr_t)i);
    }

    // Wait for threads to finish
    for (int i = 0; i < read_count; i++) {
        pthread_join(read_threads[i], NULL);
    }
    for (int i = 0; i < upper_count; i++) {
        pthread_join(upper_threads[i], NULL);
    }
    for (int i = 0; i < replace_count; i++) {
        pthread_join(replace_threads[i], NULL);
    }
    for (int i = 0; i < write_count; i++) {
        pthread_join(write_threads[i], NULL);
    }

    // Cleanup
    for (int i = 0; i < MAX_LINES; i++) {
        pthread_mutex_destroy(&line_mutex[i]);
        sem_destroy(&upper_sem[i]);
        sem_destroy(&replace_sem[i]);
    }

    return EXIT_SUCCESS;
}

void *read_thread(void *arg) {
    int id = (intptr_t)arg + 1;

    for (int i = id - 1; i < line_count; i += 4) {
        pthread_mutex_lock(&line_mutex[i]);
        printf("Read_%d Read_%d read the line %d which is \"%s\"\n", id, id, i + 1, lines[i]);
        pthread_mutex_unlock(&line_mutex[i]);
    }

    return NULL;
}

void *upper_thread(void *arg) {
    int id = (intptr_t)arg + 1;

    while (1) {
        int found = 0;
        for (int i = 0; i < line_count; i++) {
            if (sem_trywait(&upper_sem[i]) == 0) {
                pthread_mutex_lock(&line_mutex[i]);
                printf("Upper_%d Upper_%d read index %d and converted \"%s\" to ", id, id, i + 1, lines[i]);
                to_uppercase(lines[i]);
                printf("\"%s\"\n", lines[i]);
                sem_post(&replace_sem[i]);
                pthread_mutex_unlock(&line_mutex[i]);
                found = 1;
                break;
            }
        }
        if (!found) break;
    }

    return NULL;
}

void *replace_thread(void *arg) {
    int id = (intptr_t)arg + 1;

    while (1) {
        int found = 0;
        for (int i = 0; i < line_count; i++) {
            if (sem_trywait(&replace_sem[i]) == 0) {
                pthread_mutex_lock(&line_mutex[i]);
                printf("Replace_%d Replace_%d read index %d and converted \"%s\" to ", id, id, i + 1, lines[i]);
                replace_spaces(lines[i]);
                printf("\"%s\"\n", lines[i]);
                sem_post(&upper_sem[i]);
                pthread_mutex_unlock(&line_mutex[i]);
                found = 1;
                break;
            }
        }
        if (!found) break;
    }

    return NULL;
}

void *write_thread(void *arg) {
    int id = (intptr_t)arg + 1;

    for (int i = id - 1; i < line_count; i += 4) {
        pthread_mutex_lock(&line_mutex[i]);
        printf("Writer_%d Writer_%d write line %d back which is \"%s\"\n", id, id, i + 1, lines[i]);
        pthread_mutex_unlock(&line_mutex[i]);
    }

    return NULL;
}