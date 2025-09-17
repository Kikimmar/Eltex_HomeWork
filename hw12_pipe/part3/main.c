#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_ARGS 20
#define BUFFER_SIZE 100

// Функция для разбора строки на аргументы (отделение по пробелам)
void parser(char *input, char **args);

int main()
{
    pid_t child_pid1, child_pid2;
    char input[BUFFER_SIZE];
    char *args1[MAX_ARGS]; // Аргументы для команды до '|'
    char *args2[MAX_ARGS]; // Аргументы для команды после '|'

    while (1)
    {
        printf("MyShell-->Enter command: ");
        if (fgets(input, sizeof(input), stdin) == NULL)
            break;
        
        // Удаляем символ новой строки
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "exit") == 0)
        {
            printf("MyShell-->Exiting the terminal...\n");
            break;
        }

        // Проверяем, есть ли в команде конвейер '|'
        char *pipe_pos = strchr(input, '|');

        if (pipe_pos != NULL)
        {
            // Разделяем строку на две части: до и после '|'

            // Заменяем '|' на нулевой символ, чтобы получить первую команду
            *pipe_pos = '\0';
            char *cmd1 = input;           // до '|'
            char *cmd2 = pipe_pos + 1;    // после '|'

            // Убираем пробелы в начале cmd2, чтобы избежать проблем с разбором
            while (*cmd2 == ' ')
                cmd2++;

            // Разбираем обе команды на аргументы
            parser(cmd1, args1);
            parser(cmd2, args2);

            int fd[2];  // файловые дескрипторы канала: fd[0] - чтение, fd[1] - запись

            if (pipe(fd) == -1)  // создаём канал
            {
                perror("MyShell-->Pipe creation failed");
                continue;
            }

            // Форкаем первый процесс - команда слева, пишет в канал
            child_pid1 = fork();
            if (child_pid1 < 0)
            {
                perror("MyShell-->Fork failed");
                continue;
            }

            if (child_pid1 == 0)
            {
                // Ребёнок 1: перенаправляем stdout на запись в канал
                close(fd[0]);           // Закрываем неиспользуемое чтение
                dup2(fd[1], STDOUT_FILENO);  // standart output = запись в канал
                close(fd[1]);           // закроем старый дескриптор, он дублирован

                if (execvp(args1[0], args1) == -1)
                {
                    fprintf(stderr, "MyShell-->Command not found: %s\n", args1[0]);
                    exit(EXIT_FAILURE);
                }
            }

            // Форкаем второй процесс - команда справа, читает из канала
            child_pid2 = fork();
            if (child_pid2 < 0)
            {
                perror("MyShell-->Fork failed");
                continue;
            }

            if (child_pid2 == 0)
            {
                // Ребёнок 2: перенаправляем stdin с чтения из канала
                close(fd[1]);           // Закрываем неиспользуемую запись
                dup2(fd[0], STDIN_FILENO);   // standard input = чтение из канала
                close(fd[0]);           // закрываем старый дескриптор, он дублирован

                if (execvp(args2[0], args2) == -1)
                {
                    fprintf(stderr, "MyShell-->Command not found: %s\n", args2[0]);
                    exit(EXIT_FAILURE);
                }
            }

            // Родитель закрывает оба конца канала, так как процессы используют их сами
            close(fd[0]);
            close(fd[1]);

            // Ждём оба дочерних процесса
            waitpid(child_pid1, NULL, 0);
            waitpid(child_pid2, NULL, 0);
        }
        else
        {
            // В команде нет '|', просто запускаем как обычно
            parser(input, args1);

            pid_t child_pid = fork();
            if (child_pid == 0)
            {
                if (execvp(args1[0], args1) == -1)
                {
                    fprintf(stderr, "MyShell-->Command not found: %s\n", args1[0]);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                wait(NULL);
            }
        }
    }
    return 0;
}

void parser(char *input, char **args)
{
    int i = 0;
    char *token = strtok(input, " ");
    while ((token != NULL) && (i < MAX_ARGS - 1))
    {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;  // последний аргумент - NULL по спецификации execvp
}
