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

unsigned char server_mac[6] = {0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb};

// Checksum функция из клиента
unsigned short checksum(void *vdata, size_t length) 
{
    char *data = vdata;
    uint32_t acc = 0xffff;
    
    for (size_t i = 0; i + 1 < length; i += 2) 
    {
        uint16_t word;
        memcpy(&word, data + i, 2);
        acc += ntohs(word);
        if (acc > 0xffff) acc -= 0xffff;
    }
    if (length & 1) 
    {
        uint16_t word = 0;
        memcpy(&word, data + length - 1, 1);
        acc += ntohs(word);
        if (acc > 0xffff) acc -= 0xffff;
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
        if (clients[i].ip.s_addr == ip->s_addr && clients[i].port == port) 
        {
            return i;
        }
    }
    return -1;
}

void add_client(struct in_addr *ip, uint16_t port) 
{
    if (client_count >= 100) return;
    clients[client_count].ip = *ip;
    clients[client_count].port = port;
    clients[client_count].counter = 0;
    client_count++;
}

int main(int argc, char *argv[]) 
{
    char if_name[IF_NAMESIZE] = "enp34s0";
    
    int opt;
    while ((opt = getopt(argc, argv, "i:S:")) != -1) 
    {
        switch (opt) {
            case 'i':
                strncpy(if_name, optarg, IF_NAMESIZE - 1);
                if_name[IF_NAMESIZE - 1] = '\0';
                break;
            case 'S':
                if (!parse_mac(optarg, server_mac)) 
                {
                    fprintf(stderr, "Invalid server MAC format\n");
                    return 1;
                }
                break;
            default:
                fprintf(stderr, "Usage: %s [-i interface] [-S server_mac]\n", argv[0]);
                return 1;
        }
    }
    
    printf("Server MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           server_mac[0], server_mac[1], server_mac[2], 
           server_mac[3], server_mac[4], server_mac[5]);
    
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
    
    printf("Server listening on %s (ifindex %d)\n", if_name, ifindex);
    
    char buffer[BUF_SIZE];
    struct sockaddr_ll socket_addr;
    
    while (1) 
    {
        ssize_t bytes = recvfrom(sock_fd, buffer, BUF_SIZE, 0, NULL, NULL);
        if (bytes < 0) 
        {
            perror("recvfrom");
            continue;
        }
        
        if (bytes < (ssize_t)(ETH_HLEN + sizeof(struct iphdr) + sizeof(struct udphdr)))
            continue;
        
        struct iphdr *ip = (struct iphdr*)(buffer + ETH_HLEN);
        if (ip->protocol != IPPROTO_UDP) continue;
        
        unsigned int ip_header_len = ip->ihl * 4;
        struct udphdr *udp = (struct udphdr*)(buffer + ETH_HLEN + ip_header_len);
        
        if (ntohs(udp->dest) != SERVER_PORT || ntohs(udp->source) != CLIENT_PORT)
            continue;
        
        // сохраняем IP и порт ДО обработки данных
        struct in_addr client_ip;
        client_ip.s_addr = ip->saddr;
        uint16_t client_port = ntohs(udp->source);
        
        int data_len = bytes - ETH_HLEN - ip_header_len - sizeof(struct udphdr);
        if (data_len >= BUF_SIZE) data_len = BUF_SIZE - 1;
        
        char *data = buffer + ETH_HLEN + ip_header_len + sizeof(struct udphdr);
        data[data_len] = '\0';
        
        // Находим клиента или создаем нового
        int client_idx = find_client(&client_ip, client_port);
        if (client_idx == -1) 
        {
            add_client(&client_ip, client_port);
            client_idx = client_count - 1;
            printf("New client %s:%d\n", inet_ntoa(client_ip), client_port);
        }
        
        // Проверяем CLOSE
        if (strcmp(data, "CLOSE") == 0) 
        {
            printf("Client %s:%d disconnected (counter reset)\n", 
                   inet_ntoa(client_ip), client_port);
            clients[client_idx].counter = 0;
            continue;
        }
        
        // Формируем ответ: сообщение + номер
        char reply[BUF_SIZE];
        int counter = ++clients[client_idx].counter;
        snprintf(reply, sizeof(reply), "%s %d", data, counter);
        size_t reply_len = strlen(reply);
        
        // Извлекаем MAC клиента из Ethernet заголовка входящего пакета
        char *client_mac = buffer + 6;  // MAC получателя в Eth заголовке
        
        // Формируем Ethernet заголовок ответа (меняем MAC адреса местами)
        unsigned char *eth = (unsigned char*)buffer;
        memcpy(eth, client_mac, 6);     // MAC клиента
        memcpy(eth + 6, server_mac, 6); // MAC сервера
        eth[12] = ETH_P_IP >> 8;
        eth[13] = ETH_P_IP & 0xff;
        
        // IP заголовок ответа
        struct iphdr *reply_ip = (struct iphdr*)(buffer + ETH_HLEN);
        memset(reply_ip, 0, sizeof(struct iphdr));
        reply_ip->version = 4;
        reply_ip->ihl = 5;
        reply_ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + reply_len);
        reply_ip->ttl = 255;
        reply_ip->protocol = IPPROTO_UDP;
        reply_ip->saddr = ip->daddr;  // IP сервера
        reply_ip->daddr = ip->saddr;  // IP клиента
        reply_ip->check = checksum(reply_ip, reply_ip->ihl * 4);
        
        // UDP заголовок ответа
        struct udphdr *reply_udp = (struct udphdr*)(buffer + ETH_HLEN + reply_ip->ihl * 4);
        reply_udp->source = htons(SERVER_PORT);
        reply_udp->dest = udp->source;
        reply_udp->len = htons(sizeof(struct udphdr) + reply_len);
        reply_udp->check = 0;
        
        // Данные ответа
        char *reply_data = buffer + ETH_HLEN + reply_ip->ihl * 4 + sizeof(struct udphdr);
        memcpy(reply_data, reply, reply_len);
        
        // Настраиваем адресат для отправки
        memset(&socket_addr, 0, sizeof(socket_addr));
        socket_addr.sll_family = AF_PACKET;
        socket_addr.sll_ifindex = ifindex;
        socket_addr.sll_halen = ETH_ALEN;
        memcpy(socket_addr.sll_addr, client_mac, 6);
        
        size_t packet_len = ETH_HLEN + ntohs(reply_ip->tot_len);
        ssize_t sent_bytes = sendto(sock_fd, buffer, packet_len, 0,
                                  (struct sockaddr*)&socket_addr, sizeof(socket_addr));
        
        if (sent_bytes < 0) 
        {
            perror("sendto");
        } 
        else 
        {
            printf("Echo to %s:%d: '%s' (%d)\n", 
                   inet_ntoa(client_ip), client_port, reply, counter);
        }
    }
    
    close(sock_fd);
    return 0;
}
