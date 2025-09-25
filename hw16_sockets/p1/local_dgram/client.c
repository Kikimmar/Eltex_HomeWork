#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SERVER_SOCKET_PATH "/tmp/unix_dgram_socket"
#define CLIENT_SOCKET_PATH "/tmp/unix_dgram_client_socket"
#define BUF_SIZE 1024

int main() 
{
    int sockfd;
    struct sockaddr_un server_addr, client_addr;
    char buffer[BUF_SIZE];

    // Create the Dgram Unix-socket
    sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0);
    if (sockfd == -1) 
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Setting up the client address (must be unique in order to receive responses)
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sun_family = AF_LOCAL;
    strncpy(client_addr.sun_path, CLIENT_SOCKET_PATH, sizeof(client_addr.sun_path) - 1);

    // Deleting the old client's file, if it exsist
    unlink(CLIENT_SOCKET_PATH);

    // Binding the client socket to the address
    if (bind(sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1) 
    {
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Setting up the server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_LOCAL;
    strncpy(server_addr.sun_path, SERVER_SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    // Sending a message to the server
    const char *msg = "Hello";
    ssize_t sent_len = sendto(sockfd, msg, strlen(msg), 0,
                              (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (sent_len == -1) 
    {
        perror("sendto");
        close(sockfd);
        unlink(CLIENT_SOCKET_PATH);
        exit(EXIT_FAILURE);
    }

    // Receiving reply
    ssize_t recv_len = recvfrom(sockfd, buffer, BUF_SIZE - 1, 0, NULL, NULL);
    if (recv_len == -1) 
    {
        perror("recvfrom");
        close(sockfd);
        unlink(CLIENT_SOCKET_PATH);
        exit(EXIT_FAILURE);
    }
    buffer[recv_len] = '\0';

    printf("Received from server: %s\n", buffer);

    // Close socket and delete client file
    close(sockfd);
    unlink(CLIENT_SOCKET_PATH);

    return 0;
}

