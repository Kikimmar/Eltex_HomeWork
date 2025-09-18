#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <unistd.h>
#include <pthread.h>

#define CHAT_QUEUE "/chat_queue"
#define MAX_MESSAGE_SIZE 256
#define MAX_CLIENTS 10

typedef struct
{
    int type;               // 0 - регистрация, 1 - сообщение, 2 - список клиентов
    char name[32];
    char queue_name[64];
    char text[MAX_MESSAGE_SIZE];
} chat_message_t;

typedef struct
{
    char name[32];
    char queue_name[64];
    mqd_t dqueue;
} client_info_t;

client_info_t clients[MAX_CLIENTS];

int client_count = 0;

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
//=============================================================================
int add_client(const char* name, const char* queue_name)
{
    if (client_count >= MAX_CLIENTS)
        return -1;

    mqd_t qd = mq_open(queue_name, O_WRONLY);
    if (qd == (mqd_t)-1)
    {
        perror("mq_open client queue");
        return -1;
    }

    pthread_mutex_lock(&clients_mutex);
    strncpy(clients[client_count].name, name,
            sizeof(clients[client_count].name) - 1);
    strncpy(clients[client_count].queue_name, queue_name,
            sizeof(clients[client_count].queue_name) - 1);
    clients[client_count].dqueue = qd;
    client_count++;
    pthread_mutex_unlock(&clients_mutex);

    return 0;
}
//=============================================================================
void broadcast_new_clients()
{
    pthread_mutex_lock(&clients_mutex);

    chat_message_t msg = {0};
    msg.type = 2; // type 2 - список клиентов

    msg.text[0] = '\0';
    for (int i = 0; i < client_count; i++)
    {
        strncat(msg.text, clients[i].name, sizeof(msg.text) - strlen(msg.text) - 2);
        strncat(msg.text, "\n", sizeof(msg.text) - strlen(msg.text) - 1);
    }
    strncpy(msg.name, "Server", sizeof(msg.name) - 1);

    for (int i = 0; i < client_count; i++)
    {
        mq_send(clients[i].dqueue, (const char*)&msg, sizeof(msg), 0);
    }

    pthread_mutex_unlock(&clients_mutex);
}
//=============================================================================
void broadcast_message(const char *sender, const char *text)
{
    chat_message_t msg = {0};
    msg.type = 1;
    strncpy(msg.name, sender, sizeof(msg.name) - 1);
    strncpy(msg.text, text, sizeof(msg.text) - 1);

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++)
    {
        mq_send(clients[i].dqueue, (const char*)&msg, sizeof(msg), 0);
    }
    pthread_mutex_unlock(&clients_mutex);
}
//=============================================================================
int main()
{
    struct mq_attr attr = {0};
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(chat_message_t);

    mq_unlink(CHAT_QUEUE);  // unlink старой очереди

    mqd_t server_mq = mq_open(CHAT_QUEUE, O_CREAT | O_RDONLY, 0666, &attr);
    if (server_mq == (mqd_t)-1)
    {
        perror("mq_open server");
        exit(EXIT_FAILURE);
    }

    printf("Chat server started\n");

    while (1)
    {
        chat_message_t msg;
        ssize_t bytes = mq_receive(server_mq, (char*)&msg, sizeof(msg), NULL);
        if (bytes > 0)
        {
            if (msg.type == 0) // регистрация
            {
                if (add_client(msg.name, msg.queue_name) == 0)
                {
                    printf("Registered client '%s', queue '%s'\n", msg.name, msg.queue_name);
                    broadcast_new_clients();  // рассылка обновленного списка
                }
                else
                {
                    fprintf(stderr, "Failed to add client '%s'\n", msg.name);
                }
            }
            else if (msg.type == 1) // сообщение
            {
                printf("[%s]: %s\n", msg.name, msg.text);
                broadcast_message(msg.name, msg.text);  // рассылка сообщений клиентам
            }
            else
            {
                perror("mq_receive server");
            }
        }
    }

    mq_close(server_mq);
    mq_unlink(CHAT_QUEUE);
    return 0;
}
