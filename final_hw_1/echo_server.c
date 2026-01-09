#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <net/if.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#define BUF_SIZE 1500
#define SERVER_PORT 12345
#define CLIENT_PORT 54321

typedef struct 
{
    struct in_addr ip;
    uint16_t port;
    int counter;
} client_t;

client_t clients[100];
int client_count = 0;
unsigned char server_mac[6] = {0x94, 0xbb, 0x43, 0x0d, 0x56, 0xa3};

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

int find_client(struct in_addr *ip, uint16_t port) 
{
    for (int i = 0; i < client_count; i++) 
    {
        if (clients[i].ip.s_addr == ip->s_addr && clients[i].port == port) return i;
    }
    return -1;
}

void add_client(struct in_addr *ip, uint16_t port) 
{
    if (client_count >= 100) 
        return;
    clients[client_count].ip = *ip;
    clients[client_count].port = port;
    clients[client_count].counter = 0;
    client_count++;
    printf("New client %s:%d connected\n", inet_ntoa(*ip), port);
}

int main(int argc, char *argv[]) 
{
    char if_name[IF_NAMESIZE] = "wlo1";
    int opt;
    while ((opt = getopt(argc, argv, "i:S:")) != -1) 
    {
        switch (opt) 
        {
            case 'i': strncpy(if_name, optarg, IF_NAMESIZE - 1); if_name[IF_NAMESIZE - 1] = '\0'; break;
            case 'S': if (!parse_mac(optarg, server_mac)) { fprintf(stderr, "Invalid server MAC\n"); return 1; } break;
            default: fprintf(stderr, "Usage: %s [-i interface] [-S server_mac]\n", argv[0]); return 1;
        }
    }

    printf("Server on %s, MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           if_name, server_mac[0], server_mac[1], server_mac[2], server_mac[3], server_mac[4], server_mac[5]);

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

    struct sockaddr_ll addr = {0};
    addr.sll_family = AF_PACKET;
    addr.sll_ifindex = ifindex;
    addr.sll_halen = ETH_ALEN;

    char buffer[BUF_SIZE];
    printf("Server listening...\n");

    while (1) 
    {
        ssize_t bytes = recvfrom(sock_fd, buffer, BUF_SIZE, 0, NULL, NULL);
        if (bytes < (ssize_t)(ETH_HLEN + sizeof(struct iphdr))) 
            continue;

        struct iphdr *ip = (struct iphdr*)(buffer + ETH_HLEN);
        if (ip->protocol != IPPROTO_UDP) 
            continue;

        unsigned int ip_len = ip->ihl * 4;
        struct udphdr *udp = (struct udphdr*)(buffer + ETH_HLEN + ip_len);
        if (ntohs(udp->dest) != SERVER_PORT || ntohs(udp->source) != CLIENT_PORT) 
            continue;

        struct in_addr client_ip; client_ip.s_addr = ip->saddr;
        uint16_t client_port = ntohs(udp->source);
        unsigned char *client_mac = (unsigned char*)(buffer + 6);

        int data_len = bytes - ETH_HLEN - ip_len - sizeof(struct udphdr);
        if (data_len <= 0 || data_len >= BUF_SIZE) 
            continue;
        char *data = buffer + ETH_HLEN + ip_len + sizeof(struct udphdr);
        data[data_len] = 0;

        int client_idx = find_client(&client_ip, client_port);
        if (client_idx == -1) 
        {
            add_client(&client_ip, client_port);
            client_idx = client_count - 1;
        }

        if (strcmp(data, "CLOSE") == 0) 
        {
            printf("Client %s:%d disconnected\n", inet_ntoa(client_ip), client_port);
            clients[client_idx].counter = 0;
            continue;
        }

        // Формируем ответ
        char reply[BUF_SIZE];
        int counter = ++clients[client_idx].counter;
        snprintf(reply, sizeof(reply), "%s %d", data, counter);

        // Ethernet заголовок ответа
        unsigned char *eth = (unsigned char*)buffer;
        memcpy(eth, client_mac, 6);     // MAC клиента
        memcpy(eth + 6, server_mac, 6); // MAC сервера
        eth[12] = ETH_P_IP >> 8; eth[13] = ETH_P_IP & 0xFF;

        // IP заголовок ответа
        struct iphdr *reply_ip = (struct iphdr*)(buffer + ETH_HLEN);
        memset(reply_ip, 0, sizeof(struct iphdr));
        reply_ip->version = 4; reply_ip->ihl = 5;
        reply_ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + strlen(reply));
        reply_ip->id = htons(12345);
        reply_ip->frag_off = 0; reply_ip->ttl = 64;
        reply_ip->protocol = IPPROTO_UDP;
        reply_ip->saddr = ip->daddr;
        reply_ip->daddr = ip->saddr;
        reply_ip->check = checksum(reply_ip, reply_ip->ihl*4);

        // UDP заголовок ответа
        struct udphdr *reply_udp = (struct udphdr*)(buffer + ETH_HLEN + reply_ip->ihl*4);
        reply_udp->source = htons(SERVER_PORT);
        reply_udp->dest = udp->source;
        reply_udp->len = htons(sizeof(struct udphdr) + strlen(reply));
        reply_udp->check = 0;

        // Данные ответа
        char *reply_data = buffer + ETH_HLEN + reply_ip->ihl*4 + sizeof(struct udphdr);
        strcpy(reply_data, reply);

        // Отправляем ответ
        memcpy(addr.sll_addr, client_mac, 6);
        size_t packet_len = ETH_HLEN + ntohs(reply_ip->tot_len);
        ssize_t sent = sendto(sock_fd, buffer, packet_len, 0, (struct sockaddr*)&addr, sizeof(addr));
        
        if (sent > 0) 
        {
            printf("-> %s:%d: %s (%d)\n", inet_ntoa(client_ip), client_port, reply, counter);
        }
    }

    close(sock_fd);
    return 0;
}
