#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>

#define MAX_LINE 80 /* 80 chars per line, per command */
#define HISTORY_SIZE 10

char history[HISTORY_SIZE][MAX_LINE];
int history_count = 0;
char commandPath[512];
pid_t foreground_pid = 0;
pid_t background_pids[HISTORY_SIZE];
int background_count = 0;

void setup(char inputBuffer[], char *args[], int *background);
void addToHistory(char *args[]);
void printHistory();
void historyCommand(int index, char inputBuffer[], char *args[], int *background);
void moveBackgroundToForeground(int pid);
void exitRequest();
void handleRedirection(char *args[]);
void terminateRunningProcess(int signum);
int countWords(const char *buffer);


/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */

void setup(char inputBuffer[], char *args[], int *background) {
    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */

    ct = 0;

    /* read what the user enters on the command line */
    
    length = read(STDIN_FILENO, inputBuffer, MAX_LINE);
    
    /* 0 is the system predefined file descriptor for stdin (standard input),
       which is the user's screen in this case. inputBuffer by itself is the
       same as &inputBuffer[0], i.e. the starting address of where to store
       the command that is read, and length holds the number of characters
       read in. inputBuffer is not a null terminated C-string. */

    if (length == 0){
        exit(0);  /* ^d was entered, end of user command stream */
    }

    /* the signal interrupted the read system call */
    /* if the process is in the read() system call, read returns -1
    However, if this occurs, errno is set to EINTR. We can check this  value
    and disregard the -1 value */

    if ((length < 0) && (errno != EINTR)) {
        perror("Error reading command");
        exit(-1);  /* terminate with error code of -1 */
    }

    start = -1;
    *background = 0;

    for (i = 0; i < length; i++) { /* examine every character in the inputBuffer */

        switch (inputBuffer[i]) {
            case ' ':
            case '\t':                                  /* argument separators */
                if (start != -1) {
                    args[ct] = &inputBuffer[start];     /* set up pointer */
                    ct++;
                }
                inputBuffer[i] = '\0';                  /* add a null char; make a C string */
                start = -1;
                break;
            case '\n':                                  /* should be the final char examined */
                if (start != -1) {
                    args[ct] = &inputBuffer[start];
                    ct++;
                }
                inputBuffer[i] = '\0';
                args[ct] = NULL;                        /* no more arguments to this command */
                break;
            default:                                    /* some other character */
                if (start == -1) start = i;

                if (inputBuffer[i] == '&') {
                    *background = 1;
                    inputBuffer[i] = '\0';
                }
        }  /* end of switch */
    }
    args[ct] = NULL;  /* just in case the input line was > 80 */
    //for (i = 0; i <= ct; i++)
		//printf("args %d = %s\n",i,args[i]);
} /* end of setup routine */


int countWords(const char *buffer) {
    int in_word = 0;   // Flag to track if we are inside a word
    int word_count = 0;

    // Traverse each character in the buffer
    while (*buffer != '\0') {
        if (isspace((unsigned char)*buffer)) {
            // If current character is whitespace, we're not in a word
            in_word = 0;
        } else if (!in_word) {
            // If we encounter a non-whitespace character and we're not in a word, start a new word
            in_word = 1;
            word_count++;
        }
        buffer++; // Move to the next character
    }

    return word_count;
}

void addToHistory(char *args[]) {
    char command[MAX_LINE] = ""; // Buffer to store the concatenated command
    int i = 0;

    // Concatenate arguments into a single command string
    while (args[i] != NULL) {
        strcat(command, args[i]);
        strcat(command, " "); // Add a space between arguments
        i++;
    }

    // Remove the trailing space
    if (strlen(command) > 0) {
        command[strlen(command) - 1] = '\0';
    }

    // Add the command to history
    strncpy(history[history_count % HISTORY_SIZE], command, MAX_LINE - 1);
    history[history_count % HISTORY_SIZE][MAX_LINE - 1] = '\0'; // Ensure null-termination
    history_count++;
}

