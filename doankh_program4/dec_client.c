#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFFER_SIZE 2048

// Function to check if a given file contains only valid characters
// Returns 0 if the file is valid, otherwise returns 1
int validate_file(char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        fprintf(stderr, "Error: could not open file %s\n", filename);
        return 1;
    }
    int c;
    while ((c = fgetc(file)) != EOF)
    {
        if (c != ' ' && c != '\n' && (c < 'A' || c > 'Z'))
        {
            fclose(file);
            fprintf(stderr, "Error: invalid character in file %s\n", filename);
            return 1;
        }
    }
    fclose(file);
    return 0;
}

/// Function to check if the key is long enough for the given plaintext
// Returns 0 if the key is long enough, otherwise returns 1
int validate_key_length(char *plaintext_filename, char *key_filename)
{
    FILE *plaintext_file = fopen(plaintext_filename, "r");
    FILE *key_file = fopen(key_filename, "r");
    if (!plaintext_file || !key_file)
    {
        fprintf(stderr, "Error: could not open file\n");
        return 1;
    }
    int plaintext_size = 0, key_size = 0;
    char plaintext_buffer[BUFFER_SIZE];
    char key_buffer[BUFFER_SIZE];
    memset(plaintext_buffer, 0, BUFFER_SIZE);
    memset(key_buffer, 0, BUFFER_SIZE);
    int n;
    while ((n = fread(plaintext_buffer, 1, BUFFER_SIZE, plaintext_file)) > 0)
    {
        for (int i = 0; i < n; i++)
        {
            if (plaintext_buffer[i] != ' ')
                plaintext_size++;
        }
        memset(plaintext_buffer, 0, BUFFER_SIZE);
    }
    while ((n = fread(key_buffer, 1, BUFFER_SIZE, key_file)) > 0)
    {
        for (int i = 0; i < n; i++)
        {
            if (key_buffer[i] != ' ')
                key_size++;
        }
        memset(key_buffer, 0, BUFFER_SIZE);
    }
    fclose(plaintext_file);
    fclose(key_file);
    if (key_size < plaintext_size)
    {
        fprintf(stderr, "Error: key is too short\n");
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    // Check the number of arguments
    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s ciphertext key port\n", argv[0]);
        exit(1);
    }

    char *ciphertext = argv[1];
    char *key = argv[2];
    char *ptr;
    long port = strtol(argv[3], &ptr, 10);

    // Validate the input files

    if (validate_file(ciphertext) || validate_file(key) || validate_key_length(ciphertext, key))
    {
        exit(1);
    }

    // Create a socket and connect to the server
    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        fprintf(stderr, "Error: could not open socket\n");
        exit(2);
    }

    server = gethostbyname("localhost");
    if (server == NULL)
    {
        fprintf(stderr, "Error: could not find localhost\n");
        exit(2);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    serv_addr.sin_port = htons(port);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        fprintf(stderr, "Error: could not connect to server on port %d\n", port);
        exit(2);
    }

    int n = 0;
    // Send program name to server
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    sprintf(buffer, "%s", argv[0]);
    write(sockfd, buffer, strlen(buffer));
    memset(buffer, 0, BUFFER_SIZE);

    // Send cyphertext and key to server
    FILE *ciphertext_file2 = fopen(ciphertext, "r");
    while ((n = fread(buffer, 1, BUFFER_SIZE, ciphertext_file2)) > 0)
    {
        write(sockfd, buffer, n);
        memset(buffer, 0, BUFFER_SIZE);
    }
    fclose(ciphertext_file2);
    memset(buffer, 0, BUFFER_SIZE);
    sprintf(buffer, "");
    write(sockfd, buffer, strlen(buffer));
    memset(buffer, 0, BUFFER_SIZE);
    FILE *key_file2 = fopen(key, "r");
    while ((n = fread(buffer, 1, BUFFER_SIZE, key_file2)) > 0)
    {
        write(sockfd, buffer, n);
        memset(buffer, 0, BUFFER_SIZE);
    }
    fclose(key_file2);
    memset(buffer, 0, BUFFER_SIZE);
    sprintf(buffer, "\n");
    write(sockfd, buffer, strlen(buffer));
    memset(buffer, 0, BUFFER_SIZE);
    // Send newline character to server
    write(sockfd, "\n", 1);

    // Receive ciphertext from server
    char ciphertext_buffer[BUFFER_SIZE];
    memset(ciphertext_buffer, 0, BUFFER_SIZE);
    int ciphertext_len = 0;
    while ((n = read(sockfd, ciphertext_buffer + ciphertext_len, BUFFER_SIZE - ciphertext_len)) > 0)
    {
        ciphertext_len += n;
        if (ciphertext_len >= BUFFER_SIZE)
        {
            fprintf(stderr, "Error: ciphertext too long\n");
            exit(1);
        }
    }
    if (n < 0)
    {
        fprintf(stderr, "Error: could not receive ciphertext\n");
        exit(1);
    }

    // Write ciphertext to stdout
    fwrite(ciphertext_buffer, 1, ciphertext_len, stdout);

    close(sockfd);
    return 0;
}
