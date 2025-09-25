#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/unix_dgram_socket"
#define BUF_SIZE 1024

int main()
{
    int sock_fd;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[BUF_SIZE];

    // Create Dgram Unix-socket
    sock_fd = socket(AF_LOCAL, SOCK_DGRAM, 0);
    if (sock_fd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Deleting previous socket file, if it exist
    unlink(SOCKET_PATH);

    // Setting up the server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_LOCAL;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    // Binding the socke to the address
    if (bind(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // Accepting the datagram from the client
    ssize_t recv_len = recvfrom(sock_fd, buffer, BUF_SIZE - 1, 0, 
            (struct sockaddr*)&client_addr, &client_addr_len);
    if (recv_len == -1)
    {
        perror("recvfrom");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    buffer[recv_len] = '\0';
    printf("Received from client: %s\n", buffer);

    // Recponse to the client
    const char *reply = "Hi";
    ssize_t sent_len = sendto(sock_fd, reply, strlen(reply), 0, 
            (struct sockaddr*)&client_addr, client_addr_len);
    if (sent_len == -1)
    {
        perror("sendto");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // Close the socket and delet file
    close(sock_fd);
    unlink(SOCKET_PATH);

    return 0;
}
