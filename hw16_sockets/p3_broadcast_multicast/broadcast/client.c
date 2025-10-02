#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 12345
#define MAXLINE 1024

int main() 
{
    int sockfd;
    struct sockaddr_in servaddr;
    char buffer[MAXLINE];
    socklen_t len;

    // Create the UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // We allow reuse of the address/port
    int reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) 
    {
        perror("setsockopt SO_REUSEADDR failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Filling in the local address for linking
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    // Binding a socket for receiving a broadcast message
    if (bind(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) 
    {
        perror("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Client started. Listening for broadcast on port %d...\n", PORT);

    while (1) 
    {
        len = sizeof(servaddr);
        int n = recvfrom(sockfd, buffer, MAXLINE, 0, (struct sockaddr*)&servaddr, &len);
        if (n < 0) 
        {
            perror("recvfrom failed");
            continue;
        }
        buffer[n] = '\0';
        printf("Received broadcast message: %s\n", buffer);
    }

    close(sockfd);
    return 0;
}

