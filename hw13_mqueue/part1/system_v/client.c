#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

#define MAX_SIZE 1024

struct message
{
    long mtype;
    char mtext[MAX_SIZE];
};

int main()
{
    int msgid;
    struct message msg;
    ssize_t msg_len;
    key_t key;

    //Access mqueue
     if ((key = ftok("/tmp", 'S')) == -1)
    {
        perror("Server:ftok");
        exit(1);
    }

    msgid = msgget(key, 0644);
    if (msgid == -1)
    {
        perror("Client: msgget");
        exit(1);
    }
    printf("Client: mqueue accessed.\n");

    //Recieve message from server
    msg_len = msgrcv(msgid, &msg, MAX_SIZE, 1, 0);
    if (msg_len == -1)
    {
        perror("Client: msgrcv");
        exit(1);
    }
    msg.mtext[msg_len] = '\0';
    printf("Client: Recvieved message from server: '%s'\n", msg.mtext);

    //Send reply message
    msg.mtype = 2;
    strncpy(msg.mtext, "Hello!", MAX_SIZE - 1);
    msg.mtext[MAX_SIZE - 1] = '\0';

    if (msgsnd(msgid, &msg, strlen(msg.mtext) + 1, 0) == -1)
    {
        perror("Client: message");
        exit(1);
    }
    else
    {
        printf("Client: Sent message 'Hello!' to server\n");
    }
    return 0;
}