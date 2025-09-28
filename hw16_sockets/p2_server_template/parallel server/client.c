#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12345
#define THREAD_COUNT 1000
#define BUFFER_SIZE 128

void *client_thread(void *arg);

int main()
{
    pthread_t threads[THREAD_COUNT];

    // Creating multiple threads for parallel connections
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        if (pthread_create(&threads[i], NULL, client_thread, NULL) != 0)
        {
            perror("pthread_create");
        }
        usleep(10000);
    }

    // Waiting for all threads complete
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        pthread_join(threads[i], NULL);
    }
    return 0;
}

// A flow function that connects to the server and requests the time
void *client_thread(void *arg)
{
    (void)arg;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        perror("socket");
        pthread_exit(NULL);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(sock);
        pthread_exit(NULL);
    }

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == 0)
    {
        // Successfuly connected, read the time
        char buffer[BUFFER_SIZE];
        ssize_t len = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (len > 0)
        {
            buffer[len] = '\0';
            printf("Thread %lu got time: %s", pthread_self(), buffer);
        }
        else
        {
            printf("Thread %lu: Error reading from the server\n", pthread_self());
        }
    }
    else
    {
        printf("Thread %lu: Couldn't connect to the server\n", pthread_self());
    }
    close(sock);
    pthread_exit(NULL);
}
