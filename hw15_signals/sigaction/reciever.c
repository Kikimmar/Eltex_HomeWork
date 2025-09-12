#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

void sig_usr1_handler(int sig)
{
    (void)sig;
    printf("Reseived SIGUSR1 signal!\n");
}

int main()
{
    struct sigaction handler;

    handler.sa_handler = sig_usr1_handler;
    sigemptyset(&handler.sa_mask);
    handler.sa_flags = 0;

    if (sigaction(SIGUSR1, &handler, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for SIGUSR1 signal. PID: %d\n", getpid());

    while(1)
    {
        pause();
    }
    return 0;
}