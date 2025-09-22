#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>

#define SHM_SIZE 1024

int main()
{
    key_t key;
    int shmid;
    char *shmaddr;

    // Генерация ключа
    if ((key = ftok("/tmp", 'S')) == -1)
    {
        perror("Server:ftok");
        exit(1);
    }

    // Создание сегмента разделяемой памяти
    if ((shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0666)) == -1)
    {
        perror("Server:shmget");
        exit(1);
    }
    printf("Server:shared memory segment created\n");

    // Подключение к сегменту
    if ((shmaddr = (char*)shmat(shmid, NULL, 0)) == (char*)-1)
    {
        perror("Server:shmat");
        exit(1);
    }

    // Запись сообщения "Hi" в разделяемую память
    strncpy(shmaddr, "Hi!", SHM_SIZE - 1);
    shmaddr[SHM_SIZE - 1] = '\0';

    printf("Server: Sent message 'Hi!'. Waiting for reply...\n ");

    // Ждем ответ от клиента
    // ждем пока сообщение не сменится на "Hello!"
    while (strncmp(shmaddr, "Hello!", SHM_SIZE) != 0)
    {
        sleep(1);
    }
    printf("Server: Received reply from client: '%s'\n", shmaddr);

    // Отсоединяемся от сегмента разделяемой памяти
    if (shmdt(shmaddr) == -1)
    {
        perror("Server:shmdt");
        exit(1);
    }

    // Удаление сегмента
    if (shmctl(shmid, IPC_RMID, NULL) == -1)
    {
        perror("Server:shmctl");
        exit(1);
    }
    printf("Server: Shared memory segment removed.\n");

}