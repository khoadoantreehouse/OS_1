#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>

#define BUFFER_SIZE 2048

// Check if the file contains only valid characters
int is_valid_file(char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        return 0;
    }
    int ch;
    int is_end_of_file = 0;
    while (!is_end_of_file && (ch = fgetc(file)) != EOF)
    {
        if (ch != ' ' && ch != '\n' && (ch < 'A' || ch > 'Z'))
        {
            fclose(file);
            return 0;
        }
        if (ch == '\n')
        {
            is_end_of_file = 1;
        }
    }
    fclose(file);
    return 1;
}

int main(int argc, char *argv[])
{
    // Check command-line arguments
    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s <plaintext> <key> <port>\n", argv[0]);
        exit(1);
    }
    char *plaintext = argv[1];
    char *key = argv[2];
    int port = atoi(argv[3]);

    // Check plaintext and key files for bad characters
    if (!is_valid_file(plaintext))
    {
        fprintf(stderr, "Error: invalid characters in %s\n", plaintext);
        exit(1);
    }
    if (!is_valid_file(key))
    {
        fprintf(stderr, "Error: invalid characters in %s\n", key);
        exit(1);
    }

    // Check key length
    FILE *key_file = fopen(key, "r");
    fseek(key_file, 0, SEEK_END);
    size_t key_size = ftell(key_file);
    fclose(key_file);
    FILE *plaintext_file = fopen(plaintext, "r");
    fseek(plaintext_file, 0, SEEK_END);
    size_t plaintext_size = ftell(plaintext_file);
    fclose(plaintext_file);
    if (key_size < plaintext_size)
    {
        fprintf(stderr, "Error: key %s is too short\n", key);
        exit(1);
    }

    // Connect to server
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        fprintf(stderr, "Error: could not create socket\n");
        exit(2);
    }
    struct hostent *server = gethostbyname("localhost");
    if (server == NULL)
    {
        fprintf(stderr, "Error: could not resolve hostname\n");
        exit(2);
    }
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    serv_addr.sin_port = htons(port);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        fprintf(stderr, "Error: could not connect to server on port %d\n", port);
        exit(2);
    }

    // Send plaintext and key to server
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    int n;
    FILE *plaintext_file2 = fopen(plaintext, "r");
    while ((n = fread(buffer, 1, BUFFER_SIZE, plaintext_file2)) > 0)
    {
        write(sockfd, buffer, n);
        memset(buffer, 0, BUFFER_SIZE);
    }
    fclose(plaintext_file2);
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

    // Write ciphertext to file
    FILE *ciphertext_file = fopen("ciphertext", "w");
    if (!ciphertext_file)
    {
        fprintf(stderr, "Error: could not open ciphertext file\n");
        exit(1);
    }
    fwrite(ciphertext_buffer, 1, ciphertext_len, ciphertext_file);
    fclose(ciphertext_file);

    close(sockfd);
    return 0;
}
