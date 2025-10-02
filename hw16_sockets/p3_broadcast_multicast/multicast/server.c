#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MULTICAST_GROUP "224.0.0.1"
#define PORT 12345
#define MAXLINE 1024

int main() 
{
    int sockfd;
    struct sockaddr_in multicastAddr;
    char *message = "Multicast message from server!";

    // Create the UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Filling in the address for multicast
    memset(&multicastAddr, 0, sizeof(multicastAddr));
    multicastAddr.sin_family = AF_INET;
    multicastAddr.sin_addr.s_addr = inet_addr(MULTICAST_GROUP);
    multicastAddr.sin_port = htons(PORT);


    printf("Multicast server started. Sending message every 3 seconds...\n");

    while (1) 
    {
        // Sending the broadcast msg
        if (sendto(sockfd, message, strlen(message), 0, (struct sockaddr*)&multicastAddr, sizeof(multicastAddr)) < 0) 
        {
            perror("sendto failed");
        } 
        else 
        {
            printf("Multicast sent: %s\n", message);
        }
        sleep(3); // wait 3 sec
    }

    close(sockfd);
    return 0;
}

