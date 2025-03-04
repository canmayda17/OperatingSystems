#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>

#define MAX_LINES 100
#define MAX_LINE_LENGTH 1000

// Global variables
char lines[MAX_LINES][MAX_LINE_LENGTH]; // Array to store file lines
int totalLines = 0; // Total number of lines in the file
int currentReadIndex = 0; // Index tracker for read threads

pthread_mutex_t read_mutex; // Mutex for read thread synchronization
pthread_mutex_t process_mutex; // Mutex for processing and write thread synchronization

// Function to read a line from the file
void *readThreads(void *arg) {
    pthread_mutex_lock(&read_mutex); // Ensure to only one thread reads

    // ***IF YOU PLAN TO USE A DIFFERENT NAMED INPUT TEXT FILE, PLEASE INSERT FROM HERE!*** 
    // Default is deneme.txt
    FILE *file = fopen("deneme.txt", "r");
    if (file == NULL) {
        printf("Error: File not found or cannot be opened.\n");
        pthread_mutex_unlock(&read_mutex); // Unlock before exit
        pthread_exit(NULL);
    }

    int lineIndex = currentReadIndex;
    currentReadIndex++; // Next line

    for (int i = 0; i <= lineIndex; i++) {
        if (fgets(lines[lineIndex], MAX_LINE_LENGTH, file) == NULL) {
            fclose(file);
            pthread_mutex_unlock(&read_mutex);
            pthread_exit(NULL); // Exit when there is no more lines
        }
    }

    // Remove newline character and check for empty lines
    lines[lineIndex][strcspn(lines[lineIndex], "\n")] = '\0';
    if (strlen(lines[lineIndex]) == 0) {
        fclose(file);
        pthread_mutex_unlock(&read_mutex);
        pthread_exit(NULL); // Exit for empty line
    }

    totalLines++; // Total number of lines incremented.
    fclose(file);
    // Printf to make table view
    printf("Read_%d                          Read_%d Read the line %d which is \"%s\"\n", lineIndex + 1, lineIndex + 1, lineIndex + 1, lines[lineIndex]);
    pthread_mutex_unlock(&read_mutex);
    pthread_exit(NULL); // Exit from thread
}

// Function to convert a line to uppercase
void *upperThreads(void *arg) {
    int *lineIndex = (int *)arg;

    pthread_mutex_lock(&process_mutex); // Lock mutex

    // Create a copy of the original line
    char mainLine[MAX_LINE_LENGTH];
    strcpy(mainLine, lines[*lineIndex]);

    // Convert the line to uppercase, if it is a letter between a and z.
    for (int i = 0; lines[*lineIndex][i] != '\0'; i++) {
        if (lines[*lineIndex][i] >= 'a' && lines[*lineIndex][i] <= 'z') {
            lines[*lineIndex][i] -= 32;
        }
    }

    // Printf to make table view
    printf("Upper_%d                         Upper_%d read index %d and converted \"%s\" to \"%s\"\n", 
        *lineIndex + 1, *lineIndex + 1, *lineIndex + 1, mainLine, lines[*lineIndex]);

    pthread_mutex_unlock(&process_mutex); // Unlock the mutex
    pthread_exit(NULL); // Exit from thread
}

// Function to replace spaces with underscores
void *replaceThreads(void *arg) {
    int *lineIndex = (int *)arg;

    pthread_mutex_lock(&process_mutex); // Lock mutex

    // Create a copy of the original line
    char mainLine[MAX_LINE_LENGTH];
    strcpy(mainLine, lines[*lineIndex]);

    // Replace spaces with underscores
    for (int i = 0; lines[*lineIndex][i] != '\0'; i++) {
        if (lines[*lineIndex][i] == ' ') {
            lines[*lineIndex][i] = '_';
        }
    }

    // Printf to make table view
    printf("Replace_%d                       Replace_%d read index %d and replaced spaces in \"%s\" to \"%s\"\n", 
        *lineIndex + 1, *lineIndex + 1, *lineIndex + 1, mainLine, lines[*lineIndex]);

    pthread_mutex_unlock(&process_mutex); // Unlock the mutex
    pthread_exit(NULL); // Exit from the thread
}

