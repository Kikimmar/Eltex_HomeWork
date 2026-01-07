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

//Checksum IP_header
unsigned short checksum(void *vdata, size_t length);

// Parsing MAC from string "XX:XX:XX:XX:XX:XX"
int parse_mac(const char *str, unsigned char *mac);

int main(int argc, char *argv[])
{
    int opt;
    char if_name[IF_NAMESIZE] = "enp34s0";
    char client_ip_str[INET_ADDRSTRLEN] = "192.168.1.100";
    char server_ip_str[INET_ADDRSTRLEN] = "192.168.1.200";
    unsigned char client_mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    unsigned char server_mac[6] = {0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb};

    // Чтение параметров: -с клиентский IP, -s серверный IP
    // -C MAC клиента, -S MAC сервера, -i имя интерфейса
    while((opt = getopt(argc, argv, "c:s:C:S:i:")) != -1)
    {
        switch(opt)
        {
            case 'c':
                strncpy(client_ip_str,optarg, INET_ADDRSTRLEN - 1);
                client_ip_str[INET_ADDRSTRLEN - 1] = '\0';
                break;
            case 's':
                strncpy(server_ip_str, optarg, INET_ADDRSTRLEN - 1);
                server_ip_str[INET_ADDRSTRLEN - 1] = '\0';
                break;
            case 'C':
                if (!parse_mac(optarg, client_mac))
                {
                    fprintf(stderr, "Invalid client MAC format\n");
                    return 1;
                }
                break;
            case 'S':
                if (!parse_mac(optarg, server_mac))
                {
                    fprintf(stderr, "Invalid serveer MAC format\n");
                    return 1;
                }
                break;
            case 'i':
                strncpy(if_name, optarg, IF_NAMESIZE - 1);
                if_name[IF_NAMESIZE - 1] = '\0';
                break;
            default:
                fprintf(stderr, "Usage:%s[-c client ip][-s server_ip][-C client_mac][-S sever_mac]\n", argv[0]);
                return -1;
        }
    }

    printf("Client IP: %s\nServer IP: %s\n", client_ip_str, server_ip_str);
    printf("Client MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
            client_mac[0], client_mac[1], client_mac[2], client_mac[3], client_mac[4], client_mac[5]);
    printf("Server MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
            server_mac[0], server_mac[1], server_mac[2], server_mac[3], server_mac[4], server_mac[5]);


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

    unsigned int ifindex = if_nametoindex(if_name);
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
    ip->saddr = inet_addr(client_ip_str);
    ip->daddr = inet_addr(server_ip_str);
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
    printf("Enter messages or 'CLOSE' to exit:\n");

    while (1) {
        char input[BUF_SIZE];
        if (fgets(input, sizeof(input), stdin))
        {
            if (ferror(stdin))
            {
                perror("fgets error");
                break;
            }
            break;
        }

        input[strcspn(input, "\n")] = 0;  // убираем \n
        
        if (strcmp(input, "CLOSE") == 0) 
        {
            // Отправляем сообщение о закрытии
            const char *close_msg = "CLOSE";
            size_t msg_len = strlen(close_msg);
            
            // Обновляем IP пакет
            ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + msg_len);
            ip->check = 0;
            ip->check = checksum(ip, ip->ihl*4);
            
            // Обновляем UDP
            udp->len = htons(sizeof(struct udphdr) + msg_len);
            udp->check = 0;
            
            data = buffer + ETH_HLEN + ip->ihl*4 + sizeof(struct udphdr);
            memcpy(data, close_msg, msg_len);
            
            packet_len = ETH_HLEN + ntohs(ip->tot_len);
            
            sendto(sock_fd, buffer, packet_len, 0, (struct sockaddr *)&socket_addr, sizeof(struct sockaddr_ll));
            printf("Sent CLOSE message\n");
            break;
        }
        
        // Обычное сообщение
        size_t msg_len = strlen(input);
        ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + msg_len);
        ip->check = 0;
        ip->check = checksum(ip, ip->ihl*4);
        
        udp->len = htons(sizeof(struct udphdr) + msg_len);
        udp->check = 0;
        
        data = buffer + ETH_HLEN + ip->ihl*4 + sizeof(struct udphdr);
        memcpy(data, input, msg_len);
        
        packet_len = ETH_HLEN + ntohs(ip->tot_len);
        sendto(sock_fd, buffer, packet_len, 0, (struct sockaddr *)&socket_addr, sizeof(struct sockaddr_ll));
        printf("Sent: %s\n", input);
        
        // Получаем ответ
        ssize_t bytes = recvfrom(sock_fd, buffer, BUF_SIZE, 0, NULL, NULL);
        if (bytes < 0) 
        {
            perror("recvfrom");
            continue;
        }
        
        if (bytes < (ssize_t)(ETH_HLEN + sizeof(struct iphdr) + sizeof(struct udphdr))) continue;
        
        struct iphdr *recv_ip = (struct iphdr*)(buffer + ETH_HLEN);
        unsigned int recv_ip_header_len = recv_ip->ihl * 4;
        struct udphdr *udp_recv = (struct udphdr*)(buffer + ETH_HLEN + recv_ip_header_len);
        
        if (recv_ip->protocol != IPPROTO_UDP || 
            ntohs(udp_recv->source) != SERVER_PORT || 
            ntohs(udp_recv->dest) != CLIENT_PORT) continue;
        
        int data_len = bytes - ETH_HLEN - recv_ip_header_len - sizeof(struct udphdr);
        char* recv_data = buffer + ETH_HLEN + recv_ip_header_len + sizeof(struct udphdr);
        recv_data[data_len] = '\0';
        
        printf("Server: %s\n", recv_data);
    }

    close(sock_fd);

    return 0;
}

//=============================================================================
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
//=============================================================================
int parse_mac(const char *str, unsigned char *mac)
{
    return sscanf(str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
            &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) == 6;
}