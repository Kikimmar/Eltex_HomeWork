#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>

#define SHM_SIZE 1024

int main()
{
    key_t key;
    int shmid;
    char * shmaddr;

    // Генерация ключа
    if ((key = ftok("/tmp", 'S')) == -1)
    {
        perror("Client:ftok");
        exit(1);
    }

    // Получение доступа
    if ((shmid = shmget(key, SHM_SIZE, 0666)) == -1)
    {
        perror("Client:shmget");
        exit(1);
    }
    printf("Client: Accessed shared memory segment\n");

    // Подключение к сегменту
    if ((shmaddr = (char*)shmat(shmid, NULL, 0)) == (char*)-1)
    {
        perror("Client:shmat");
        exit(1);
    }

    // Чтение сообщения от сервера
    printf("Client: Received message from server: '%s'\n", shmaddr);

    // Запись ответа "Hello!" в разделяемую память
    strncpy(shmaddr, "Hello!", SHM_SIZE - 1);
    shmaddr[SHM_SIZE - 1] = '\0';

    printf("Client: Sent message 'Hello!'\n");

    // Отсоединение
    if (shmdt(shmaddr) == -1)
    {
        perror("Client:shmdt");
        exit(1);
    }
    return 0;
}