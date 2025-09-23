#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define SHM_NAME "/chat_shm"
#define MAX_CLIENTS 10
#define MAX_MESSAGES 50
#define MAX_NAME_LEN 32
#define MAX_MSG_LEN 256

typedef struct 
{
    char name[MAX_NAME_LEN];
    char text[MAX_MSG_LEN];
} chat_message_t;

typedef struct 
{
    char name[MAX_NAME_LEN];
    int active;
} client_t;

typedef struct 
{
    sem_t sem;  // семафор для синхронизации доступа
    client_t clients[MAX_CLIENTS];
    int client_count;
    chat_message_t messages[MAX_MESSAGES];
    int msg_start;
    int msg_count;
} chat_room_t;

chat_room_t *chat_room = NULL;

int add_client(const char* name) 
{
    sem_wait(&chat_room->sem);
    if (chat_room->client_count >= MAX_CLIENTS) 
    {
        sem_post(&chat_room->sem);
        return -1; // больше нет места
    }
    for (int i = 0; i < MAX_CLIENTS; i++) 
    {
        if (!chat_room->clients[i].active) 
        {
            strncpy(chat_room->clients[i].name, name, MAX_NAME_LEN - 1);
            chat_room->clients[i].name[MAX_NAME_LEN - 1] = '\0';
            chat_room->clients[i].active = 1;
            chat_room->client_count++;
            sem_post(&chat_room->sem);
            return i; // индекс клиента
        }
    }
    sem_post(&chat_room->sem);
    return -1;
}

void remove_client(int client_index) 
{
    sem_wait(&chat_room->sem);
    if (client_index >= 0 && client_index < MAX_CLIENTS && chat_room->clients[client_index].active) 
    {
        chat_room->clients[client_index].active = 0;
        chat_room->client_count--;
    }
    sem_post(&chat_room->sem);
}

void add_message(const char* name, const char* text) 
{
    sem_wait(&chat_room->sem);
    int pos = (chat_room->msg_start + chat_room->msg_count) % MAX_MESSAGES;
    strncpy(chat_room->messages[pos].name, name, MAX_NAME_LEN - 1);
    chat_room->messages[pos].name[MAX_NAME_LEN - 1] = '\0';
    strncpy(chat_room->messages[pos].text, text, MAX_MSG_LEN - 1);
    chat_room->messages[pos].text[MAX_MSG_LEN - 1] = '\0';

    if (chat_room->msg_count < MAX_MESSAGES)
        chat_room->msg_count++;
    else
        chat_room->msg_start = (chat_room->msg_start + 1) % MAX_MESSAGES;

    sem_post(&chat_room->sem);
}

void print_clients() 
{
    printf("Active clients (%d):\n", chat_room->client_count);
    for (int i = 0; i < MAX_CLIENTS; i++) 
    {
        if (chat_room->clients[i].active) 
        {
            printf("%s\n", chat_room->clients[i].name);
        }
    }
}

int main() 
{
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) 
    {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, sizeof(chat_room_t)) == -1) 
    {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    chat_room = mmap(NULL, sizeof(chat_room_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (chat_room == MAP_FAILED) 
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Инициализация структуры
    sem_init(&chat_room->sem, 1, 1);
    chat_room->client_count = 0;
    chat_room->msg_start = 0;
    chat_room->msg_count = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) 
    {
        chat_room->clients[i].active = 0;
        chat_room->clients[i].name[0] = '\0';
    }

    printf("Chat server started with shared memory and semaphore.\n");

    while (1) 
    {
        sem_wait(&chat_room->sem);
        if (chat_room->client_count > 0) 
        {
            print_clients();
        }
        sem_post(&chat_room->sem);
        sleep(5);
    }

    // Очистка (после выхода)
    munmap(chat_room, sizeof(chat_room_t));
    shm_unlink(SHM_NAME);

    return 0;
}
