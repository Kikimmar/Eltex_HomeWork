#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>      
#include <sys/stat.h>
#include <mqueue.h>
#include <errno.h>

#define QUEUE_NAME_S2C "/srv_to_cli"
#define QUEUE_NAME_C2S "/cli_to_srv"
#define MAX_SIZE 1024

int main() 
{
    mqd_t mq_s2c, mq_c2s;
    char buffer[MAX_SIZE];
    unsigned int prio;

    // Opening mqueue server-client
    mq_s2c = mq_open(QUEUE_NAME_S2C, O_RDONLY);
    if (mq_s2c == (mqd_t) -1) 
    {
        perror("Client: mq_open srv_to_cli");
        mq_close(mq_s2c);
        exit(1);
    }

    // Opening mqueue client-server
    mq_c2s = mq_open(QUEUE_NAME_C2S, O_WRONLY);
    if (mq_c2s == (mqd_t) -1) 
    {
        perror("Client: mq_open cli_to_srv");
        mq_close(mq_c2s);
        exit(1);
    }

    // Reading the message from server
    ssize_t bytes_read = mq_receive(mq_s2c, buffer, MAX_SIZE, &prio);
    if (bytes_read >= 0) 
    {
        buffer[bytes_read] = '\0';
        printf("Client: Received message from server: '%s'\n", buffer);
    } 
    else 
    {
        perror("Client: mq_receive");
    }

    // Sending a response to the server
    if (mq_send(mq_c2s, "Hello!", strlen("Hello!") + 1, 0) == -1) 
    {
        perror("Client: mq_send");
    }
    else 
    {
        printf("Client: Sent message 'Hello!' to server\n");
    }

    mq_close(mq_s2c);
    mq_close(mq_c2s);
    return 0;
}
