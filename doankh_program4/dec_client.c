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
    int port = atoi(argv[3]);

    // Validate the input files
    // validate_file(ciphertext) || validate_file(key) ||
    if (validate_key_length(ciphertext, key))
    {
        exit(1);
    }
    printf("Here");

    // Create a socket and connect to the server
    int sockfd, n;
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

    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr_list[0], server->h_length);
    serv_addr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        fprintf(stderr, "Error: could not connect to server on port %d\n", port);
        exit(2);
    }

    // Check if connected to dec_server
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    strcpy(buffer, "dec_client");
    write(sockfd, buffer, strlen(buffer));
    memset(buffer, 0, BUFFER_SIZE);
    read(sockfd, buffer, BUFFER_SIZE - 1);
    if (strcmp(buffer, "dec_server") != 0)
    {
        fprintf(stderr, "Error: cannot connect to encryption server on port %d\n", port);
        exit(2);
    }

    printf("Here");
    // Send ciphertext and key to server
    memset(buffer, 0, BUFFER_SIZE);
    FILE *ciphertext_file = fopen(ciphertext, "r");
    while ((n = fread(buffer, 1, BUFFER_SIZE, ciphertext_file)) > 0)
    {
        write(sockfd, buffer, n);
        memset(buffer, 0, BUFFER_SIZE);
    }
    fclose(ciphertext_file);
    memset(buffer, 0, BUFFER_SIZE);
    sprintf(buffer, "%d", -1);
    write(sockfd, buffer, strlen(buffer));
    memset(buffer, 0, BUFFER_SIZE);
    FILE *key_file = fopen(key, "r");
    while ((n = fread(buffer, 1, BUFFER_SIZE, key_file)) > 0)
    {
        write(sockfd, buffer, n);
        memset(buffer, 0, BUFFER_SIZE);
    }
    fclose(key_file);

    // Receive decrypted plaintext from server and print to stdout
    memset(buffer, 0, BUFFER_SIZE);
    while ((n = read(sockfd, buffer, BUFFER_SIZE - 1)) > 0)
    {
        printf("%s", buffer);
        memset(buffer, 0, BUFFER_SIZE);
    }

    close(sockfd);
    return 0;
}
