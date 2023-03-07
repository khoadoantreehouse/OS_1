#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_CLIENTS 5
#define BUFFER_SIZE 150000

typedef struct
{
    char *name;
    int plaintext_size;
    int key_size;
    char *plaintext;
    char *key;
} Cipher;

void error(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

char my_encrypt(char message, char key)
{
    char ciphertext;
    int i;

    int messageVal = message - 'A';
    int keyVal = key - 'A';
    if (messageVal == -33)
        messageVal = 26;
    if (keyVal == -33)
        keyVal = 26;
    int sum = messageVal + keyVal;
    int cipherVal = sum % 27;
    ciphertext = cipherVal + 'A';
    if (cipherVal == 26)
        ciphertext = ' ';

    return ciphertext;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        error("Usage: enc_server <listening_port>");
        exit(1);
    }

    char *ptr;
    long port = strtol(argv[1], &ptr, 10);
    if (port <= 0)
    {
        error("Invalid listening port");
        exit(1);
    }

    // create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error("Error opening socket");
        close(sockfd);
        exit(1);
    }

    // set SO_REUSEADDR option
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

    // bind socket to port
    struct sockaddr_in serv_addr, cli_addr;
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("Error binding socket to port");
        close(sockfd);
        exit(1);
    }

    // listen for connections
    listen(sockfd, 5);

    // handle client connections
    socklen_t clilen = sizeof(cli_addr);
    int clientfd, pid;
    char buffer[BUFFER_SIZE], key[BUFFER_SIZE], ciphertext[BUFFER_SIZE];
    int n, i;

    while (1)
    {
        // accept connection
        clientfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (clientfd < 0)
        {
            error("Error accepting connection");
            close(clientfd);
            exit(1);
        }

        // fork child process to handle client
        pid = fork();
        if (pid < 0)
        {
            error("Error forking child process");
            close(clientfd);
            exit(1);
        }
        else if (pid == 0)
        {
            // child process

            // verify client identity
            // Receive data in a loop until the entire message has been received
            char *buffer = NULL;
            int buffer_size = 0;
            int total_received = 0;
            while (1)
            {
                // Resize the buffer to accommodate more data
                buffer_size += BUFFER_SIZE;
                buffer = realloc(buffer, buffer_size);
                if (buffer == NULL)
                {
                    error("Error allocating memory");
                    close(clientfd);
                    exit(1);
                }

                // Receive data into the buffer
                n = recv(clientfd, buffer + total_received, BUFFER_SIZE, 0);
                if (n < 0)
                {
                    error("Error reading from socket");
                    close(clientfd);
                    exit(1);
                }
                total_received += n;

                // Check if we have received the entire message
                if (buffer[total_received - 1] == '\n')
                {
                    break;
                }
            }

            // Break the buffer into cipher using strtok
            char *brk = strdup(buffer);
            Cipher cipher;
            cipher.name = strtok(brk, "\n");
            cipher.plaintext_size = atoi(strtok(NULL, "\n"));
            cipher.key_size = atoi(strtok(NULL, "\n"));
            cipher.plaintext = strtok(NULL, "\n");
            cipher.key = strtok(NULL, "\n");

            // Print the cipher data
            fprintf(stdout, "Name: %s\n", cipher.name);
            fprintf(stdout, "Plaintext size: %d\n", cipher.plaintext_size);
            fprintf(stdout, "Key size: %d\n", cipher.key_size);
            fprintf(stderr, "Plaintext: %s\n", cipher.plaintext);
            fprintf(stderr, "Key: %s\n", cipher.key);
            send(clientfd, cipher.key, cipher.key_size, 0);

            // Free the buffer
            free(buffer);

            if (strstr(cipher.name, "enc_client") == NULL)
            {
                error("Not authorized client");
                close(clientfd);
                exit(1);
            }

            // perform encryption
            for (i = 0; i < cipher.plaintext_size; i++)
            {
                ciphertext[i] = toupper(cipher.plaintext[i]);
                key[i] = toupper(cipher.key[i]);
                ciphertext[i] = my_encrypt(ciphertext[i], key[i]);
            }

            if (cipher.plaintext_size > strlen(cipher.plaintext))
            {
                ciphertext[cipher.plaintext_size - 1] = '\n';
            }
            // // send ciphertext back to client
            int encrypted_text_size = strlen(ciphertext);
            char encrypted_text[encrypted_text_size + 1];
            strcpy(encrypted_text, ciphertext);
            n = send(clientfd, encrypted_text, encrypted_text_size, 0);
            if (n < 0)
            {
                error("Error sending encrypted text");
                close(clientfd);
                exit(1);
            }

            // close the client socket and exit child process
            close(clientfd);
            exit(0);
        }
        else
        {
            // in parent process
            // close the client socket in the parent process to allow for more connections
            close(clientfd);
        }
    }

    // close the server socket
    close(sockfd);
    return 0;
}
