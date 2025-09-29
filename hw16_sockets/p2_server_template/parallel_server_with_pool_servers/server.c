#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <errno.h>

#define PORT 12345
#define BACKLOG 10
#define WORKER_MAX 5
#define BUFFER_SIZE 64

typedef struct 
{
    pid_t pid;
    int comm_fd;
    int busy;
} worker_t;

worker_t workers[WORKER_MAX];
int worker_count = 0;

//=============================================================================
void get_time(char *buffer, size_t size);
void worker_loop(int comm_fd);
int spawn_worker(int index);
int find_free_worker();
int send_fd(int socket, int fd_to_send);
int recv_fd(int socket);

//=============================================================================
int main()
{
    int server_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, BACKLOG) < 0)
    {
        perror("listen");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Listening server start on potr: %d\n", PORT);

    // Initialize the firs several workers
    int initial_workers = 2;
    for (int i = 0; i < initial_workers; i++)
    {
        if (spawn_worker(i) < 0)
        {
            fprintf(stderr, "Failed to spawn worker %d\n", i);
            exit(EXIT_FAILURE);
        }
    }

    // The main loop server/ Connecting the clients and transfer to the workers
    while (1)
    {
        int client_fd = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
        if (client_fd < 0)
        {
            perror("accept");
            continue;
        }
        printf("New client: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        int idx = find_free_worker();

        // If there are no free workers, but you can create more, we create a new one.
        if (idx < 0 && worker_count < WORKER_MAX)
        {
            idx = worker_count;
            if (spawn_worker(idx) < 0)
            {
                close(client_fd);
                continue;
            } 
        }

        // If there are no free workers and the maximum number is reached,
        // we refuse the client.
        if (idx < 0)
        {
            printf("All workers busy, rejecting connection\n");
            close(client_fd);
            continue;
        }

        // We transfer the client socket to the worker via unix socketpair
        // with the transfer of the descriptor
        if (send_fd(workers[idx].comm_fd, client_fd) < 0)
        {
            perror("send_fd to worker");
            close(client_fd);
            workers[idx].busy = 0;
            continue;
        }

        workers[idx].busy = 1;
        close(client_fd);

        // We check the messages from the workers that they are free
        for  (int i = 0; i < worker_count; i++)
        {
            if (workers[i].busy)
            {
                char status;
                ssize_t r = read(workers[i].comm_fd, &status, 1);
                if (r == 1 && status == '1')
                    {
                        // The worker is free and ready to accept a new client.
                        workers[i].busy = 0;
                    }
            }
        }
    }
    close(server_sock);
    return 0;
}

//=============================================================================
// We get the current date and time in string format
void get_time(char *buffer, size_t size)
{
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S\n", tm_now);
}

//=============================================================================
// The main worker cycle receives client sockets through unix socket
// and processes them
void worker_loop(int comm_fd)
{
    while (1)
    {
        // We accept the client descriptor from the main server
        int client_fd = recv_fd(comm_fd);
        if (client_fd < 0)
        {
            // Error
            break;
        }
        char buffer[BUFFER_SIZE];
        get_time(buffer, sizeof(buffer));

        // Send to the client the current time and date
        send(client_fd, buffer, strlen(buffer), 0);
 
        close(client_fd);

        // We send a signal to the main server that
        // the worker is free ('1' - free)
        char status = '1';
        write(comm_fd, &status, 1);
    }
    close(comm_fd);
    exit(0);
}

//=============================================================================
//Creating a worker process and a unix socketpair to communicate with it
int spawn_worker(int index)
{
    int sv[2]; // socketpair for bidirectional communication between server and worker
    
    // Creating a socketpair of the AF_UNIX domain,
    //  the SOCK_DGRAM type for transmitting fd
    if (socketpair(AF_UNIX, SOCK_DGRAM, 0, sv) == -1)
    {
        perror("socketpair");
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork");
        close(sv[0]);
        close(sv[1]);
        return -1;
    }
    if (pid == 0)
    {
       // In the child process, we close the server side
       close(sv[0]);
       // Start the worker loop
       worker_loop(sv[1]);
       exit(0);
    }
    // In the parent process, we close the worker's side
    close(sv[1]);

    workers[index].pid = pid;
    workers[index].comm_fd = sv[0];
    workers[index].busy = 0;

    worker_count++;
    return 0;
}
 
//=============================================================================
// A function for sending a file descriptor via a unix domain socket
int send_fd(int socket, int fd_to_send)
{
    struct msghdr msg = {0};
    char buf[CMSG_SPACE(sizeof(fd_to_send))], dummy = '*';
    struct iovec io = { .iov_base = &dummy, .iov_len = sizeof(dummy) };

    msg.msg_iov = &io;
    msg.msg_iovlen = 1;

    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);
    
    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(fd_to_send));

    *((int *) CMSG_DATA(cmsg)) = fd_to_send;

    msg.msg_controllen = cmsg->cmsg_len;

    if (sendmsg(socket, &msg, 0) < 0)
    {
        perror("sendmsg");
        return -1;
    }
    return 0;
}

//=============================================================================
// A function for receiving a file descriptor via a unix domain socket
int recv_fd(int socket)
{
    struct msghdr msg = {0};
    char m_buffer[1], c_buffer[CMSG_SPACE(sizeof(int))];
    struct iovec io = { .iov_base = m_buffer, .iov_len = sizeof(m_buffer) };
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;

    msg.msg_control = c_buffer;
    msg.msg_controllen = sizeof(c_buffer);

    if (recvmsg(socket, &msg, 0) < 0)
    {
        perror("recvmsg");
        return -1;
    }

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);

    if (cmsg == NULL || cmsg->cmsg_type != SCM_RIGHTS)
    {
        fprintf(stderr, "No file descriptor received\n");
        return -1;
    }

    int fd;
    memcpy(&fd, CMSG_DATA(cmsg), sizeof(fd));
    return fd;
}

//=============================================================================
// Search for a free worker (busy == 0). If not, return -1
int find_free_worker()
{
    for (int i = 0; i < worker_count; i++)
    {
        if (workers[i].busy == 0)
            return i;
    }
    return -1;
}
