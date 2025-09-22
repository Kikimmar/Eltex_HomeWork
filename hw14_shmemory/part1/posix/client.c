#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define SHM_SIZE 1024

int main()
{
    int shm_fd;
    void *shm_ptr = NULL;
    // Открываем сегмент разд. памяти
    if((shm_fd = shm_open("/myshm", O_RDWR, 0666)) == -1)
    {
        perror("Client:shm_open");
        exit(1);
    }

    // Отображаем в адресное пространство
    if ((shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
    {
        perror("Client:mmap");
        close(shm_fd);
        exit(1);
    }

    // Читаем сообщение от сервера
    printf("Client: received message: '%s'\n", (char*)shm_ptr);

    // Записываем ответ в разделяемую память
    strncpy((char*)shm_ptr, "Hello!", SHM_SIZE - 1);
    ((char*)shm_ptr)[SHM_SIZE - 1] = '\0';
    printf("Client: sent reply '%s'\n", (char*)shm_ptr);

    // Отсоединяем память и закрываем дескриптор
    if (munmap(shm_ptr, SHM_SIZE) == -1)
    {
        perror("Client:munmap");
    }
    close(shm_fd);
    return 0;
}