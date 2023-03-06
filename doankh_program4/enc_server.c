#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_CLIENTS 5

void error(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

char my_encrypt(char plaintext, char key)
{
    if (plaintext == ' ')
    {
        return key;
    }
    else
    {
        return ((plaintext - 'A' + key - 'A') % 26) + 'A';
    }
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s plaintext key port\n", argv[0]);
        exit(1);
    }

    if (strcmp(argv[0], "enc_client") != 0)
    {
        fprintf(stderr, "Error: not a valid client\n");
        exit(1);
    }

    char *plaintext = argv[1];
    char *key = argv[2];
    int port = atoi(argv[3]);

    if (port <= 0)
    {
        fprintf(stderr, "Invalid listening port\n");
        exit(1);
    }

    // create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Error opening socket");
        exit(1);
    }

    // connect to server
    struct sockaddr_in serv_addr;
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Error connecting to server");
        exit(1);
    }

    // send identity to server
    if (write(sockfd, "enc_client", 11) < 0)
    {
        perror("Error writing to socket");
        exit(1);
    }

    // send plaintext to server
    if (write(sockfd, plaintext, strlen(plaintext)) < 0)
    {
        perror("Error writing to socket");
        exit(1);
    }

    // send key to server
    if (write(sockfd, key, strlen(key)) < 0)
    {
        perror("Error writing to socket");
        exit(1);
    }

    // read ciphertext from server
    char buffer[2048];
    memset(buffer, 0, 2048);
    int n = read(sockfd, buffer, 2047);
    if (n < 0)
    {
        perror("Error reading from socket");
        exit(1);
    }

    // write ciphertext to file
    FILE *f = fopen("ciphertext", "w");
    if (f == NULL)
    {
        perror("Error opening file");
        exit(1);
    }

    fprintf(f, "%s", buffer);
    fclose(f);

    close(sockfd);

    return 0;
}
