#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

int main()
{
    const char *fifo = "/tmp/myfifo";

    //Создаем именованныйы канал с правами доступа 0666
    if(mkfifo(fifo, 0666) == -1)
    {
        perror("mkfifo");
    }

    //Открываем FIFO только для чтения
    int fd = open(fifo, O_WRONLY);
    if (fd == -1)
    {
        perror("open");
        exit(1);
    }

    //Записываем чтроку "Hi" (3 символа + '\0') - 4 байта
    const char *msg = "Hi!";
    if (write(fd, msg, 4) == -1)
    {
        perror("write");
    }
    close(fd);
    return 0;
}
