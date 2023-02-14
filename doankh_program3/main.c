#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_COMMAND_LENGTH 2048
#define MAX_ARGUMENTS 512

int main()
{
    char command[MAX_COMMAND_LENGTH];
    char *arguments[MAX_ARGUMENTS];
    char *token;
    int argument_count;

    while (1)
    {
        // Print the command prompt
        printf(": ");

        // Read in the command line
        if (fgets(command, MAX_COMMAND_LENGTH, stdin) == NULL)
        {
            // Exit if an error occurs while reading input
            exit(1);
        }

        // Parse the command line into individual arguments
        argument_count = 0;
        token = strtok(command, " \n");
        while (token != NULL && argument_count < MAX_ARGUMENTS - 1)
        {
            arguments[argument_count] = token;
            argument_count++;
            token = strtok(NULL, " \n");
        }
        arguments[argument_count] = NULL;

        // TODO: Handle the parsed command and arguments here
    }

    return 0;
}
