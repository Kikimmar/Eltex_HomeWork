#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

int main() 
{
    int pipe1[2]; // сервер -> клиент
    pid_t pid;
    char buf[10];

    if (pipe(pipe1) == -1) 
    {
        perror("pipe1");
        exit(1);
    }

    pid = fork();
    if (pid == -1) 
    {
        perror("fork");
        exit(1);
    }

    if (pid == 0) 
    { // клиент
        close(pipe1[1]); // закрываем запись в первом канале

        read(pipe1[0], buf, sizeof(buf)); // читаем "Hi"
        printf("Client received: %s\n", buf);

        close(pipe1[0]);
        exit(0);
    } 
    else 
    { // сервер
        close(pipe1[0]); // закрываем чтение в первом канале
        
        const char *msg = "Hi";
        write(pipe1[1], msg, strlen(msg) + 1); // отправляем "Hi"
        wait(NULL); // ждем завершения клиента

        close(pipe1[1]);
    }
    return 0;
}

