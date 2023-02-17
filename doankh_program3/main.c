#define _POSIX_SOURCE
#define SA_RESTART 0

// Error handling libraries
#include <errno.h>
#include <poll.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_COMMAND_LENGTH 2048
#define MAX_ARGUMENTS 512
#define MAX_BG_PROCESSES 100

struct command
{
    char *name;
    char *arguments[MAX_ARGUMENTS];
    char *input_file;
    char *output_file;
    int background;
};

char *getCommand()
{
    // allocate memory for the command
    char *command = calloc(MAX_COMMAND_LENGTH, sizeof(char));

    // print the prompt and read the command
    printf(": ");
    fflush(stdout);
    fgets(command, MAX_COMMAND_LENGTH, stdin);

    // remove the newline character from the end of the command
    command[strcspn(command, "\n")] = '\0';

    // if the command is a comment or a blank line, ignore it and read another command
    if (command[0] == '#' || strlen(command) == 0)
    {
        free(command);
        return NULL;
    }

    return command;
}

char *expandVariable(char *command)
{
    char *pid_string = calloc(16, sizeof(char));
    pid_t pid = getpid();
    sprintf(pid_string, "%d", pid);
    char *result = calloc(MAX_COMMAND_LENGTH, sizeof(char));
    char *pos;
    // Expanding the string indefinitely, because we aren't sure how long the pid will be
    while ((pos = strstr(command, "$$")) != NULL)
    {
        int offset = pos - command;
        strncpy(result, command, offset);
        strcat(result, pid_string);
        strcat(result, command + offset + 2);
        strcpy(command, result);
    }
    free(result);
    free(pid_string);
    return command;
}

pid_t background_processes[MAX_BG_PROCESSES]; // MAX_BG_PROCESSES is a macro that defines the maximum number of background processes that can be running at any given time
int num_background_processes = 0;             // keeps track of the number of background processes currently running
// We can just directly access the array of background processes, no need to use pointers

void changeDirectory(char *path)
{
    if (path == NULL)
    {
        chdir(getenv("HOME"));
    }
    else
    {
        if (chdir(path) == -1)
        {
            perror("chdir");
        }
    }
}

void printStatus(int status)
{
    if (WIFEXITED(status))
    {
        printf("exit value %d\n", WEXITSTATUS(status));
    }
    else if (WIFSIGNALED(status))
    {
        printf("terminated by signal %d\n", WTERMSIG(status));
    }
}

void executeCommand(struct command cmd, int *status)
{
    *status = 0;
    execvp(cmd.name, cmd.arguments);
    perror(cmd.name);
    *status = 1;
    exit(1);
}

void redirectInput(char *inputFile, int *status)
{
    int input_fd = open(inputFile, O_RDONLY);
    if (input_fd == -1)
    {
        perror("open");
        *status = 1;
        exit(1);
    }
    if (dup2(input_fd, STDIN_FILENO) == -1)
    {
        perror("dup2");
        *status = 1;
        exit(1);
    }
    close(input_fd);
}

void redirectOutput(char *outputFile, int *status)
{
    int output_fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0664);
    if (output_fd == -1)
    {
        perror("open");
        *status = 1;
        exit(1);
    }
    if (dup2(output_fd, STDOUT_FILENO) == -1)
    {
        perror("dup2");
        *status = 1;
        exit(1);
    }
    close(output_fd);
}

void executeCommandWithRedirection(struct command cmd, int *status)
{
    if (cmd.input_file != NULL)
    {
        redirectInput(cmd.input_file, status);
    }
    else if (cmd.background == 1)
    {
        redirectInput("/dev/null", status);
    }

    if (cmd.output_file != NULL)
    {
        redirectOutput(cmd.output_file, status);
    }
    else if (cmd.background == 1)
    {
        redirectOutput("/dev/null", status);
    }

    executeCommand(cmd, status);
}

void checkBackgroundProcess(int *status)
{
    int i = 0;
    while (i < num_background_processes)
    {
        pid_t wpid = waitpid(background_processes[i], status, WNOHANG);
        if (wpid == -1)
        {
            perror("waitpid");
            *status = 1;
            exit(1);
        }
        else if (wpid != 0)
        {
            if (WIFSIGNALED(*status))
            {
                // if the child process was terminated by a signal, save the signal to status
                *status = WTERMSIG(*status);
                char buf[255];
                int n = sprintf(buf, "background pid %d is done: terminated value %d\n", background_processes[i], *status);
                write(STDOUT_FILENO, buf, n);
                num_background_processes--; // decrement the number of background processes
                for (int j = i; j < num_background_processes; j++)
                {
                    background_processes[j] = background_processes[j + 1]; // shift remaining background processes down
                }
            }
            else
            {
                char buf[255];
                int n = sprintf(buf, "background pid %d is done: exit value %d\n", background_processes[i], WEXITSTATUS(*status));
                write(STDOUT_FILENO, buf, n);
                num_background_processes--; // decrement the number of background processes
                for (int j = i; j < num_background_processes; j++)
                {
                    background_processes[j] = background_processes[j + 1]; // shift remaining background processes down
                }
            }
        }
        else
        {
            i++;
        }
    }
}

