#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define MAX_EVENTS 10
#define PORT 12345
#define BUFFER_SIZE 1024

int main()
{
    int tcp_socket, udp_socket, epoll_fd;
    struct sockaddr_in addr;
    struct epoll_event ev, events[MAX_EVENTS];
    char buffer[BUFFER_SIZE];

    // Create the TCP-socket
    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket < 0)
    {
        perror("socket_tcp");
        exit(EXIT_FAILURE);
    }

    // Create the UDP-socket
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0)
    {
        perror("socket_udp");
        exit(EXIT_FAILURE);
    }

    // Configurated the address for both sockets
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    // Bind the TCP-socket
    if (bind(tcp_socket, (struct sockaddr*)&addr, sizeof(addr)))
    {
        perror("bind_tcp");
        close(tcp_socket);
        close(udp_socket);
        exit(EXIT_FAILURE);
    }

    // Bind the UDP-socket to the same port
    if (bind(udp_socket, (struct sockaddr*)&addr, sizeof(addr)))
    {
        perror("bind_udp");
        close(tcp_socket);
        close(udp_socket);
        exit(EXIT_FAILURE);
    }

    // Start to listen the TCP-socket
    if (listen(tcp_socket, SOMAXCONN))
    {
        perror("listen");
        close(tcp_socket);
        close(udp_socket);
        exit(EXIT_FAILURE);
    }

    // Create epoll
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0)
    {
        perror("epol_create1");
        close(tcp_socket);
        close(udp_socket);
        exit(EXIT_FAILURE);
    }

    // Registering a TCP socket in epoll for reading (new connections)
    ev.events = EPOLLIN;
    ev.data.fd = tcp_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, tcp_socket, &ev) < 0)
    {
        perror("epoll_ctl tcp-socket");
        close(tcp_socket);
        close(udp_socket);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    // Registering a UDP socket in epoll for reading (coming messages)
    ev.events = EPOLLIN;
    ev.data.fd = udp_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, udp_socket, &ev) < 0)
    {
        perror("epoll_ctl udp_socket");
        close(tcp_socket);
        close(udp_socket);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server running on port %d\n", PORT);

    while (1)
    {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, - 1);
        if (n < 0)
        {
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; i++)
        {
            int fd = events[i].data.fd;

            // New TCP connection
            if (fd == tcp_socket)
            {
                int client_fd = accept(tcp_socket, NULL, NULL);
                if (client_fd < 0)
                {
                    perror("accept");
                    continue;
                }

                ssize_t bytes = read(client_fd, buffer, sizeof(buffer) - 1);
                if (bytes > 0)
                {
                    buffer[bytes] = '\0';
                    printf("TCP recv: %s\n", buffer);

                    if (strcmp(buffer, "Hello") == 0)
                    {
                        const char *response = "Salut from TCP\n";
                        write(client_fd, response, strlen(response));
                    }
                }
                close(client_fd);
            }
            // UDP message
            else if (fd == udp_socket)
            {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                ssize_t bytes = recvfrom(udp_socket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&client_addr, &client_len);
                if (bytes < 0)
                {
                    perror("recvfrom");
                    continue;
                }
                
                buffer[bytes] = '\0';

                printf("UDP recv from %s:%d: %s\n", inet_ntoa(client_addr.sin_addr),
                                                    ntohs(client_addr.sin_port),
                                                    buffer);
                if (strcmp(buffer, "Hi") == 0)
                {
                    const char *udp_response = "Salut form UDP\n";
                    sendto(udp_socket, udp_response, strlen(udp_response), 0, (struct sockaddr*)&client_addr, client_len);
                }
            }
        }
    }
    close(tcp_socket);
    close(udp_socket);
    close(epoll_fd);

    return 0;    
}