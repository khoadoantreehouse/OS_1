#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#define BUFFER_SIZE 150000

int is_valid_file(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        return 0;
    }

    int valid = 1;
    int c;
    while ((c = fgetc(file)) != EOF)
    {
        if (c == '\n')
        {
            continue;
        }
        if (c != ' ' && (c < 'A' || c > 'Z'))
        {
            valid = 0;
            break;
        }
    }

    fclose(file);

    return valid;
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
    char *ptr;
    long port = strtol(argv[3], &ptr, 10);

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
    int n;
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
        fprintf(stderr, "Error: could not connect to server on port %ld\n", port);
        exit(2);
    }

    // Send newline character to server
    char newline[2] = "\n";
    // Send program name to server
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    sprintf(buffer, "%s ", argv[0]);
    send(sockfd, buffer, strlen(buffer), 0);
    memset(buffer, 0, BUFFER_SIZE);
    send(sockfd, newline, strlen(newline), 0);

    // Send plaintext and key size to server
    FILE *plaintext_file2 = fopen(plaintext, "r");
    fseek(plaintext_file2, 0, SEEK_END);
    size_t plaintext_size2 = ftell(plaintext_file2);
    fclose(plaintext_file2);
    sprintf(buffer, "%lu ", plaintext_size2);
    send(sockfd, buffer, strlen(buffer), 0);
    send(sockfd, newline, strlen(newline), 0);

    memset(buffer, 0, BUFFER_SIZE);
    FILE *key_file2 = fopen(key, "r");
    fseek(key_file2, 0, SEEK_END);
    size_t key_size2 = ftell(key_file2);
    fclose(key_file2);
    sprintf(buffer, "%lu ", key_size2);
    send(sockfd, buffer, strlen(buffer), 0);
    send(sockfd, newline, strlen(newline), 0);

    memset(buffer, 0, BUFFER_SIZE);
    // Send plaintext and key to server
    FILE *plaintext_file3 = fopen(plaintext, "r");
    while ((n = fread(buffer, 1, BUFFER_SIZE, plaintext_file3)) > 0)
    {
        send(sockfd, buffer, n, 0);
        memset(buffer, 0, BUFFER_SIZE);
    }
    fclose(plaintext_file3);
    send(sockfd, newline, strlen(newline), 0);

    memset(buffer, 0, BUFFER_SIZE);

    FILE *key_file3 = fopen(key, "r");
    while ((n = fread(buffer, 1, BUFFER_SIZE, key_file3)) > 0)
    {
        send(sockfd, buffer, n, 0);
        memset(buffer, 0, BUFFER_SIZE);
    }
    fclose(key_file3);
    memset(buffer, 0, BUFFER_SIZE);

    send(sockfd, newline, strlen(newline), 0);

    // Receive ciphertext from server
    memset(buffer, 0, BUFFER_SIZE);
    int ciphertext_len = 0;
    int expected_ciphertext_len = plaintext_size2;
    while (ciphertext_len < expected_ciphertext_len)
    {
        n = recv(sockfd, buffer + ciphertext_len, BUFFER_SIZE, 0);
        fprintf(stdout, "%d", n);
        if (n < 0)
        {
            fprintf(stderr, "Error: could not receive ciphertext\n");
            exit(1);
        }
        ciphertext_len += n;
    }

    // Write ciphertext to stdout
    fwrite(buffer, 1, ciphertext_len, stdout);

    close(sockfd);
    return 0;
}
