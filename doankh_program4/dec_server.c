#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 2048

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(1);
    }

    int portno = atoi(argv[1]);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error("ERROR opening socket");
    }

    struct sockaddr_in serv_addr;
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("ERROR on binding");
    }

    listen(sockfd, 5);

    while (1)
    {
        int newsockfd = accept(sockfd, NULL, NULL);
        if (newsockfd < 0)
        {
            error("ERROR on accept");
        }

        // Check that connection is coming from dec_client
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);
        read(newsockfd, buffer, BUFFER_SIZE - 1);
        if (strcmp(buffer, "dec_client") != 0)
        {
            fprintf(stderr, "ERROR: Connection not authorized from dec_client\n");
            close(newsockfd);
            continue;
        }

        // Receive ciphertext and key from client
        memset(buffer, 0, BUFFER_SIZE);
        int ciphertext_size;
        read(newsockfd, &ciphertext_size, sizeof(int));
        char ciphertext[ciphertext_size];
        memset(ciphertext, 0, ciphertext_size);
        int n;
        while ((n = read(newsockfd, ciphertext, ciphertext_size)) > 0)
        {
            break;
        }
        memset(buffer, 0, BUFFER_SIZE);
        read(newsockfd, buffer, BUFFER_SIZE - 1);
        int key_size = strlen(buffer);
        if (key_size < ciphertext_size)
        {
            fprintf(stderr, "ERROR: Key too short\n");
            close(newsockfd);
            continue;
        }
        char key[key_size];
        memset(key, 0, key_size);
        memcpy(key, buffer, key_size);

        // Decrypt ciphertext and write plaintext to socket
        char plaintext[ciphertext_size];
        int i;
        for (i = 0; i < ciphertext_size; i++)
        {
            plaintext[i] = ciphertext[i] - key[i];
            if (plaintext[i] < ' ')
            {
                plaintext[i] += 95;
            }
        }
        write(newsockfd, plaintext, ciphertext_size);

        close(newsockfd);
    }

    return 0;
}
