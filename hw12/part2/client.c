#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

int main()
{
    const char *fifo = "/tmp/myfifo";
    char buf[100];

    //Открываем FIFO для чтения
    int fd = open(fifo, O_RDONLY);
    if (fd == -1)
        {
            perror("open");
            exit(1);
        }
    //Читаем строку из FIFO
    ssize_t len = read(fd, buf, sizeof(buf) - 1);
    if (len == -1)
    {
        perror("read");
        close(fd);
        exit(1);
    }

    buf[len] = '\0';
    printf("Client resived: %s\n", buf);
    close(fd);
    
    //Удаляем FIFO (именованный канал)
    if (unlink(fifo) == -1)
    {
        perror("unlink");
    }
    return 0;
}
