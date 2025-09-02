#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>

int main()
{
    pid_t proc1, proc2, proc3, proc4, proc5;
    int status;
    
    //Первый форк, дочерний процесс 
    proc1 = fork();
    if (proc1 == 0)
    {
        proc3 = fork();
        if (proc3 == 0)
        {
            printf("Proc3 pid = %d, ppid = %d\n", getpid(), getppid());
            exit(3);
        }

        proc4 = fork();
        if (proc4 == 0)
        {
            printf("Proc4 pid = %d, ppid = %d\n", getpid(), getppid());
            exit(4); 
        }
        // Ждем завершения Проц3 и Проц4
        printf("Proc1 pid = %d, ppid = %d\n", getpid(), getppid());
        waitpid(proc3, &status, 0);
        waitpid(proc4, &status, 0);
        exit(1);
    }

    // Родительский процесс от первого форка
    else
    {
        proc2 = fork();
        if(proc2 == 0)
        {
            proc5 = fork();
            if (proc5 == 0)
            {
                printf("Proc5 pid = %d, ppid = %d\n", getpid(), getppid());
                exit(5);            
            }
            // Проц2 ждет завершения Проц5
            printf("Proc2 pid = %d, ppid = %d\n", getpid(), getppid());
            waitpid(proc5, &status, 0);
            exit(2);
        }
        // Родитель ждет завершения Проц1 и Проц2
        else
        {
            printf("Parent pid = %d, ppid = %d\n", getpid(), getppid());
            waitpid(proc1, &status, 0);
            waitpid(proc2, &status, 0);
        }
    }
}
