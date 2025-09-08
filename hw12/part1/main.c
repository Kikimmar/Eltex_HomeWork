#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

int main() 
{
    int pipe1[2]; // сервер -> клиент
    int pipe2[2]; // клиент -> сервер
    pid_t pid;
    char buf[10];

    if (pipe(pipe1) == -1) 
    {
        perror("pipe1");
        exit(1);
    }
    if (pipe(pipe2) == -1) 
    {
        perror("pipe2");
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
        close(pipe2[0]); // закрываем чтение во втором канале

        read(pipe1[0], buf, sizeof(buf)); // читаем "Hello"
        printf("Client received: %s\n", buf);

        const char *msg = "Hi";
        write(pipe2[1], msg, strlen(msg) + 1); // отправляем "Hi"
        close(pipe1[0]);
        close(pipe2[1]);
        exit(0);
    } 
    else 
    { // сервер
        close(pipe1[0]); // закрываем чтение в первом канале
        close(pipe2[1]); // закрываем запись во втором канале

        const char *msg = "Hello";
        write(pipe1[1], msg, strlen(msg) + 1); // отправляем "Hello"
        wait(NULL); // ждем завершения клиента

        read(pipe2[0], buf, sizeof(buf)); // читаем "Hi"
        printf("Server received: %s\n", buf);

        close(pipe1[1]);
        close(pipe2[0]);
    }
    return 0;
}

