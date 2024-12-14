#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
 
#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define HISTORY_SIZE 10


// Globals for managing history and foreground process
char history[HISTORY_SIZE][MAX_LINE];
int history_count = 0;
char commandPath[512]; /* Buffer to store command path */
pid_t foreground_pid = 0;
pid_t background_pids[HISTORY_SIZE];
int background_count = 0;

void setup(char inputBuffer[], char *args[], int *background);

// Function to add a command to history
void add_to_history(const char *command) {
    strncpy(history[history_count % HISTORY_SIZE], command, MAX_LINE - 1);
    history[history_count % HISTORY_SIZE][MAX_LINE - 1] = '\0'; // Ensure null termination
    history_count++;
}

// Function to print history
void print_history() {
    int start = (history_count > HISTORY_SIZE) ? history_count - HISTORY_SIZE : 0;
    int end = history_count;

    for (int i = end - 1; i >= start; i--) {
        printf("%d %s\n", end - i - 1, history[i % HISTORY_SIZE]);
    }
}


// Signal handler for ^Z (SIGTSTP)
void sigtstp_handler(int signum) {
    if (foreground_pid > 0) {
        kill(foreground_pid, SIGKILL);
        foreground_pid = 0;
        printf("Foreground process terminated.\n");
    }
}

// Function to execute a command from history
void execute_history_command(int index, char inputBuffer[], char *args[], int *background) {
    int start = (history_count > HISTORY_SIZE) ? history_count - HISTORY_SIZE : 0;
    if (index < 0 || index >= HISTORY_SIZE || index >= (history_count - start)) {
        fprintf(stderr, "Invalid history index.\n");
        return;
    }

    // Copy the selected command to inputBuffer
    strcpy(inputBuffer, history[(start + index) % HISTORY_SIZE]);
    printf("%s\n", inputBuffer); // Print the executed command for clarity

    // Parse the command using setup
    setup(inputBuffer, args, background);

    // Add the executed command back to history
    add_to_history(inputBuffer);
}


// Function to bring a background process to the foreground
void bring_to_foreground(int pid) {
    for (int i = 0; i < background_count; i++) {
        if (background_pids[i] == pid) {
            foreground_pid = pid;
            waitpid(pid, NULL, 0);
            foreground_pid = 0;
            background_pids[i] = background_pids[--background_count];
            return;
        }
    }
    fprintf(stderr, "No such background process.\n");
}

// Function to handle the "exit" command
void handle_exit() {
    if (background_count > 0) {
        printf("There are still %d background processes running. Please terminate them before exiting.\n", background_count);
        return;
    }
    exit(0);
}



 
/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */

void setup(char inputBuffer[], char *args[], int *background) {
    int length, i, start, ct;
    ct = 0;

    length = read(STDIN_FILENO, inputBuffer, MAX_LINE);

    if (length == 0)
        exit(0); // ^d was entered
    if ((length < 0) && (errno != EINTR)) {
        perror("error reading the command");
        exit(-1);
    }

    start = -1;
    *background = 0;
    for (i = 0; i < length; i++) {
        switch (inputBuffer[i]) {
            case ' ':
            case '\t': // Argument separators
                if (start != -1) {
                    args[ct] = &inputBuffer[start];
                    ct++;
                }
                start = -1;
                break;

            case '\n': // Final character
                if (start != -1) {
                    args[ct] = &inputBuffer[start];
                    ct++;
                }
                inputBuffer[i] = '\0'; // Null-terminate the input
                args[ct] = NULL;       // End of arguments
                break;

            default: // Other characters
                if (start == -1){
                    start = i;
                }
                if (inputBuffer[i] == '&') {
                    *background = 1;
                    inputBuffer[i - 1] = '\0';
                }
        }
    }
    args[ct] = NULL; // Safety null-termination
}



int main(void)
{
    char inputBuffer[MAX_LINE]; /* buffer to hold command entered */
    int background;            /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE / 2 + 1]; /* command line arguments */

    // Set up signal handler for ^Z
    signal(SIGTSTP, sigtstp_handler);

while (1) {
    background = 0;
    printf("myshell> ");
    fflush(stdout);

    setup(inputBuffer, args, &background); // Read and parse user input

    // Make a copy of the input before setup modifies it


    // Add the full command to history (using the copy)
    if (args[0] != NULL && strcmp(args[0], "history") != 0) {
        add_to_history(inputBuffer);
    }

    if (args[0] == NULL) {
        continue; // Empty command
    }

    // Handle "history" and other internal commands
    if (strcmp(args[0], "history") == 0) {
        print_history();
        continue;
    }

    pid_t pid = fork(); // Create a new process

    if (pid < 0) {
        perror("Fork failed");
        continue;
    }

    if (pid == 0) {
        // Child process: Execute the command
        char *pathEnv = getenv("PATH");
        if (!pathEnv) {
            fprintf(stderr, "Error: PATH environment variable not set.\n");
            exit(1);
        }

        char *pathDir = strtok(pathEnv, ":");
        while (pathDir != NULL) {
            snprintf(commandPath, sizeof(commandPath), "%s/%s", pathDir, args[0]);
            if (access(commandPath, X_OK) == 0) {
                execv(commandPath, args);
                perror("execv failed");
                exit(1);
            }
            pathDir = strtok(NULL, ":");
        }
        fprintf(stderr, "Command not found: %s\n", args[0]);
        exit(1);
    } else {
        // Parent process
        if (background == 0) {
            foreground_pid = pid;
            waitpid(pid, NULL, 0); // Wait for the child to finish
            foreground_pid = 0;
        } else {
            printf("Background process started with PID %d\n", pid);
            background_pids[background_count++] = pid;
        }
    }
}

}