#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>      // For O_* constants
#include <sys/stat.h>   // For mode constants
#include <mqueue.h>
#include <errno.h>

#define QUEUE_NAME_S2C "/srv_to_cli"
#define QUEUE_NAME_C2S "/cli_to_srv"
#define MAX_SIZE 1024

int main() 
{
    mqd_t mq_s2c, mq_c2s;
    struct mq_attr attr;

    char buffer[MAX_SIZE];
    unsigned int prio;

    // Message Queue Attributes
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_SIZE;
    attr.mq_curmsgs = 0;

    // Сreating a mqueue for server-client
    mq_s2c = mq_open(QUEUE_NAME_S2C, O_CREAT | O_WRONLY, 0644, &attr);
    if (mq_s2c == (mqd_t) -1) 
    {
        perror("Server: mq_open srv_to_cli");
        mq_close(mq_s2c);
        mq_unlink(QUEUE_NAME_C2S);
        exit(1);
    }

    // Creating a mqueue for client-server
    mq_c2s = mq_open(QUEUE_NAME_C2S, O_CREAT | O_RDONLY, 0644, &attr);
    if (mq_c2s == (mqd_t) -1) 
    {
        perror("Server: mq_open cli_to_srv");
        mq_close(mq_c2s);
        mq_unlink(QUEUE_NAME_S2C);
        exit(1);
    }

    printf("Server: Queues created. Waiting for client...\n");

    // Sending a message for client
    if (mq_send(mq_s2c, "Hi!", strlen("Hi!") + 1, 0) == -1) 
    {
        perror("Server: mq_send");
    }
    printf("Server: Sent message 'Hi!' to client\n");

    // Waiting a response from client
    ssize_t bytes_read = mq_receive(mq_c2s, buffer, MAX_SIZE, &prio);
    if (bytes_read >= 0) 
    {
        buffer[bytes_read] = '\0';
        printf("Server: Received message from client: '%s'\n", buffer);
    } 
    else 
    {
        perror("Server: mq_receive");
    }
    
    // Close and delete mqueues
    mq_close(mq_s2c);
    mq_close(mq_c2s);
    mq_unlink(QUEUE_NAME_S2C);
    mq_unlink(QUEUE_NAME_C2S);

    printf("Server: Queues closed and unlinked. Exiting.\n");
    return 0;
}
