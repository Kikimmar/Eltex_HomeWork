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
    socklen_t addr_len = sizeof(server_addr);

    // Creating UDP-socket
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Setting up the server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0)
    {
        perror("inet_pron");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // Sending a message to the server without connetion
    const char* msg = "Hello";
    ssize_t sent_len = sendto(sock_fd, msg, strlen(msg), 0, (struct sockaddr*)&server_addr, addr_len);
    if (sent_len == -1)
    {
        perror("sendto");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    
    // Receiving a response from the server
    ssize_t recv_len = recvfrom(sock_fd, buffer, BUF_SIZE - 1, 0, NULL, NULL);
    if (recv_len == -1)
    {
        perror("recvfrom");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    buffer[recv_len] = '\0';

    printf("Received from server: %s\n", buffer);

    close(sock_fd);
    return 0;
}