void addToHistoryForHistoryCommand(char args[]) {
    // Ensure the input is not empty
    if (args == NULL || strlen(args) == 0) {
        return;
    }

    // Add the command to the history
    strncpy(history[history_count % HISTORY_SIZE], args, MAX_LINE - 1);

    // Null-terminate the string to prevent overflow
    history[history_count % HISTORY_SIZE][MAX_LINE - 1] = '\0';

    // Increment the history count
    history_count++;
}



void printHistory() {
    int start = (history_count > HISTORY_SIZE) ? history_count - HISTORY_SIZE : 0;
    int end = history_count;
    for (int i = history_count - 1; i >= start; i--) {
        printf("%d %s\n", end - i - 1, history[i % HISTORY_SIZE]);
    }
}

void historyCommand(int index, char inputBuffer[], char *args[], int *background) {
    int start = (history_count > HISTORY_SIZE) ? history_count - HISTORY_SIZE : 0;
    int end = history_count;

    // Invert the index to fetch the correct command
    int effective_index = history_count - 1 - index;

    if (effective_index < start || effective_index >= end) {
        fprintf(stderr, "Invalid history index.\n");
        return;
    }

    // Retrieve the command from history
    

    printf("Executing command from history: %s\n", history[effective_index % HISTORY_SIZE]);
    addToHistoryForHistoryCommand(history[effective_index % HISTORY_SIZE]);
    
    //setup(history[effective_index % HISTORY_SIZE], NULL, background);
    
}


// fg %num
void moveBackgroundToForeground(int pid) {
    for (int i = 0; i < background_count; i++) {
        if (background_pids[i] == pid) {
            foreground_pid = pid;  // Set as the foreground process
            waitpid(pid, NULL, 0);  // Wait for the process to finish
            foreground_pid = 0;  // Reset the foreground PID
            background_count--;
            background_pids[i] = background_pids[background_count];  // Remove from background list
            return;
        }
    }
    fprintf(stderr, "No such background process.\n");
}


void exitRequest() {
    if (background_count > 0) {
        printf("There are %d background processes running.\n", background_count);
        for (int i = 0; i < background_count; i++) {
            printf("PID: %d\n", background_pids[i]);
        }
        printf("Do you want to terminate all background processes and exit? (y/n): ");
        char choice = getchar();
        if (choice == 'y' || choice == 'Y') {
            for (int i = 0; i < background_count; i++) {
                kill(background_pids[i], SIGKILL);
            }
            background_count = 0;
            exit(0);
        }
        return;
    }
    else{
        exit(0);
    }
}


void handleRedirection(char *args[]) {
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

//^z
void terminateRunningProcess(int signum) {
    if (foreground_pid > 0) {
        // Send SIGKILL to the process group
        kill(-foreground_pid, SIGKILL);  // Negative PID sends to the process group
        printf("Foreground process %d and its group terminated.\n", foreground_pid);
        foreground_pid = 0;
    }
}


int main(void) {

    char inputBuffer[MAX_LINE];    /*buffer to hold command entered */
    int background; /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE / 2 + 1]; /*command line arguments */
    
    signal(SIGTSTP, terminateRunningProcess);

    while (1) {
        background = 0;
        printf("myshell> ");
        fflush(stdout);

        /*setup() calls exit() when Control-D is entered */
        setup(inputBuffer, args, &background);


        /** the steps are:
        (1) fork a child process using fork()
        (2) the child process will invoke execv()
        (3) if background == 0, the parent will wait,
        otherwise it will invoke the setup() function again. */




        if (args[0] == NULL) continue;
        
        if (args[0] != NULL && strcmp(args[0], "history") != 0) {
        addToHistory(args);
        }

        if (strcmp(args[0], "history") == 0) {
            if (args[1] && strcmp(args[1], "-i") == 0) {
                if (args[2]) {
                    int index = atoi(args[2]);
                    historyCommand(index, inputBuffer, args, &background);
                    continue;
                } else {
                    fprintf(stderr, "Usage: history -i <index>\n");
                }
            } else {
                printHistory();
            }
            continue;
        } else if (strcmp(args[0], "exit") == 0) {
            exitRequest();
            continue;
        } else if (strcmp(args[0], "fg") == 0) {
            if (args[1] && args[1][0] == '%') {
                int pid = atoi(&args[1][1]);
                moveBackgroundToForeground(pid);
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
            handleRedirection(args);
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
