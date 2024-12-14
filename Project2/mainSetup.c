#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
 
#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define HISTORY_SIZE 10


// Globals for managing history and foreground process
char history[HISTORY_SIZE][MAX_LINE];
int history_count = 0;
char commandPath[512]; /* Buffer to store command path */
pid_t foreground_pid = 0;
pid_t background_pids[HISTORY_SIZE];
int background_count = 0;

void setup(char inputBuffer[], char *args[],int *background);

// Function to add a command to history
void add_to_history(const char *command) {
    strncpy(history[history_count % HISTORY_SIZE], command, MAX_LINE - 1);
    history[history_count % HISTORY_SIZE][MAX_LINE - 1] = '\0'; // Ensure null termination
    history_count++;
}

// Function to print history
void print_history() {
    int start = (history_count > HISTORY_SIZE) ? history_count - HISTORY_SIZE : 0;
    for (int i = start; i < history_count; i++) {
        printf("%d %s\n", i - start, history[i % HISTORY_SIZE]);
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
    if (index < 0 || index >= HISTORY_SIZE || index >= history_count) {
        fprintf(stderr, "Invalid history index.\n");
        return;
    }
    strcpy(inputBuffer, history[index]);
    setup(inputBuffer, args, background);
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

void setup(char inputBuffer[], char *args[],int *background)
{
    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */
    
    ct = 0;
        
    /* read what the user enters on the command line */
    length = read(STDIN_FILENO,inputBuffer,MAX_LINE);  

    /* 0 is the system predefined file descriptor for stdin (standard input),
       which is the user's screen in this case. inputBuffer by itself is the
       same as &inputBuffer[0], i.e. the starting address of where to store
       the command that is read, and length holds the number of characters
       read in. inputBuffer is not a null terminated C-string. */

    start = -1;
    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */

/* the signal interrupted the read system call */
/* if the process is in the read() system call, read returns -1
  However, if this occurs, errno is set to EINTR. We can check this  value
  and disregard the -1 value */
    if ( (length < 0) && (errno != EINTR) ) {
        perror("error reading the command");
	exit(-1);           /* terminate with error code of -1 */
    }

    printf(">>%s<<",inputBuffer);
    for (i=0;i<length;i++){ /* examine every character in the inputBuffer */

        switch (inputBuffer[i]){
	    case ' ':
	    case '\t' :               /* argument separators */
		if(start != -1){
                    args[ct] = &inputBuffer[start];    /* set up pointer */
		    ct++;
		}
                inputBuffer[i] = '\0'; /* add a null char; make a C string */
		start = -1;
		break;

            case '\n':                 /* should be the final char examined */
		if (start != -1){
            args[ct] = &inputBuffer[start];     
		    ct++;
		}
                inputBuffer[i] = '\0';
                args[ct] = NULL; /* no more arguments to this command */
		break;

	    default :             /* some other character */
		if (start == -1)
		    start = i;
                if (inputBuffer[i] == '&'){
		            *background  = 1;
                    inputBuffer[i-1] = '\0';
                }
	} /* end of switch */
     }    /* end of for */
     args[ct] = NULL; /* just in case the input line was > 80 */

	//for (i = 0; i <= ct; i++)
		//printf("args %d = %s\n",i,args[i]);
} /* end of setup routine */


int main(void)
{
    char inputBuffer[MAX_LINE]; /*buffer to hold command entered */
    int background;            /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE / 2 + 1]; /* command line arguments */

    // Set up signal handler for ^Z
    signal(SIGTSTP, sigtstp_handler);

    while (1) {
        background = 0;
        printf("myshell: ");
        fflush(stdout);

        /* setup() calls exit() when Control-D is entered */
        setup(inputBuffer, args, &background); // read user input

        /** the steps are:
                        (1) fork a child process using fork()
                        (2) the child process will invoke execv()
						(3) if background == 0, the parent will wait,
                        otherwise it will invoke the setup() function again. */


        if (args[0] == NULL) {
            continue; /* empty command */
        }


                // Add the command to history
        if (args[0] != NULL && strcmp(args[0], "history") != 0) {
            add_to_history(inputBuffer);
        }



        // Check for internal commands
        if (strcmp(args[0], "history") == 0) {
            if (args[1] != NULL && strcmp(args[1], "-i") == 0) {
                if (args[2] != NULL) {
                    int index = atoi(args[2]);
                    execute_history_command(index, inputBuffer, args, &background);
                } else {
                    fprintf(stderr, "Usage: history -i <index>\n");
                }
            } else {
                print_history();
            }
            continue;
        } 
        else if (strcmp(args[0], "exit") == 0) {
            handle_exit();
            continue;
        } 
        else if (strcmp(args[0], "fg") == 0) {
            if (args[1] != NULL && args[1][0] == '%') {
                int pid = atoi(&args[1][1]);
                bring_to_foreground(pid);
            } else {
                fprintf(stderr, "Usage: fg %%<pid>\n");
            }
            continue;
        }



        pid_t pid = fork(); /* create a new process */

        if (pid < 0) {
            perror("Fork failed");
            continue;
        }
        if (pid == 0) {
            /* Child process: Execute the command */

            /* Get the PATH environment variable */
            char *pathEnv = getenv("PATH");
            
            if (!pathEnv) {
                fprintf(stderr, "Error: PATH environment variable not set.\n");
                exit(1);
            }

            /* Tokenize PATH and search for the command */
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
                    // Foreground process
                    foreground_pid = pid;
                    waitpid(pid, NULL, 0);
                    foreground_pid = 0;
                } else {
                    // Background process
                    printf("Background process started with PID %d\n", pid);
                    background_pids[background_count++] = pid;
                }
        }
    

    }
}
