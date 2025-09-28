#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 12345
#define BACKLOG 10
#define BUFFER_SIZE 64

void *client_handler(void *arg);

int main()
{
    int server_sock, *client_sock_ptr;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Create the TCP-socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Port remmaping
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Filling in the server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Associating the socket with the address
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    //Starting to listen for incoming connections
    if (listen(server_sock, BACKLOG) < 0)
    {
        perror("listen");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", PORT);

    while(1)
    {
        // Accepting a new connection
        int *client_sock = malloc(sizeof(int));
        if (!client_sock)
        {
            perror("malloc");
            continue;
        }

        *client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
        if (*client_sock < 0)
        {
            perror("accept");
            free(client_sock);
            continue;
        }

        printf("Connected client: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // The flow for customer service
        pthread_t tid;
        if (pthread_create(&tid, NULL, client_handler, client_sock) !=0)
        {
            perror("pthread_create");
            close(*client_sock);
            free(client_sock);
            continue;
        }

        // Disconnecting the stream
        pthread_detach(tid);
    }

    close(server_sock);
    return 0;
}

// The client-handler thread
void *client_handler(void *arg)
{
    int client_sock = *(int*)arg;
    free(arg);

    char buffer[BUFFER_SIZE];

    // Get the current time as a string
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S\n", tm_now);

    // Sending the time to the client
    send(client_sock, buffer, strlen(buffer), 0);

    close(client_sock);
    pthread_exit(NULL);
}
