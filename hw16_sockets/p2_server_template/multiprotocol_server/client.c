#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT 12345
#define BUFFER_SIZE 1024

int main()
{
    int tcp_socket, udp_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    ssize_t bytes;

    // Configurated the address for both sockets
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
    server_addr.sin_port = htons(PORT);

    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    connect(tcp_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
    write(tcp_socket, "Hello", 5);
    bytes = read(tcp_socket, buffer, sizeof(buffer) - 1);
    if (bytes > 0)
    {
        buffer[bytes] = '\0';
        printf("TCP server replied: %s\n", buffer);
    }
    close(tcp_socket);

    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    sendto(udp_socket, "Hi", 2, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    bytes = recvfrom(udp_socket, buffer, sizeof(buffer) - 1, 0, NULL, NULL);
    if (bytes > 0)
    {
        buffer[bytes] = '\0';
        printf("UDP server replied: %s\n", buffer);
    }
    close(udp_socket);
    return 0;
}