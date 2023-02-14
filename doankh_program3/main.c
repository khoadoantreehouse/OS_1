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

int main()
{
    char command_line[MAX_COMMAND_LENGTH];
    struct command cmd;
    char *token;
    int argument_count;

    while (1)
    {
        // Print the command prompt
        printf(": ");
        fflush(stdout);

        // Read in the command line
        if (fgets(command_line, MAX_COMMAND_LENGTH, stdin) == NULL)
        {
            // Exit if an error occurs while reading input
            exit(1);
        }

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

        // TODO: Handle the parsed command using the command struct
    }

    return 0;
}
