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
    struct sockaddr_in broadcastAddr;
    char *message = "Broadcast message from server!";

    // Create the UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Enabling the ability to send broadcast
    int broadcastEnable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0) 
    {
        perror("setsockopt SO_BROADCAST failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Filling in the address for broadcast
    memset(&broadcastAddr, 0, sizeof(broadcastAddr));
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(PORT);
    broadcastAddr.sin_addr.s_addr = inet_addr("255.255.255.255");

    printf("Broadcast server started. Sending message every 3 seconds...\n");

    while (1) 
    {
        // Sending the broadcast msg
        if (sendto(sockfd, message, strlen(message), 0, (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr)) < 0) 
        {
            perror("sendto failed");
        } 
        else 
        {
            printf("Broadcast sent: %s\n", message);
        }
        sleep(3); // wait 3 sec
    }

    close(sockfd);
    return 0;
}

