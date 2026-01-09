#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#define BUF_SIZE 1500
#define CLIENT_PORT 54321
#define SERVER_PORT 12345

unsigned short checksum(void *vdata, size_t length);
int parse_mac(const char *str, unsigned char *mac);

int main(int argc, char *argv[])
{
    int opt;
    char if_name[IF_NAMESIZE] = "enp34s0";
    char client_ip_str[INET_ADDRSTRLEN] = "192.168.1.100";
    char server_ip_str[INET_ADDRSTRLEN] = "192.168.1.200";
    unsigned char client_mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    unsigned char server_mac[6] = {0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb};

    while((opt = getopt(argc, argv, "c:s:C:S:i:")) != -1)
    {
        switch(opt)
        {
            case 'c': strncpy(client_ip_str, optarg, INET_ADDRSTRLEN - 1); client_ip_str[INET_ADDRSTRLEN - 1] = '\0'; break;
            case 's': strncpy(server_ip_str, optarg, INET_ADDRSTRLEN - 1); server_ip_str[INET_ADDRSTRLEN - 1] = '\0'; break;
            case 'C': if (!parse_mac(optarg, client_mac)) { fprintf(stderr, "Invalid client MAC\n"); return 1; } break;
            case 'S': if (!parse_mac(optarg, server_mac)) { fprintf(stderr, "Invalid server MAC\n"); return 1; } break;
            case 'i': strncpy(if_name, optarg, IF_NAMESIZE - 1); if_name[IF_NAMESIZE - 1] = '\0'; break;
            default: 
                fprintf(stderr, "Usage: %s [-c client_ip] [-s server_ip] [-C client_mac] [-S server_mac] [-i interface]\n", argv[0]);
                return 1;
        }
    }

    printf("Client IP: %s, Server IP: %s, Interface: %s\n", client_ip_str, server_ip_str, if_name);
    printf("Client MAC: %02x:%02x:%02x:%02x:%02x:%02x\nServer MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           client_mac[0], client_mac[1], client_mac[2], client_mac[3], client_mac[4], client_mac[5],
           server_mac[0], server_mac[1], server_mac[2], server_mac[3], server_mac[4], server_mac[5]);

    
    int sock_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock_fd < 0) 
        { 
            perror("socket"); 
            exit(EXIT_FAILURE); 
        }

    unsigned int ifindex = if_nametoindex(if_name);
    if (ifindex == 0) 
    { 
        perror("if_nametoindex"); 
        close(sock_fd); 
        exit(EXIT_FAILURE); 
    }

    struct sockaddr_ll socket_addr;
    memset(&socket_addr, 0, sizeof(socket_addr));
    socket_addr.sll_family = AF_PACKET;
    socket_addr.sll_ifindex = ifindex;
    socket_addr.sll_halen = ETH_ALEN;
    memcpy(socket_addr.sll_addr, server_mac, 6);

    char buffer[BUF_SIZE];
    printf("Enter messages or 'CLOSE' to exit:\n");

    while (1) 
    {
        char input[BUF_SIZE];
        if (fgets(input, sizeof(input), stdin) == NULL) 
            break;
        
        input[strcspn(input, "\n")] = 0;
        
        if (strcmp(input, "CLOSE") == 0) 
        {
            printf("Sending CLOSE...\n");
            // Отправляем CLOSE и выходим
        }
        
        size_t msg_len = strlen(input);
        
        // Ethernet заголовок
        unsigned char *eth = (unsigned char*)buffer;
        memcpy(eth, server_mac, 6);        // Destination MAC (server)
        memcpy(eth + 6, client_mac, 6);    // Source MAC (client)
        eth[12] = ETH_P_IP >> 8; eth[13] = ETH_P_IP & 0xFF;

        // IP заголовок
        struct iphdr *ip = (struct iphdr*)(buffer + ETH_HLEN);
        memset(ip, 0, sizeof(struct iphdr));
        ip->version = 4;
        ip->ihl = 5;
        ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + msg_len);
        ip->id = htons(54321);  // Уникальный ID
        ip->frag_off = 0;
        ip->ttl = 64;
        ip->protocol = IPPROTO_UDP;
        ip->saddr = inet_addr(client_ip_str);
        ip->daddr = inet_addr(server_ip_str);
        ip->check = checksum(ip, ip->ihl*4);

        // UDP заголовок
        struct udphdr *udp = (struct udphdr*)(buffer + ETH_HLEN + ip->ihl*4);
        udp->source = htons(CLIENT_PORT);
        udp->dest = htons(SERVER_PORT);
        udp->len = htons(sizeof(struct udphdr) + msg_len);
        udp->check = 0;  // UDP checksum отключен

        // Данные
        char *data = buffer + ETH_HLEN + ip->ihl*4 + sizeof(struct udphdr);
        memcpy(data, input, msg_len);
        data[msg_len] = 0;

        size_t packet_len = ETH_HLEN + ntohs(ip->tot_len);
        
        ssize_t sent = sendto(sock_fd, buffer, packet_len, 0, (struct sockaddr*)&socket_addr, sizeof(socket_addr));
        if (sent < 0) 
            { 
                perror("sendto"); 
                continue; 
            }
        
        printf("Sent (%zd bytes): %s\n", sent, input);
        
        if (strcmp(input, "CLOSE") == 0) 
            break;

        // Получаем ответ
        ssize_t bytes = recvfrom(sock_fd, buffer, BUF_SIZE, 0, NULL, NULL);
        if (bytes < (ssize_t)(ETH_HLEN + sizeof(struct iphdr))) 
            continue;
        
        struct iphdr *recv_ip = (struct iphdr*)(buffer + ETH_HLEN);
        if (recv_ip->protocol != IPPROTO_UDP) 
            continue;
        
        unsigned int ip_len = recv_ip->ihl * 4;
        struct udphdr *recv_udp = (struct udphdr*)(buffer + ETH_HLEN + ip_len);
        if (ntohs(recv_udp->source) != SERVER_PORT || ntohs(recv_udp->dest) != CLIENT_PORT) 
            continue;
        
        int data_len = bytes - ETH_HLEN - ip_len - sizeof(struct udphdr);
        if (data_len > 0) 
        {
            char *recv_data = buffer + ETH_HLEN + ip_len + sizeof(struct udphdr);
            recv_data[data_len] = 0;
            printf("Server: %s\n", recv_data);
        }
    }

    close(sock_fd);
    return 0;
}

unsigned short checksum(void *vdata, size_t length)
{
    uint32_t acc = 0xffff;
    uint8_t *data = vdata;

    for (size_t i = 0; i + 1 < length; i += 2) 
    {
        uint16_t word;
        memcpy(&word, data + i, 2);
        acc += ntohs(word);
        if (acc > 0xffff) 
            acc -= 0xffff;
    }
    
    if (length & 1) 
    {
        uint16_t word = 0;
        memcpy(&word, data + length - 1, 1);
        acc += ntohs(word);
        if (acc > 0xffff) 
            acc -= 0xffff;
    }
    return htons(~acc);
}

int parse_mac(const char *str, unsigned char *mac)
{
    return sscanf(str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
            &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) == 6;
}
