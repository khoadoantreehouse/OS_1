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

volatile int foreground_only_mode = 0; // This variable is used to check if background process is enabled
// The reason why we need volatile keyword is because when a signal is fired
// there is no way to indicate whether the variable value is changing or not in the code
// so we need constantly load the variable. The volatile keyword indicate that the value of this variable can change anytime
// whenever a signal is fired, not following the code surrounding it
int foreground_process_id;

void handleSIGTSTP(int signo)
{
    if (foreground_process_id != -1) // if there is a foreground process running
    {
        char *message = "Entering foreground-only mode (& is now ignored)\n";
        write(STDOUT_FILENO, message, 49); // display informative message
        fflush(stdout);
        foreground_only_mode = 1;             // set foreground only mode
        kill(foreground_process_id, SIGTERM); // terminate the foreground process
        foreground_process_id = -1;           // reset foreground process id
    }
    else
    {
        char *message = "Exiting foreground-only mode\n";
        write(STDOUT_FILENO, message, 29); // display informative message
        fflush(stdout);
        foreground_only_mode = 0; // unset foreground only mode
    }
}

void handleCommand(struct command cmd, int *status)
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
        pid_t pid = fork();
        if (pid == -1)
        {
            perror("fork");
            *status = 1;
            exit(1);
        }
        else if (pid == 0)
        {
            // child process
            struct sigaction act_child;
            sigemptyset(&act_child.sa_mask);
            act_child.sa_flags = 0;

            // ignore SIGINT and SIGTSTP signals
            act_child.sa_handler = SIG_IGN;
            if (sigaction(SIGINT, &act_child, NULL) == -1)
            {
                perror("sigaction");
                *status = 1;
                exit(1);
            }
            if (sigaction(SIGTSTP, &act_child, NULL) == -1)
            {
                perror("sigaction");
                *status = 1;
                exit(1);
            }

            executeCommandWithRedirection(cmd, status);
            // run the command
            executeCommand(cmd, status);
        }
        else
        {
            // parent process
            struct sigaction act_parent;
            sigemptyset(&act_parent.sa_mask);
            act_parent.sa_flags = 0;

            // ignore SIGTSTP signal
            act_parent.sa_handler = handleSIGTSTP;
            if (sigaction(SIGTSTP, &act_parent, NULL) == -1)
            {
                perror("sigaction");
                *status = 1;
                exit(1);
            }

            if (cmd.background == 0)
            {
                // foreground process - wait for child to finish
                foreground_process_id = pid;
                int status_temp = 0;
                pid_t wpid;
                do
                {
                    wpid = waitpid(pid, &status_temp, WUNTRACED);
                } while (!WIFEXITED(status_temp) && !WIFSIGNALED(status_temp) && !WIFSTOPPED(status_temp));
                foreground_process_id = 0;

                if (WIFSIGNALED(status_temp))
                {
                    // print signal number if the child was terminated by a signal
                    char signal_str[50];
                    sprintf(signal_str, "terminated by signal %d\n", WTERMSIG(status_temp));
                    write(STDOUT_FILENO, signal_str, strlen(signal_str));
                }

                *status = WEXITSTATUS(status_temp);
            }
            else
            {
                // background process - add to list and print message
                background_processes[num_background_processes] = pid;
                num_background_processes++;
                char bg_str[50];
                sprintf(bg_str, "background pid is %d\n", pid);
                write(STDOUT_FILENO, bg_str, strlen(bg_str));
            }

            // check if any background processes have completed
            checkBackgroundProcess(status);
        }
    }
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
        signal(SIGINT, SIG_IGN);  // Ignore SIGINT in the main shell
        signal(SIGTSTP, SIG_IGN); // Ignore SIGTSTP in the main shell
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
            handleCommand(cmd, &status);

            // Free the memory allocated for the command
            free(command_line);
        }
        else
            continue;
    }

    return 0;
}
