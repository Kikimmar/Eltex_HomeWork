#include <stdio.h>
#include <stdlib.h>
#include <ptreads.h>
#include <time.h>

int main()
{
    int shop[5];
    srand(time(NULL));
    for (int i = 0; i < 5; i++)
        shop[i] = rand() % (600 - 400 + 1) + 400;

    pthread_t shoper[4];
    for (int i = 0; i < 3; i++)
        pthread_create(&shoper[i], NULL, , NULL);
    pthread_create(&shoper[3], NULL, < NULL);

    return 0;
}
