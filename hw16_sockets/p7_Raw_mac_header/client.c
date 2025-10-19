#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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

// MAC
const unsigned char client_mac[6] = {0x00, 0xd8, 0x61, 0xd7, 0xf7, 0x14};
const unsigned char server_mac[6] = {0x94, 0xbb, 0x43, 0x0d, 0x56, 0xa3};

// IP & PORTs
#define CLIENT_IP "192.168.0.109"
#define SERVER_IP "192.168.0.104"
#define CLIENT_PORT 54321
#define SERVER_PORT 12345

//Checksum IP_header
unsigned short checksum(void *vdata, size_t length);

int main()
{
    int sock_fd;
    char buffer[BUF_SIZE];
    char *data;
    struct sockaddr_ll socket_addr;
    struct iphdr *ip;
    struct udphdr *udp;
   
    const char *msg = "Hello, server";
    size_t msg_len = strlen(msg);
   

    sock_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock_fd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    unsigned int ifindex = if_nametoindex("enp34s0");
    if (ifindex == 0)
    {
        perror("if_nametoindex");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // Заполняем Ethernet заголовк (14 байт)
    unsigned char *ethernet_header = (unsigned char*)buffer;
    memcpy(ethernet_header, server_mac, 6);
    memcpy(ethernet_header + 6, client_mac, 6);
    ethernet_header[12] = ETH_P_IP >> 8;
    ethernet_header[13] = ETH_P_IP & 0xff;

    // IP header
    ip = (struct iphdr *)(buffer + ETH_HLEN);
    memset(ip, 0, sizeof(struct iphdr));
    ip->version = 4;
    ip->ihl = 5;
    ip->tos = 0;
    ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + msg_len);
    ip->id = htons(0);
    ip->frag_off = 0;
    ip->ttl = 255;
    ip->protocol = IPPROTO_UDP;
    ip->check = 0;
    ip->saddr = inet_addr(CLIENT_IP);
    ip->daddr = inet_addr(SERVER_IP);
    ip->check = checksum(ip, ip->ihl*4);

    // UDP header
    udp = (struct udphdr *)(buffer + ETH_HLEN + ip->ihl*4);
    udp->source = htons(CLIENT_PORT);
    udp->dest = htons(SERVER_PORT);
    udp->len = htons(sizeof(struct udphdr) + msg_len);
    udp->check = 0;

    
    data = buffer + ETH_HLEN + ip->ihl*4 + sizeof(struct udphdr);
    memcpy(data, msg, msg_len);

    memset(&socket_addr, 0, sizeof(struct sockaddr_ll));
    socket_addr.sll_family = AF_PACKET;
    //socket_address.sll_protocol = htons(ETH_P_IP);
    socket_addr.sll_ifindex = ifindex;
    //socket_address.hatype = ;
    //socket_address.pkttype = ;
    socket_addr.sll_halen = ETH_ALEN;
    memcpy(socket_addr.sll_addr, server_mac, 6);

    size_t packet_len = ETH_HLEN + ntohs(ip->tot_len);  //общая длинна Eth + IP + UDP + данные

    ssize_t sent_bytes = sendto(sock_fd, buffer, packet_len, 0,
            (struct sockaddr *)&socket_addr, sizeof(struct sockaddr_ll));
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

        if (bytes < ETH_HLEN + sizeof(struct iphdr) + sizeof(struct udphdr))
            continue;

        struct iphdr *recv_ip = (struct iphdr*)(buffer + ETH_HLEN);
        unsigned int recv_ip_header_len = recv_ip->ihl * 4;
        struct udphdr *udp_recv = (struct udphdr*)(buffer + ETH_HLEN + recv_ip_header_len);
        
        if (recv_ip->protocol != IPPROTO_UDP)
            continue;
        if (ntohs(udp_recv->source) != SERVER_PORT)
            continue;
        if (ntohs(udp_recv->dest) != CLIENT_PORT)
            continue;

        int data_len = bytes - ETH_HLEN - recv_ip_header_len - sizeof(struct udphdr);
        if (data_len >= BUF_SIZE)
            data_len = BUF_SIZE - 1;

        char* data = buffer + ETH_HLEN + recv_ip_header_len + sizeof(struct udphdr);
        data[data_len] = '\0';

        printf("Received msg from server: %s\n", data);
        break;
    }

    close(sock_fd);
    return 0;
}

unsigned short checksum(void *vdata, size_t length)
{
    char *data = (char*)vdata;
    uint32_t aac = 0xffff;

    // Считаем сумму 16 битных слов
    for (size_t i = 0; i + 1 < length; i += 2)
    {
        uint16_t word;
        memcpy(&word, data + i, 2);
        aac += ntohs(word);
        if (aac > 0xffff)
            aac -= 0xffff;
    }
    // Если длинна нечетная - добавляем последний байт
    if (length & 1)
    {
        uint16_t word = 0;
        memcpy(&word, data + length - 1, 1);
        aac += ntohs(word);
        if (aac > 0xffff)
            aac -= 0xffff;
    }
    return htons(~aac);
}
