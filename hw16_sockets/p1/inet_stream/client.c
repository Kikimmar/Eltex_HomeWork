#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT 12345
#define BUF_SIZE 1024

int main()
{
    int sock_fd;
    struct sockaddr_in server_addr;
    char buffer[BUF_SIZE];

    // Create TCP-socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    //setting up the server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // Connecting to the server
    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    
    // Send a message to the server
    const char* msg = "Hello";
    send(sock_fd, msg, strlen(msg), 0);

    // Receive a response from the server
    ssize_t recv_len = recv(sock_fd, buffer, BUF_SIZE - 1, 0);
    if (recv_len == -1)
    {
        perror("recv");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    buffer[recv_len] = '\0';

    printf("Received from server: %s\n", buffer);

    // Closing the connection
    close(sock_fd);
    return 0;
}
