#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ALLOWED_CHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZ "

int main(int argc, char *argv[])
{
    // Check the number of arguments
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s keylength\n", argv[0]);
        exit(1);
    }

    // Get the key length
    int key_length = atoi(argv[1]);

    // Seed the random number generator
    srand(time(NULL));

    // Generate the key and print it to stdout
    int i;
    for (i = 0; i < key_length; i++)
    {
        char random_char = ALLOWED_CHARS[rand() % 27];
        printf("%c", random_char);
    }
    printf("\n");

    return 0;
}
