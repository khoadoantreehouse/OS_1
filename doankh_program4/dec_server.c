#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 2048

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

char my_decrypt(char ciphertext, char key)
{
    char message;
    int i;

    int cipherVal = ciphertext - 'A';
    int keyVal = key - 'A';
    if (cipherVal == -33)
        cipherVal = 26;
    if (keyVal == -33)
        keyVal = 26;
    int diff = cipherVal - keyVal;
    if (diff < 0)
        diff += 27;
    int messageVal = diff % 27;
    message = messageVal + 'A';
    if (messageVal == 26)
        message = ' ';

    return message;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        error("Usage: dec_server <listening_port>");
    }
    char *ptr;
    long port = strtol(argv[1], &ptr, 10);
    if (port <= 0)
    {
        error("Invalid listening port");
    }

    // create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error("Error opening socket");
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
    }

    // listen for connections
    listen(sockfd, 5);

    // handle client connections
    socklen_t clilen = sizeof(cli_addr);
    int clientfd, pid;
    char buffer[2048], key[2048], ciphertext[2048];
    int n, i;

    while (1)
    {
        // accept connection
        clientfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (clientfd < 0)
        {
            error("Error accepting connection");
        }

        // fork child process to handle client
        pid = fork();
        if (pid < 0)
        {
            error("Error forking child process");
        }
        else if (pid == 0)
        {
            // child process

            // verify client identity
            memset(buffer, 0, 2048);
            n = read(clientfd, buffer, 2047);

            if (n < 0)
            {
                error("Error reading from socket");
            }

            if (strcmp(buffer, "dec_client") != 0)
            {
                error("Error: not a valid client");
            }

            // receive plaintext and key
            memset(buffer, 0, 2048);
            n = read(clientfd, buffer, 2047);
            if (n < 0)
            {
                error("Error reading from socket");
            }
            strcpy(ciphertext, buffer);

            memset(key, 0, 2048);
            n = read(clientfd, key, 2047);
            if (n < 0)
            {
                error("Error reading from socket");
            }

            // perform decryption
            for (i = 0; i < strlen(ciphertext); i++)
            {
                ciphertext[i] = my_decrypt(ciphertext[i], key[i]);
            }

            // send ciphertext back to client
            n = write(clientfd, ciphertext, strlen(ciphertext));
            if (n < 0)
            {
                error("Error writing to socket");
                // Close the socket for this client
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
} // added this line to fix the expected '}' error
