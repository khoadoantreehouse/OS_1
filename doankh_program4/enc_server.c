#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_CLIENTS 5
#define BUFFER_SIZE 256

void error(char *msg)
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

void validateArgs(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(1);
    }
    int port = atoi(argv[1]);
    if (port < 1024 || port > 65535)
    {
        fprintf(stderr, "Invalid port number: %s\n", argv[1]);
        exit(1);
    }
}

int setupSocket(int portNum)
{
    int serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0)
    {
        error("Error opening socket");
    }

    struct sockaddr_in serverAddr;
    bzero((char *)&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(portNum);

    if (bind(serverSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        error("Error binding socket");
    }

    listen(serverSock, MAX_CLIENTS);
    return serverSock;
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

int daemonize()
{
    // Fork the process
    pid_t pid = fork();
    if (pid < 0)
    {
        error("ERROR: Could not fork");
        exit(1);
    }
    if (pid > 0)
    {
        // Parent process
        exit(0);
    }

    // Child process
    // Set the file mode mask
    umask(0);

    // Create a new session
    pid_t sid = setsid();
    if (sid < 0)
    {
        error("ERROR: Could not create new session");
        exit(1);
    }

    // Close standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    return 0;
}

void handleConnection(int clientSock)
{
    char buffer[BUFFER_SIZE];
    char *clientName = "enc_client";
    int charsRead;

    // Verify connection is from enc_client
    memset(buffer, '\0', BUFFER_SIZE);
    charsRead = recv(clientSock, buffer, BUFFER_SIZE - 1, 0);
    if (charsRead < 0)
    {
        error("ERROR reading from socket");
        close(clientSock);
        exit(1);
    }
    if (strcmp(buffer, clientName) != 0)
    {
        fprintf(stderr, "ERROR: Connection not from %s\n", clientName);
        close(clientSock);
        exit(2);
    }

    // Receive plaintext
    memset(buffer, '\0', BUFFER_SIZE);
    charsRead = recv(clientSock, buffer, BUFFER_SIZE - 1, 0);
    if (charsRead < 0)
    {
        error("ERROR reading from socket");
        close(clientSock);
        exit(1);
    }
    char *plaintext = strdup(buffer);

    // Receive key
    memset(buffer, '\0', BUFFER_SIZE);
    charsRead = recv(clientSock, buffer, BUFFER_SIZE - 1, 0);
    if (charsRead < 0)
    {
        error("ERROR reading from socket");
        close(clientSock);
        free(plaintext);
        exit(1);
    }
    char *key = strdup(buffer);

    // Encrypt plaintext using key
    char *ciphertext = encrypt(plaintext, key);

    // Send ciphertext back to client
    charsRead = send(clientSock, ciphertext, strlen(ciphertext), 0);
    if (charsRead < 0)
    {
        error("ERROR writing to socket");
        close(clientSock);
        free(plaintext);
        free(key);
        free(ciphertext);
        exit(1);
    }

    // Clean up
    close(clientSock);
    free(plaintext);
    free(key);
    free(ciphertext);
}

int main(int argc, char *argv[])
{
    // Validate command-line arguments
    validateArgs(argc, argv);

    // Setup socket
    int portNum = atoi(argv[1]);
    int listenSock = setupSocket(portNum);

    // Daemonize process
    daemonize();

    // Listen for incoming connections
    while (1)
    {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientSock = accept(listenSock, (struct sockaddr *)&clientAddr, &clientLen);

        if (clientSock < 0)
        {
            error("ERROR on accept");
            continue;
        }

        // Handle the connection in a separate process
        pid_t pid = fork();
        if (pid < 0)
        {
            error("ERROR on fork");
            close(clientSock);
            continue;
        }
        else if (pid == 0)
        {
            close(listenSock);
            handleConnection(clientSock);
            close(clientSock);
            exit(0);
        }
        else
        {
            close(clientSock);
        }
    }

    // Close the listening socket
    close(listenSock);

    return 0;
}
