#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_COMMAND_LENGTH 2048
#define MAX_ARGUMENTS 512

struct command
{
    char *name;
    char *arguments[MAX_ARGUMENTS];
    char *input_file;
    char *output_file;
    int background;
};

// function to read a command from the user
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

void print_command(struct command cmd)
{
    printf("name: %s\n", cmd.name);
    printf("arguments: ");
    for (int i = 0; cmd.arguments[i] != NULL; i++)
    {
        printf("%s ", cmd.arguments[i]);
    }
    printf("\n");
    printf("input file: %s\n", cmd.input_file);
    printf("output file: %s\n", cmd.output_file);
    printf("background: %d\n", cmd.background);
}

int main()
{
    struct command cmd;
    char *token;
    int argument_count;

    while (1)
    {
        // Parse the command line into individual arguments and set the command struct members
        argument_count = 0;
        token = strtok(getCommand(), " \n");
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

        // Print out the contents of the command struct
        print_command(cmd);
    }

    return 0;
}
