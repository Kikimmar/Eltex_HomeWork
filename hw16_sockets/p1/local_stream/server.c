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
    int server_fd, client_fd;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[BUF_SIZE];

    // Create Unix-socket
    server_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Deleting the socket file if if already exist
    unlink(SOCKET_PATH);

    // Setting up the socket address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_LOCAL;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    // Binding the socket to the address in the file system
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listening for incoming connection
    if (listen(server_fd, 5) == -1)
    {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on %s\n", SOCKET_PATH);

    // Accepting the client's connection
    client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
    if (client_fd == -1)
    {
        perror("accept");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Reading a message from the client
    ssize_t recv_len = read(client_fd, buffer, BUF_SIZE - 1);
    if (recv_len == -1)
    {
        perror("read");
        close(client_fd);
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    buffer[recv_len] = '\0';

    printf("Received from client: %s\n", buffer);

    // Sending a response to the client
    const char *reply = "Hi";
    if (write(client_fd, reply, strlen(reply)) == -1)
    {
            perror("write");
            close(client_fd);
            close(server_fd);
            exit(EXIT_FAILURE);
    }
    
    // Close the connections and delete the socket file
    close(client_fd);
    close(server_fd);
    unlink(SOCKET_PATH);

    return 0;
}
