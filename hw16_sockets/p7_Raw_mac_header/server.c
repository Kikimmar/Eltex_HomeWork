#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 12345
#define BUF_SIZE 1024

int main()
{
    int sock_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[BUF_SIZE];

    // Create the UDP-socket
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Setting up the server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Binding the socket to the address
    if (bind(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // Accepting a message from client without connection
    ssize_t recv_len = recvfrom(sock_fd, buffer, BUF_SIZE - 1, 0, (struct sockaddr*)&client_addr, &client_addr_len);
    if (recv_len == -1)
    {
        perror("recvfrom");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    buffer[recv_len] = '\0';

    printf("Received from client: %s\n", buffer);

    // Sending a response to the client
    const char* reply = "Hi";
    ssize_t sent_len = sendto(sock_fd, reply, strlen(reply), 0, (struct sockaddr*)&client_addr, client_addr_len);
    if (sent_len == -1)
    {
        perror("sendto");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    close(sock_fd);
    return 0;
}
