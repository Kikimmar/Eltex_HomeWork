#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12345
#define BUF_SIZE 1024

#define UDP_HDRLEN 8

int main()
{
    int sock_fd;
    struct sockaddr_in server_addr;
    char buffer[BUF_SIZE];

    int source_port = 54321;  //придуманный
    
    const char *msg = "Hello, server";
    size_t msg_len = strlen(msg);
    size_t udp_packet_len = UDP_HDRLEN + msg_len;

    sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if (sock_fd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) != 1)
    {
        perror("inet_pton");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // UDP header
    struct udphdr *udp = (struct udphdr *)buffer;
    udp->source = htons(source_port);
    udp->dest = htons(SERVER_PORT);
    udp->len = htons(udp_packet_len);
    udp->check = 0;

    memcpy(buffer + UDP_HDRLEN, msg, msg_len);

    ssize_t sent_bytes = sendto(sock_fd, buffer, udp_packet_len, 0, 
            (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (sent_bytes < 0)
    {
        perror("sendto");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    printf("Sent %zd bytes to server.\n", sent_bytes);

    while (1)
    {
        ssize_t bytes = recvfrom(sock_fd, buffer, BUF_SIZE, 0, NULL, NULL);
        if (bytes < 0)
        {
            perror("recvfrom");
            close(sock_fd);
            exit(EXIT_FAILURE);
        }
        
        struct iphdr *ip = (struct iphdr*)buffer;
        unsigned int ip_hdr_len = ip->ihl * 4;
        if (bytes < ip_hdr_len + UDP_HDRLEN)
        {
            continue;
        }
        
        struct udphdr *udp_recv = (struct udphdr*)(buffer + ip_hdr_len);
        if (htons(udp_recv->source) != SERVER_PORT)
        {
            continue;
        }

        int data_len = bytes - ip_hdr_len - UDP_HDRLEN;
        if (data_len >= BUF_SIZE)
        {
            data_len = BUF_SIZE - 1;
        }

        char* data = buffer + ip_hdr_len + UDP_HDRLEN;
        data[data_len] = '\0';

        printf("Received msg from server: %s\n", data);
       break; 

    }

    close(sock_fd);
    return 0;
}