// Function to write processed lines to the output file
void *writeThreads(void *arg) {

    // Output is written to output.txt
    FILE *file = fopen("output.txt", "w");
    if (file == NULL) {
        perror("Error opening output file");
        pthread_mutex_unlock(&process_mutex); // Before exit ensure to unlock mutex
        pthread_exit(NULL);
    }

    // Write all changed lines
    for (int i = 0; i < totalLines; i++) {
        // Printf to make table view
        fprintf(file, "%s\n", lines[i]);
        printf("Write_%d                         Write_%d write line %d back which is \"%s\"\n", i + 1, i + 1,  i + 1, lines[i]);
    }

    fclose(file); // Closing file, it is done
    pthread_mutex_unlock(&process_mutex); // Unlock mutex after closed file
    pthread_exit(NULL); // Exit from the thread
}

int main(int argc, char *argv[]) {
    
    if (argc < 6 || strcmp(argv[1], "-d") != 0 || strcmp(argv[3], "-n") != 0) {
        fprintf(stderr, "Usage: %s -d <file_name> -n <read_threads> <upper_threads> <replace_threads> <write_threads>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Read arguments
    const char *fileName = argv[2];
    int readThreadsNumber = atoi(argv[4]);
    int upperThreadsNumber = atoi(argv[5]);
    int replaceThreadsNumber = atoi(argv[6]);
    int writeThreadsNumber = atoi(argv[7]);

    // Initialize mutexes
    pthread_mutex_init(&read_mutex, NULL);
    pthread_mutex_init(&process_mutex, NULL);

    // Initialize thread arrays
    pthread_t read_threads[readThreadsNumber];
    pthread_t upper_threads[upperThreadsNumber];
    pthread_t replace_threads[replaceThreadsNumber];
    pthread_t write_threads[writeThreadsNumber];

    // Thread index
    int threadIndex[MAX_LINES];

    printf("<Thread-type and ID>            <Output>\n");

    // Create read threads
    for (int i = 0; i < readThreadsNumber; i++) {
        threadIndex[i] = i;
        if (pthread_create(&read_threads[i], NULL, readThreads, &threadIndex[i]) != 0) {
            perror("Error creating read thread");
            exit(EXIT_FAILURE);
        }
    }

    // Wait for threads to finish
    for (int i = 0; i < readThreadsNumber; i++) {
        pthread_join(read_threads[i], NULL);
    }

    // Create upper threads
    for (int i = 0; i < upperThreadsNumber; i++) {
        threadIndex[i] = i;
        if (pthread_create(&upper_threads[i], NULL, upperThreads, &threadIndex[i]) != 0) {
            perror("Error creating upper thread");
            exit(EXIT_FAILURE);
        }
    }

    // Create replace threads
    for (int i = 0; i < replaceThreadsNumber; i++) {
        threadIndex[i] = i;
        if (pthread_create(&replace_threads[i], NULL, replaceThreads, &threadIndex[i]) != 0) {
            perror("Error creating replace thread");
            exit(EXIT_FAILURE);
        }
    }

    // Wait for upper threads to finish
    for (int i = 0; i < upperThreadsNumber; i++) {
        pthread_join(upper_threads[i], NULL);
    }

    // Wait for replace threads to finish
    for (int i = 0; i < replaceThreadsNumber; i++) {
        pthread_join(replace_threads[i], NULL);
    }

    // Create write thread
    pthread_t write_thread;
    if (pthread_create(&write_thread, NULL, writeThreads, NULL) != 0) {
        perror("Error creating write thread");
        exit(EXIT_FAILURE);
    }

    // Wait for write threads to finish
    pthread_join(write_thread, NULL);

    // Destroy mutexes end exit from the program
    pthread_mutex_destroy(&read_mutex);
    pthread_mutex_destroy(&process_mutex);

    return 0;
}