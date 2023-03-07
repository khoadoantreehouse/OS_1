#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#define MAX_CONNECTIONS 5

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

char *encrypt(char *message, char *key)
{
    char *ciphertext = (char *)malloc((strlen(message) + 1) * sizeof(char));
    int i;
    for (i = 0; i < strlen(message); i++)
    {
        int messageVal = message[i] - 'A';
        int keyVal = key[i] - 'A';
        if (messageVal == -33)
            messageVal = 26;
        if (keyVal == -33)
            keyVal = 26;
        int sum = messageVal + keyVal;
        int cipherVal = sum % 27;
        ciphertext[i] = cipherVal + 'A';
        if (cipherVal == 26)
            ciphertext[i] = ' ';
    }
    ciphertext[i] = '\0';
    return ciphertext;
}

void error(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

char *encrypt(char *message, char *key)
{
    char *ciphertext = (char *)malloc((strlen(message) + 1) * sizeof(char));
    int i;
    for (i = 0; i < strlen(message); i++)
    {
        int messageVal = message[i] - 'A';
        int keyVal = key[i] - 'A';
        if (messageVal == -33)
            messageVal = 26;
        if (keyVal == -33)
            keyVal = 26;
        int sum = messageVal + keyVal;
        int cipherVal = sum % 27;
        ciphertext[i] = cipherVal + 'A';
        if (cipherVal == 26)
            ciphertext[i] = ' ';
    }
    ciphertext[i] = '\0';
    return ciphertext;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(1);
    }

    int portno = atoi(argv[1]);
    if (portno == 0)
    {
        error("Invalid port number");
    }

    int sockfd, newsockfd;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error("ERROR opening socket");
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("ERROR on binding");
    }

    listen(sockfd, MAX_CONNECTIONS);

    clilen = sizeof(cli_addr);
    int i;
    for (i = 0; i < MAX_CONNECTIONS; i++)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            while (1)
            {
                newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
                if (newsockfd < 0)
                {
                    error("ERROR on accept");
                }

                char buffer[256];
                bzero(buffer, 256);
                int n = read(newsockfd, buffer, 255);
                if (n < 0)
                {
                    error("ERROR reading from socket");
                }

                if (strcmp(buffer, "enc_client") != 0)
                {
                    fprintf(stderr, "Connection from invalid client\n");
                    close(newsockfd);
                    continue;
                }

                bzero(buffer, 256);
                n = read(newsockfd, buffer, 255);
                if (n < 0)
                {
                    error("ERROR reading from socket");
                }
                char *plaintext = strdup(buffer);

                bzero(buffer, 256);
                n = read(newsockfd, buffer, 255);
                if (n < 0)
                {
                    error("ERROR reading from socket");
                }
                char *key = strdup(buffer);

                if (strlen(plaintext) > strlen(key))
                {
                    fprintf(stderr, "Key length is too short\n");
                    close(newsockfd);
                    free(plaintext);
                    free(key);
                    continue;
                }

                char *ciphertext = encrypt(plaintext, key);
                n = write(newsockfd, ciphertext, strlen(ciphertext));
                if (n < 0)
                {
                    error("ERROR writing to socket");
                }
                free(plaintext);
                free(key);
                free(ciphertext);
                close(newsockfd);
            }
            exit(0);
        }
        else if (pid < 0)
        {
            error("ERROR on fork");
        }
    }

    close(sockfd);

    return 0;
}
