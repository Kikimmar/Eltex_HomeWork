#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 12345
#define BACKLOG 50
#define BUFFER_SIZE 64
#define THREAD_POOL_SIZE 10

// Queue for storing client descriptors
typedef struct client_queue
{
    int *data;
    int capacity;
    int front;
    int rear;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t cond_nonempty;
    pthread_cond_t cond_nonfull;
} socket_queue_t;

// Queue for a clients (sockets)
socket_queue_t client_queue;

void queue_init(socket_queue_t *q, int capacity);
void queue_destroy(socket_queue_t *q);

// Push socket in the queue, bloks if the queue is full
void queue_push(socket_queue_t *q, int client_sock);

// Pop socket from the queue, bloks if the queue is empty
int queue_pop(socket_queue_t *q);

// Clients handler
void handle_client(int client_sock);

// Work flow function, endlessly waiting for clients from the queue
void *worker_thread(void *ard);

int main()
{
   int server_sock;
   struct sockaddr_in server_addr, client_addr;
   socklen_t addr_len = sizeof(client_addr);

   queue_init(&client_queue, 16);

   // Create workflow
   pthread_t threads[THREAD_POOL_SIZE];
   for (int i = 0; i < THREAD_POOL_SIZE; i++)
   {
       if (pthread_create(&threads[i], NULL, worker_thread, NULL) != 0)
       {
           perror("prthread_create");
           exit(EXIT_FAILURE);
       }
   }

   // Create the server-socket
   server_sock = socket(AF_INET, SOCK_STREAM, 0);
   if (server_sock < 0)
   {
       perror("socket");
       exit(EXIT_FAILURE);
   }

   // Setting up for fast reload the server
   int opt = 1;
   setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

   // Setting up the server address
   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   server_addr.sin_addr.s_addr = INADDR_ANY;
   server_addr.sin_port = htons(PORT);

   // Bind the socket with the adderss
   if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
   {
       perror("bind");
       close(server_sock);
       exit(EXIT_FAILURE);
   }

   // Start listen
   if (listen(server_sock, BACKLOG) < 0)
   {
       perror("listen");
       close(server_sock);
       exit(EXIT_FAILURE);
   }

   printf("Server with thread pool listening on port %d\n", PORT);

   while (1)
   {
       // Accepting a new connection
       int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
       if (client_sock < 0)
       {
           perror("accept");
           continue;
       }

       printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

       queue_push(&client_queue, client_sock);
    }
   
    queue_destroy(&client_queue);
    close(server_sock);

    return 0;
}


void queue_init(socket_queue_t *q, int capacity)
{
    q->capacity = capacity;
    q->data = malloc(sizeof(int) * capacity);
    q->front = 0;
    q->rear = 0;
    q->count = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond_nonempty, NULL);
    pthread_cond_init(&q->cond_nonfull, NULL);
}

void queue_destroy(socket_queue_t *q)
{
    free(q->data);
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond_nonempty);
    pthread_cond_destroy(&q->cond_nonfull);
}

void queue_push(socket_queue_t *q, int client_sock)
{
    pthread_mutex_lock(&q->mutex);
    while (q->count == q->capacity)
    {
        pthread_cond_wait(&q->cond_nonfull, &q->mutex);
    }
    q->data[q->rear] = client_sock;
    q->rear = (q->rear + 1) % q->capacity;
    q->count++;
    pthread_cond_signal(&q->cond_nonempty);
    pthread_mutex_unlock(&q->mutex);
}

int queue_pop(socket_queue_t *q)
{
    pthread_mutex_lock(&q->mutex);
    while(q->count == 0)
    {
        pthread_cond_wait(&q->cond_nonempty, &q->mutex);
    }
    int client_sock = q->data[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->count--;
    pthread_cond_signal(&q->cond_nonfull);
    pthread_mutex_unlock(&q->mutex);
 
    return client_sock;
}

void handle_client(int client_sock)
{
    char buffer[BUFFER_SIZE];

    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S\n", tm_now);

    send(client_sock, buffer, strlen(buffer), 0);

    close(client_sock);
}

void *worker_thread(void *arg)
{
    (void)arg;
    while(1)
    {
        int client_sock = queue_pop(&client_queue);
        if (client_sock >= 0)
        {
            handle_client(client_sock);
        }
    }
    return NULL;
}
