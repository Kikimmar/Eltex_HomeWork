#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>

int main()
{
    pid_t child_pid;
    int status;

    child_pid = fork();

    if (child_pid == 0)
    {
        printf("Child pid = %d\nChild ppid = %d\n", getpid(), getppid());
        exit(5);
    }
    else
    {
        printf("Parent pid = %d\nParent ppid = %d\n", getpid(), getppid());
        wait(&status);
        printf("Status = %d\n", WEXITSTATUS(status));
    }
    return 0;
}
