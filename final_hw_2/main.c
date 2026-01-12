#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>

#define MAX_DRIVERS 100
#define SOCKET_PATH "/tmp/cli_drivers.sock"
#define BUF_SIZE 256
#define MAX_EVENTS 50
#define EPOLL_TIMEOUT 1000  // 1 сек

typedef struct {
    pid_t pid;
    int task_timer;
    time_t task_end_time;
    int sock_fd;
    int active;
    char recv_buf[BUF_SIZE];
    int recv_len;
} Driver;

Driver drivers[MAX_DRIVERS];
int driver_count = 0;
int server_sock = -1;
int epoll_fd = -1;

int find_driver(pid_t pid) 
{
    for (int i = 0; i < driver_count; i++) 
    {
        if (drivers[i].pid == pid && drivers[i].active) 
        {
            return i;
        }
    }
    return -1;
}

int find_driver_by_fd(int fd) 
{
    for (int i = 0; i < driver_count; i++) 
    {
        if (drivers[i].active && drivers[i].sock_fd == fd) 
        {
            return i;
        }
    }
    return -1;
}

void print_driver_status(int index) 
{
    Driver *d = &drivers[index];
    time_t now = time(NULL);
    if (d->task_timer > 0 && now < d->task_end_time) 
    {
        int remaining = (int)(d->task_end_time - now);
        printf("Driver PID %d: Busy %d sec\n", d->pid, remaining);
    } 
    else 
    {
        printf("Driver PID %d: Available\n", d->pid);
    }
}

void print_all_drivers() 
{
    if (driver_count == 0) 
    {
        printf("No drivers created.\n");
        return;
    }
    printf("Active drivers:\n");
    for (int i = 0; i < driver_count; i++) 
    {
        if (drivers[i].active) 
        {
            print_driver_status(i);
        }
    }
}

void send_to_driver(int drv_index, const char* cmd) 
{
    Driver *d = &drivers[drv_index];
    if (d->sock_fd >= 0) 
    {
        if (send(d->sock_fd, cmd, strlen(cmd), 0) < 0) 
        {
            perror("send_to_driver failed");
        }
    }
}

void handle_driver_response(int drv_index) 
{
    Driver *d = &drivers[drv_index];
    printf("[Driver %d response]: %.*s", d->pid, d->recv_len, d->recv_buf);
    d->recv_len = 0;  // Сбрасываем буфер
}

void add_to_epoll(int fd, uint32_t events) 
{
    struct epoll_event ev = {0};
    ev.events = events | EPOLLET;  // Edge-triggered
    ev.data.fd = fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) 
    {
        perror("epoll_ctl add failed");
    }
}

void modify_epoll(int fd, uint32_t events) 
{
    struct epoll_event ev = {0};
    ev.events = events | EPOLLET;
    ev.data.fd = fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev) < 0) 
    {
        perror("epoll_ctl mod failed");
    }
}

void handle_connections_and_drivers() 
{
    struct epoll_event events[MAX_EVENTS];
    
    while (1) 
    {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, EPOLL_TIMEOUT);
        
        for (int i = 0; i < nfds; i++) 
        {
            int fd = events[i].data.fd;
            
            // Новое подключение от драйвера
            if (fd == server_sock) 
            {
                struct sockaddr_un client_addr;
                socklen_t addr_len = sizeof(client_addr);
                int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
                
                if (client_sock < 0) 
                {
                    if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) continue;
                    perror("accept failed");
                    continue;
                }
                
                // Non-blocking сокет
                int flags = fcntl(client_sock, F_GETFL, 0);
                fcntl(client_sock, F_SETFL, flags | O_NONBLOCK);
                
                pid_t client_pid = 0;
                if (recv(client_sock, &client_pid, sizeof(pid_t), MSG_DONTWAIT) <= 0) 
                {
                    close(client_sock);
                    continue;
                }
                
                int index = find_driver(client_pid);
                if (index >= 0) 
                {
                    drivers[index].sock_fd = client_sock;
                    printf("[CLI] Driver %d connected (fd=%d)\n", client_pid, client_sock);
                    add_to_epoll(client_sock, EPOLLIN);
                } 
                else 
                {
                    printf("[CLI] Unknown driver %d rejected\n", client_pid);
                    close(client_sock);
                }
            }
            // Данные от драйвера
            else if (events[i].events & EPOLLIN) 
            {
                int index = find_driver_by_fd(fd);
                if (index >= 0) 
                {
                    Driver *d = &drivers[index];
                    int n = recv(fd, d->recv_buf + d->recv_len, 
                               BUF_SIZE - d->recv_len - 1, MSG_DONTWAIT);
                    if (n > 0) 
                    {
                        d->recv_len += n;
                        d->recv_buf[d->recv_len] = '\0';
                        handle_driver_response(index);
                    } 
                    else if (n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) 
                    {
                        printf("[CLI] Driver %d disconnected\n", d->pid);
                        close(fd);
                        d->sock_fd = -1;
                        modify_epoll(server_sock, EPOLLIN);
                    }
                }
            }
        }
    }
}

