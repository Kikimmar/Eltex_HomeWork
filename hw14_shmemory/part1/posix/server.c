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

    // Создаем разделяемую память
    if((shm_fd = shm_open("/myshm", O_CREAT | O_RDWR, 0666)) == -1)
    {
        perror("Server:sh_open");
        exit(1);
    }

    // Задаем размер сегмента разд. памяти
    if (ftruncate(shm_fd, SHM_SIZE) == -1)
    {
        perror("Server:ftrumcate");
        shm_unlink("/myshm");
        exit(1);
    }

    // Отображаем память в адресное пространство 
    if ((shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
    {
        perror("Server:mmap");
        shm_unlink("/myshm");
        exit(1);
    }

    // Записываем сообщение в разд. память
    strncpy((char*)shm_ptr, "Hi!", SHM_SIZE - 1);
    ((char*)shm_ptr)[SHM_SIZE - 1] = '\0';

    printf("Server: wrote message '%s'\n", (char*)shm_ptr);

    // Ожидание ответа от клиента
    // Ждем изменения на "Hello!"
    while ((strncmp((char*)shm_ptr, "Hello!", SHM_SIZE)) != 0)
    {
        sleep(1);
    }
    printf("Server: received reply '%s'\n", (char*)shm_ptr);

    // Отсоединяем разд. память
    if (munmap(shm_ptr, SHM_SIZE) == -1)
    {
        perror("Server: munmap");
    }
    // Закрываем дескриптор
    close(shm_fd);

    // Удаляем объект разделяемой памяти
    if (shm_unlink("/myshm") == -1)
    {
        perror("Server: shm_unlink");
    }
    else
    {
        printf("Server:shared memory removed\n");
    }
    return 0;
}