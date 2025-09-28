#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <fcntl.h>

#define PORT 12345
#define BACKLOG 10
#define WORKER_MAX 5
#define BUFFER_SIZE 64

typedef struct
{
    pid_t pid;
    int pipe_fd[2];
    int busy;
} worker_t;

worker_t workers[WORKER_MAX];
int worker_count = 0;

//=============================================================================
void get_time(char *buffer, size_t size);
void worker_loop(int read_fd, int write_fd);
int spawn_worker(int index);
int find_free_worker();

//=============================================================================
int main()
{
    int server_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Create the server-socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Settings for fast realod
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Setting up the address
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

    printf("Listening server start on port %d\n", PORT);

    // Creatin a minimal number of support servers
    int initial_workers = 2;
    for (int i = 0; i < initial_workers; i++)
    {
        if (spawn_worker(i) < 0)
        {
            fprintf(stderr, "Failed to spawn worker %d\n", i);
            exit(EXIT_FAILURE);
        }
    }

    // Main loop the listen-server
    while (1)
    {
        int client_fd = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
        if (client_fd < 0)
        {
            perror("accept");
            continue;
        }
        printf("New client: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        // Searching the free support-server
        int idx = find_free_worker();

        if (idx < 0 && worker_count < WORKER_MAX)
        {
            // There is no dree one, but can create a new one
            idx = worker_count;
            if (spawn_worker(idx) < 0)
            {
                close(client_fd);
                continue;
            }
        }

        if (idx < 0)
        {
            printf("All workers busy, rejecting connections\n");
            close(client_fd);
            continue;
        }
        
        // Transfer the client;s socket to the support-server
        // via pipe. Sending the fd
        if (write(workers[idx].pipe_fd[1], &client_fd, sizeof(client_fd)) != sizeof(client_fd))
        {
            perror("write to worker pipe");
            close(client_fd);
            workers[idx].busy = 0;
            continue;
        }

        workers[idx].busy = 1;

        // Checking whether any shutdown signals have come from the support-server
        for (int i = 0; i < worker_count; i++)
        {
            if (workers[i].busy)
            {
                char status;
                ssize_t r = read(workers[i].pipe_fd[0], &status, 1);
                if (r == 1)
                {
                    // The support-server is now free
                    workers[i].busy = 0;
                }
            }
        }
        // ===
    }

    close(server_sock);
    return 0;
}

//=============================================================================
// To get the current time
void get_time(char *buffer, size_t size)
{
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S\n", tm_now);
}

//=============================================================================
// Support-server, the processing cycle loop 
void worker_loop(int read_fd, int write_fd)
{
    char buffer[BUFFER_SIZE];

    while (1)
    {
        // Waiting from the listen-server the signal about a new client
        // reed client_fd grom pipe
        int client_fd;
        ssize_t r = read(read_fd, &client_fd, sizeof(client_fd));
        if (r <= 0)
        {
            break;
        }

        // Client processing - send the time
        get_time(buffer, sizeof(buffer));
        send(client_fd, buffer, strlen(buffer), 0);

        close(client_fd);

        // Inform the listening server that we are readey for the next client
        char status = '1';
        write(write_fd, &status, 1);
    }
    close(read_fd);
    close(write_fd);
    exit(0);
}

//=============================================================================
// Creatind and starting a new support-server
int spawn_worker(int index)
{
    if (pipe(workers[index].pipe_fd) == -1)
    {
        perror("pipe");
        return -1;
    }

    int parent_write = workers[index].pipe_fd[1];
    int child_read = workers[index].pipe_fd[0];

    // Creating the second pipe for feedback - child to parent
    int pipe_back[2];
    if (pipe(pipe_back) == -1)
    {
        perror("pipe");
        close(workers[index].pipe_fd[0]);
        close(workers[index].pipe_fd[1]);
        return -1;
    }
    
    int parent_read = pipe_back[0];
    int child_write = pipe_back[1];

    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork");
        close(workers[index].pipe_fd[0]);
        close(workers[index].pipe_fd[1]);
        close(pipe_back[0]);
        close(pipe_back[1]);
        return -1;
    }

    if (pid == 0)
    {
        close(parent_write);
        close(parent_read);

        // Starting the loop client processing
        worker_loop(child_read, child_write);
    }

    close(child_read);
    close(child_write);

    workers[index].pid = pid;
    workers[index].pipe_fd[1] = parent_write;
    workers[index].pipe_fd[0] = parent_read;
    workers[index].busy = 0;

    worker_count++;
    return 0;
}

//=============================================================================
// Searching the free support-server
int find_free_worker()
{
    for (int i = 0; i < WORKER_MAX; i++)
    {
        if (workers[i].busy == 0)
        {
            return i;
        }
        return -1;
    }
}
