#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

int main()
{
    sigset_t set;
    int sig;

    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);

    if (sigprocmask(SIG_BLOCK, &set, NULL) == -1)
    {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }
    while (1)
    {
        if (sigwait(&set, &sig) != 0)
        {
            perror("sigwait");
            exit(EXIT_FAILURE);
        }
        printf("Received signal: %d (SIGUSR1)\n", sig);
    }
    return 0;
}
