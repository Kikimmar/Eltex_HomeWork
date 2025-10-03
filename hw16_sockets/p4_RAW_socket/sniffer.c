#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>

#define MAX_SIZE 65536

int main()
{
    int sockfd;
    char buffer[MAX_SIZE];

    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sockfd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        ssize_t bytes = recvfrom(sockfd, buffer, MAX_SIZE, 0, NULL, NULL);
        if (bytes < 0)
        {
            perror("recvfrom");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        printf("Received TCP packet of %zd bytes\n", bytes);

    }

    close(sockfd);
    return 0;
}

