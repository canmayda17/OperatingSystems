#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE 80 /* 80 chars per line, per command */
#define HISTORY_SIZE 10

// Globals for managing history and foreground process
char history[HISTORY_SIZE][MAX_LINE];
int history_count = 0;
char commandPath[512];
pid_t foreground_pid = 0;
pid_t background_pids[HISTORY_SIZE];
int background_count = 0;

void setup(char inputBuffer[], char *args[], int *background);
void add_to_history(const char *command);
void print_history();
void execute_history_command(int index, char inputBuffer[], char *args[], int *background);
void bring_to_foreground(int pid);
void handle_exit();
void handle_redirection(char *args[]);
void sigtstp_handler(int signum);

void setup(char inputBuffer[], char *args[], int *background) {
    int length, i, start, ct;
    ct = 0;
    length = read(STDIN_FILENO, inputBuffer, MAX_LINE);
    
    if (length == 0) exit(0);

    if ((length < 0) && (errno != EINTR)) {
        perror("Error reading command");
        exit(-1);
    }



    start = -1;
    *background = 0;
    for (i = 0; i < length; i++) {
        switch (inputBuffer[i]) {
            case ' ':
            case '\t':
                if (start != -1) {
                    args[ct] = &inputBuffer[start];
                    ct++;
                }
                inputBuffer[i] = '\0';
                start = -1;
                break;
            case '\n':
                if (start != -1) {
                    args[ct] = &inputBuffer[start];
                    ct++;
                }
                inputBuffer[i] = '\0';
                args[ct] = NULL;
                break;
            default:
                if (start == -1) start = i;
                if (inputBuffer[i] == '&') {
                    *background = 1;
                    inputBuffer[i] = '\0';
                }
        }
    }
    args[ct] = NULL;
}

void add_to_history(const char *command) {
    strncpy(history[history_count % HISTORY_SIZE], command, MAX_LINE - 1);
    //history[history_count % HISTORY_SIZE][MAX_LINE - 1] = '\0';
    history_count++;
}

void print_history() {
    int start = (history_count > HISTORY_SIZE) ? history_count - HISTORY_SIZE : 0;
    int end = history_count;
    for (int i = history_count - 1; i >= start; i--) {
        printf("%d %s\n", end - i - 1, history[i % HISTORY_SIZE]);
    }
}

void execute_history_command(int index, char inputBuffer[], char *args[], int *background) {
    int start = (history_count > HISTORY_SIZE) ? history_count - HISTORY_SIZE : 0;
    int end = history_count;

    // Invert the index to fetch the correct command
    int effective_index = history_count - 1 - index;

    if (effective_index < start || effective_index >= end) {
        fprintf(stderr, "Invalid history index.\n");
        return;
    }

    // Retrieve the command from history
    char *selected_command = history[effective_index % HISTORY_SIZE];

    printf("Executing command from history: %s\n", selected_command);
    add_to_history(selected_command);

    setup(selected_command, args, background);
    
    
}



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

void handle_exit() {
    if (background_count > 0) {
        printf("There are %d background processes running.\n", background_count);
        for (int i = 0; i < background_count; i++) {
            printf("PID: %d\n", background_pids[i]);
        }
        return;
    }
    exit(0);
}

void handle_redirection(char *args[]) {
    int i = 0;

    while (args[i] != NULL) {
        if (strcmp(args[i], ">") == 0) {
            // Standard output redirection
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("Error opening file for output redirection");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            args[i] = NULL; // Remove redirection operators from args
        } else if (strcmp(args[i], ">>") == 0) {
            // Standard output append
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd < 0) {
                perror("Error opening file for output append");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            args[i] = NULL;
        } else if (strcmp(args[i], "<") == 0) {
            // Standard input redirection
            int fd = open(args[i + 1], O_RDONLY);
            if (fd < 0) {
                perror("Error opening file for input redirection");
                exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
            args[i] = NULL;
        } else if (strcmp(args[i], "2>") == 0) {
            // Standard error redirection
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("Error opening file for error redirection");
                exit(1);
            }
            dup2(fd, STDERR_FILENO);
            close(fd);
            args[i] = NULL;
        }
        i++;
    }
}

void sigtstp_handler(int signum) {
    if (foreground_pid > 0) {
        kill(foreground_pid, SIGKILL);
        printf("Foreground process %d terminated.\n", foreground_pid);
        foreground_pid = 0;
    }
}

int main(void) {
    char inputBuffer[MAX_LINE];
    char *args[MAX_LINE / 2 + 1];
    int background;

    signal(SIGTSTP, sigtstp_handler);

    while (1) {
        background = 0;
        printf("myshell> ");
        fflush(stdout);

        setup(inputBuffer, args, &background);

        if (args[0] == NULL) continue;
        
        if (args[0] != NULL && strcmp(args[0], "history") != 0) {
        add_to_history(inputBuffer);
        }

        if (strcmp(args[0], "history") == 0) {
            if (args[1] && strcmp(args[1], "-i") == 0) {
                if (args[2]) {
                    int index = atoi(args[2]);
                    execute_history_command(index, inputBuffer, args, &background);
                    continue;
                } else {
                    fprintf(stderr, "Usage: history -i <index>\n");
                }
            } else {
                print_history();
            }
            continue;
        } else if (strcmp(args[0], "exit") == 0) {
            handle_exit();
            continue;
        } else if (strcmp(args[0], "fg") == 0) {
            if (args[1] && args[1][0] == '%') {
                int pid = atoi(&args[1][1]);
                bring_to_foreground(pid);
            } else {
                fprintf(stderr, "Usage: fg %%<pid>\n");
            }
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            continue;
        }
        if (pid == 0) {
            handle_redirection(args);
            char *pathEnv = getenv("PATH");
            if (!pathEnv) {
                fprintf(stderr, "PATH not set\n");
                exit(1);
            }
            char *pathDir = strtok(pathEnv, ":");
            while (pathDir) {
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
            if (!background) {
                foreground_pid = pid;
                waitpid(pid, NULL, 0);
                foreground_pid = 0;
            } else {
                background_pids[background_count++] = pid;
                printf("Background process started with PID %d\n", pid);
            }
        }
    }
}
