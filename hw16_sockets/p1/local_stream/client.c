#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/unix_socket"
#define BUF_SIZE 1024

int main() 
{
    int sock_fd;
    struct sockaddr_un server_addr;
    char buffer[BUF_SIZE];

    // Creating the Unix-socket
    sock_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (sock_fd == -1) 
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Setting up the server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_LOCAL;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    // Connecting with the server via Unix-socket
    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) 
    {
        perror("connect");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // Sending a message to the server
    const char *msg = "Hello";
    if (write(sock_fd, msg, strlen(msg)) == -1) 
    {
        perror("write");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // Receiving a response from the server
    ssize_t recv_len = read(sock_fd, buffer, BUF_SIZE - 1);
    if (recv_len == -1) 
    {
        perror("read");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    buffer[recv_len] = '\0';

    printf("Received from server: %s\n", buffer);

    // Closing the socket
    close(sock_fd);

    return 0;
}

