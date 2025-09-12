#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

int main()
{
    sigset_t set;
    int ret;

    sigemptyset(&set);
    sigaddset(&set, SIGINT);

    ret = sigprocmask(SIG_BLOCK, &set, NULL);
    if (ret != 0)
    {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }
    printf("SIGINT is blocked. PID=%d\n", getpid());

    while (1)
    {
        pause();
    }
    return 0;
}