// This variable is used to check if background process is enabled
// The reason why we need volatile keyword is because when a signal is fired
// there is no way to indicate whether the variable value is changing or not in the code
// so we need constantly load the variable. The volatile keyword indicate that the value of this variable can change anytime
// whenever a signal is fired, not following the code surrounding it

volatile sig_atomic_t foreground_only = 0;

void handle_SIGTSTP(int signo)
{
    if (foreground_only == 0)
    {
        char *msg = "\nEntering foreground-only mode (& is now ignored)\n";
        write(STDOUT_FILENO, msg, 50);
        foreground_only = 1;
    }
    else
    {
        char *msg = "\nExiting foreground-only mode\n";
        write(STDOUT_FILENO, msg, 31);
        foreground_only = 0;
    }
}

volatile int handleCommand(struct command cmd, int *status)
{
    if (strcmp(cmd.name, "exit") == 0)
    {
        exit(0); // terminate the shell
    }
    else if (strcmp(cmd.name, "cd") == 0)
    {
        // change directory
        changeDirectory(cmd.arguments[1]);
    }
    else if (strcmp(cmd.name, "status") == 0)
    {
        // print the exit status or signal of the last foreground process
        printStatus(*status);
    }
    else
    {
        // ignore SIGINT in the parent process
        signal(SIGINT, SIG_IGN);
        // register handler for SIGTSTP signal
        struct sigaction SIGTSTP_action = {0};
        SIGTSTP_action.sa_handler = handle_SIGTSTP;
        sigfillset(&SIGTSTP_action.sa_mask);
        SIGTSTP_action.sa_flags = SA_RESTART;
        sigaction(SIGTSTP, &SIGTSTP_action, NULL);

        pid_t pid = fork();
        if (pid == -1)
        {
            perror("fork");
            *status = 1;
            exit(1);
        }
        else if (pid == 0)
        {
            // ignore SIGINT in the child process
            signal(SIGINT, SIG_IGN);
            if (foreground_only == 1)
                cmd.background = 0;

            if (cmd.background == 0)
            {
                // if running in foreground, handle SIGINT with default action
                signal(SIGINT, SIG_DFL);
                // ignore SIGTSTP in the foreground child process
                signal(SIGTSTP, SIG_IGN);
            }
            executeCommandWithRedirection(cmd, status);
        }
        else
        {
            // parent process
            if (cmd.background == 0 || foreground_only == 1)
            {
                // wait for the child process to finish, if not a background process or in foreground-only mode
                int child_status;
                pid_t wpid = waitpid(pid, &child_status, 0);
                signal(SIGTSTP, handle_SIGTSTP); // Handle SIGTSTP

                *status = child_status;
                // print signal number if the child was terminated by a signal
                if (WIFSIGNALED(child_status))
                {
                    char buf[256];
                    sprintf(buf, "Terminated by signal %d\n", WTERMSIG(child_status));
                    write(STDOUT_FILENO, buf, strlen(buf));
                }
            }
            else if (foreground_only != 1)
            {
                printf("background pid is %d\n", pid);
                background_processes[num_background_processes] = pid; // add the background process to the list
                num_background_processes++;                           // increment the number of background processes
            }

            // check if any background processes have completed
            checkBackgroundProcess(status);
        }
    }

    return foreground_only;
}

int status = 0; // Global status keeps track of the recent exit status -> read and write status to memory slot

int main()
{
    char *command_line;
    struct command cmd;
    char *token;
    int argument_count;

    while (1)
    {
        signal(SIGINT, SIG_IGN);         // Ignore SIGINT in the main shell
        signal(SIGTSTP, handle_SIGTSTP); // Handle SIGTSTP

        // Get the command from the user
        command_line = getCommand();
        if (command_line != NULL)
        {
            // Expand any instances of "$$" in the command
            command_line = expandVariable(command_line);

            // Initialize the command struct
            memset(&cmd, 0, sizeof(struct command));

            // Parse the command line into individual arguments and set the command struct members
            argument_count = 0;
            token = strtok(command_line, " \n");
            while (token != NULL && argument_count < MAX_ARGUMENTS - 1)
            {
                if (strcmp(token, "<") == 0)
                {
                    // Input redirection
                    token = strtok(NULL, " \n");
                    cmd.input_file = token;
                }
                else if (strcmp(token, ">") == 0)
                {
                    // Output redirection
                    token = strtok(NULL, " \n");
                    cmd.output_file = token;
                }
                else if (strcmp(token, "&") == 0)
                {
                    // Background process
                    cmd.background = 1;
                }
                else
                {
                    // Command or argument
                    if (argument_count == 0)
                    {
                        cmd.name = token;
                    }
                    cmd.arguments[argument_count] = token;
                    argument_count++;
                }
                token = strtok(NULL, " \n");
            }
            cmd.arguments[argument_count] = NULL;

            // Execute the command
            int current = handleCommand(cmd, &status);

            printf("%d\n", current);
            // Free the memory allocated for the command
            free(command_line);
        }
        else
            continue;
    }

    return 0;
}