void create_driver() 
{
    if (driver_count >= MAX_DRIVERS) 
    {
        printf("Error: Maximum drivers limit reached.\n");
        return;
    }

    pid_t pid = fork();
    if (pid == 0) 
    {
        int sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock < 0) 
        {
            perror("socket failed");
            exit(1);
        }
        
        struct sockaddr_un addr = {0};
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
        
        while (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) 
        {
            if (errno == ECONNREFUSED || errno == ENOENT || errno == EAGAIN) 
            {
                usleep(100000);
                continue;
            }
            perror("driver connect failed");
            close(sock);
            exit(1);
        }
        
        pid_t my_pid = getpid();
        send(sock, &my_pid, sizeof(pid_t), 0);
        printf("[Driver %d] Started and connected to CLI\n", my_pid);
        
        char buf[BUF_SIZE];
        while (1) 
        {
            int n = recv(sock, buf, sizeof(buf) - 1, 0);
            if (n <= 0) break;
            buf[n] = '\0';
            
            if (strncmp(buf, "TASK:", 5) == 0) 
            {
                int timer;
                if (sscanf(buf + 5, "%d", &timer) == 1) 
                {
                    printf("[Driver %d] Received task for %d sec\n", my_pid, timer);
                    send(sock, "TASK_OK\n", 7, 0);
                    sleep(timer);
                    send(sock, "AVAILABLE\n", 10, 0);
                    printf("[Driver %d] Task completed\n", my_pid);
                }
            } 
            else if (strcmp(buf, "STATUS") == 0) 
            {
                send(sock, "AVAILABLE\n", 10, 0);
            } 
            else if (strcmp(buf, "EXIT") == 0) 
            {
                send(sock, "EXIT_OK\n", 8, 0);
                break;
            }
        }
        close(sock);
        exit(0);
    } 
    else if (pid > 0) 
    {
        int index = driver_count;
        drivers[index].pid = pid;
        drivers[index].task_timer = 0;
        drivers[index].task_end_time = 0;
        drivers[index].active = 1;
        drivers[index].sock_fd = -1;
        drivers[index].recv_len = 0;
        printf("Created driver with PID %d\n", pid);
        driver_count++;
    } 
    else 
    {
        perror("fork failed");
    }
}

void send_task(pid_t target_pid, int task_timer) 
{
    int index = find_driver(target_pid);
    if (index == -1) 
    {
        printf("Error: Driver with PID %d not found.\n", target_pid);
        return;
    }

    Driver *d = &drivers[index];
    time_t now = time(NULL);

    if (d->task_timer > 0 && now < d->task_end_time) 
    {
        int remaining = (int)(d->task_end_time - now);
        printf("Error: Driver PID %d is Busy %d sec\n", target_pid, remaining);
        return;
    }

    char cmd[64];
    snprintf(cmd, sizeof(cmd), "TASK:%d\n", task_timer);
    send_to_driver(index, cmd);
    
    d->task_timer = task_timer;
    d->task_end_time = now + task_timer;
    printf("Task sent to driver PID %d for %d seconds.\n", target_pid, task_timer);
}

void get_status(pid_t target_pid) 
{
    int index = find_driver(target_pid);
    if (index == -1) 
    {
        printf("Error: Driver with PID %d not found.\n", target_pid);
        return;
    }
    print_driver_status(index);
}

int main() 
{
    unlink(SOCKET_PATH);
    
    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock < 0) 
    {
        perror("socket failed");
        return 1;
    }
    
    // Non-blocking серверный сокет
    int flags = fcntl(server_sock, F_GETFL, 0);
    fcntl(server_sock, F_SETFL, flags | O_NONBLOCK);
    
    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
    if (bind(server_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) 
    {
        perror("bind failed");
        close(server_sock);
        return 1;
    }
    
    if (listen(server_sock, 5) < 0) 
    {
        perror("listen failed");
        close(server_sock);
        unlink(SOCKET_PATH);
        return 1;
    }
    
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) 
    {
        perror("epoll_create1 failed");
        close(server_sock);
        unlink(SOCKET_PATH);
        return 1;
    }
    
    // Добавляем серверный сокет в epoll
    add_to_epoll(server_sock, EPOLLIN);
    
    pid_t conn_pid = fork();
    if (conn_pid == 0) 
    {
        handle_connections_and_drivers();
        exit(0);
    }

    printf("CLI started with EPOLL. Commands:\n");
    printf("  create_driver\n");
    printf("  send_task <pid> <timer>\n");
    printf("  get_status <pid>\n");
    printf("  get_drivers\n");
    printf("  exit\n");

    char line[BUF_SIZE];
    char cmd[32];
    pid_t pid_arg;
    int timer_arg;

    while (1) 
    {
        printf("> ");
        fflush(stdout);
        if (fgets(line, sizeof(line), stdin) == NULL) 
            break;

        if (sscanf(line, "%s", cmd) != 1) 
            continue;

        if (strcmp(cmd, "create_driver") == 0) 
        {
            create_driver();
        } 
        else if (strcmp(cmd, "get_drivers") == 0) 
        {
            print_all_drivers();
        } 
        else if (sscanf(line, "%s %d %d", cmd, &pid_arg, &timer_arg) == 3 &&
                   strcmp(cmd, "send_task") == 0) 
        {
            send_task(pid_arg, timer_arg);
        } 
        else if (sscanf(line, "%s %d", cmd, &pid_arg) == 2 &&
                   strcmp(cmd, "get_status") == 0) 
        {
            get_status(pid_arg);
        } 
        else if (strcmp(cmd, "exit") == 0) 
        {
            for (int i = 0; i < driver_count; i++) 
            {
                if (drivers[i].active && drivers[i].sock_fd >= 0) 
                {
                    send_to_driver(i, "EXIT\n");
                    kill(drivers[i].pid, SIGTERM);
                }
            }
            break;
        } 
        else 
        {
            printf("Unknown command.\n");
        }
    }

    close(epoll_fd);
    if (server_sock >= 0) 
    {
        close(server_sock);
        unlink(SOCKET_PATH);
    }
    printf("CLI exited.\n");
    return 0;
}
