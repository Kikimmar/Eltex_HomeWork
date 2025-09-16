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

    //Create mqueue
    if ((key = ftok("/tmp", 'S')) == -1)
    {
        perror("Server:ftok");
        exit(1);
    }

    msgid = msgget(key, IPC_CREAT | 0644);
    if (msgid == -1)
    {
        perror("Server:msgget");
        exit(1);
    }
    printf("Server: Message queue created.\n");

    //Send message to client
    msg.mtype = 1;
    strncpy(msg.mtext, "Hi!", MAX_SIZE - 1);
    msg.mtext[MAX_SIZE - 1] = '\0';

    if (msgsnd(msgid, &msg, strlen(msg.mtext) + 1, 0) == -1)
    {
        perror("Server: message");
    }
    else
    {
        printf("Server: Sent message 'Hi' to client\n");
    }

    //Recieve message from client
    msg_len = msgrcv(msgid, &msg, MAX_SIZE, 2, 0);
    if (msg_len == -1)
    {
        perror("Server: msgrcv");
    }
    else
    {
        msg.mtext[msg_len] = '\0';
        printf("Server: recieved message from client: '%s'\n", msg.mtext);
    }

    //Delete mqueue
    if (msgctl(msgid, IPC_RMID, NULL) == -1)
    {
        perror("Server: msgctl(IPC_RMID)");
        exit(1);
    }
    printf("Server: mqueue removed.\n");

    return 0;
}