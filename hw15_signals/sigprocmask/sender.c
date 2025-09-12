#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

int main(int argc, char *argv[])
{
    if (argc != 2 )
    {
        fprintf(stderr, "Usage: %s <PID>\n", argv[0]);
        return EXIT_FAILURE;
    }

    pid_t pid = (pid_t)atoi(argv[1]);
    if (kill(pid, SIGINT) == -1)
    {
        perror("kill");
        return EXIT_FAILURE;
    }
    printf("Sent SIGINT to process %d\n", pid);
    return EXIT_SUCCESS;
}