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

void handleCommand(struct command cmd, int *status)
{
    if (strcmp(cmd.name, "exit") == 0)
    {
        exit(0); // terminate the shell
    }
    else if (strcmp(cmd.name, "cd") == 0)
    {
        // change directory
        if (cmd.arguments[1] == NULL)
        {
            chdir(getenv("HOME"));
        }
        else
        {
            if (chdir(cmd.arguments[1]) == -1)
            {
                perror("chdir");
            }
        }
    }
    else if (strcmp(cmd.name, "status") == 0)
    {
        // print the exit status or signal of the last foreground process
        if (WIFEXITED(*status))
        {
            printf("exit value %d\n", WEXITSTATUS(*status));
        }
        else if (WIFSIGNALED(*status))
        {
            printf("terminated by signal %d\n", WTERMSIG(*status));
        }
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
            // redirect input from a file, if specified
            if (cmd.input_file != NULL)
            {
                int input_fd = open(cmd.input_file, O_RDONLY);
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
            else if (cmd.background == 1)
            {
                // If the user doesn't redirect the standard input for a background command, then standard input should be redirected to /dev/null
                int input_fd = open("/dev/null", O_RDONLY);
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
            // redirect output to a file, if specified
            if (cmd.output_file != NULL)
            {
                int output_fd = open(cmd.output_file, O_WRONLY | O_CREAT | O_TRUNC, 0664);
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
            else if (cmd.background == 1)
            {
                // If the user doesn't redirect the standard output for a background command, then standard output should be redirected to /dev/null
                int output_fd = open("/dev/null", O_WRONLY);
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
            // run the command
            *status = 0;
            execvp(cmd.name, cmd.arguments);

            perror(cmd.name); // this only runs if execvp fails
            *status = 1;
            exit(1);
        }
        else
        {
            // parent process
            if (cmd.background == 0)
            {
                // wait for the child process to finish, if not a background process
                pid_t wpid = waitpid(pid, status, 0);
                if (wpid == -1)
                {
                    perror("waitpid");
                    *status = 1;
                    exit(1);
                }
            }
            else
            {
                printf("background pid is %d\n", pid);
                background_processes[num_background_processes] = pid; // add the background process to the list
                num_background_processes++;                           // increment the number of background processes
            }

            // check if any background processes have completed
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
                        printf("background pid %d is done: terminated value %d\n", background_processes[i], *status);
                    }
                    else
                    {
                        printf("background pid %d is done: exit value %d\n", background_processes[i], WEXITSTATUS(*status));
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
        // Get the command from the user
        command_line = getCommand();

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

    return 0;
}
