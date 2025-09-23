#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <ncurses.h>

#define SHM_NAME "/chat_shm"
#define MAX_CLIENTS 10
#define MAX_MESSAGES 50
#define MAX_NAME_LEN 32
#define MAX_MSG_LEN 256

typedef struct 
{
    char name[MAX_NAME_LEN];
    char text[MAX_MSG_LEN];
} chat_message_t;

typedef struct 
{
    char name[MAX_NAME_LEN];
    int active;
} client_t;

typedef struct 
{
    sem_t sem;
    client_t clients[MAX_CLIENTS];
    int client_count;
    chat_message_t messages[MAX_MESSAGES];
    int msg_start;
    int msg_count;
} chat_room_t;

chat_room_t *chat_room = NULL;

char client_name[MAX_NAME_LEN];
int client_idx = -1;

WINDOW *win_chat, *win_users, *win_input;
pthread_mutex_t ncurses_mutex = PTHREAD_MUTEX_INITIALIZER;
//=============================================================================
// Отображение сообщений из разделяемой памяти
void print_messages() 
{
    werase(win_chat);
    box(win_chat, 0, 0);
    mvwprintw(win_chat, 0, 2, " Chat messages ");
    int start = chat_room->msg_start;
    for (int i = 0; i < chat_room->msg_count; i++) 
    {
        int idx = (start + i) % MAX_MESSAGES;
        mvwprintw(win_chat, i + 1, 1, "[%s]: %s", chat_room->messages[idx].name, chat_room->messages[idx].text);
        if (i + 2 >= getmaxy(win_chat)) break;
    }
    wrefresh(win_chat);
}
//=============================================================================
// Отображение списка клиентов из разделяемой памяти
void print_clients() 
{
    werase(win_users);
    box(win_users, 0, 0);
    mvwprintw(win_users, 0, 2, " Users ");
    int row = 1;
    for (int i = 0; i < MAX_CLIENTS && row < getmaxy(win_users) - 1; i++) 
    {
        if (chat_room->clients[i].active) 
        {
            mvwprintw(win_users, row++, 1, "%s", chat_room->clients[i].name);
        }
    }
    wrefresh(win_users);
}
//=============================================================================
// Поток для чтения обновлений сообщений и списка клиентов
void* receive_thread(void* arg) 
{
    int last_msg_count = 0;
    int last_client_count = 0;

    while (1) 
    {
        sem_wait(&chat_room->sem);

        if (chat_room->msg_count != last_msg_count) 
        {
            print_messages();
            last_msg_count = chat_room->msg_count;
        }

        if (chat_room->client_count != last_client_count) 
        {
            print_clients();
            last_client_count = chat_room->client_count;
        }

        sem_post(&chat_room->sem);

        pthread_mutex_lock(&ncurses_mutex);
        pthread_mutex_unlock(&ncurses_mutex);

        usleep(200000); // 200 мс
    }
    return NULL;
}
//=============================================================================
int main() 
{
    printf("Enter your name: ");
    if (!fgets(client_name, sizeof(client_name), stdin)) 
    {
        fprintf(stderr, "Failed to read name\n");
        exit(EXIT_FAILURE);
    }
    client_name[strcspn(client_name, "\n")] = 0;
    if (strlen(client_name) == 0) 
    {
        fprintf(stderr, "Empty name not allowed\n");
        exit(EXIT_FAILURE);
    }

    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) 
    {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    chat_room = mmap(NULL, sizeof(chat_room_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (chat_room == MAP_FAILED) 
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Регистрируем клиента
    sem_wait(&chat_room->sem);
    client_idx = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) 
    {
        if (!chat_room->clients[i].active) 
        {
            strncpy(chat_room->clients[i].name, client_name, MAX_NAME_LEN - 1);
            chat_room->clients[i].name[MAX_NAME_LEN - 1] = '\0';
            chat_room->clients[i].active = 1;
            chat_room->client_count++;
            client_idx = i;
            break;
        }
    }
    sem_post(&chat_room->sem);

    if (client_idx == -1) 
    {
        fprintf(stderr, "Chat room is full. Cannot join.\n");
        exit(EXIT_FAILURE);
    }

    // Инициализация ncurses
    initscr();
    cbreak();
    noecho();
    curs_set(1);

    int height, width;
    getmaxyx(stdscr, height, width);

    int user_width = 30;
    int input_height = 5;

    // Создаем окна
    win_chat = newwin(height - input_height, width - user_width, 0, 0);
    scrollok(win_chat, TRUE);
    box(win_chat, 0, 0);
    mvwprintw(win_chat, 0, 2, " Chat messages ");
    wrefresh(win_chat);

    win_users = newwin(height - input_height, user_width, 0, width - user_width);
    box(win_users, 0, 0);
    mvwprintw(win_users, 0, 2, " Users ");
    wrefresh(win_users);

    win_input = newwin(input_height, width, height - input_height, 0);
    box(win_input, 0, 0);
    mvwprintw(win_input, 0, 2, " Input (type 'q' to quit) ");
    mvwprintw(win_input, 1, 1, "> ");
    wrefresh(win_input);

    pthread_t thr_receive;
    pthread_create(&thr_receive, NULL, receive_thread, NULL);

    char input[MAX_MSG_LEN];

    while (1) 
    {
        pthread_mutex_lock(&ncurses_mutex);
        werase(win_input);
        box(win_input, 0, 0);
        mvwprintw(win_input, 0, 2, " Input (type 'q' to quit) ");
        mvwprintw(win_input, 1, 1, "> ");
        wmove(win_input, 1, 3);
        echo();
        wrefresh(win_input);
        pthread_mutex_unlock(&ncurses_mutex);

        wgetnstr(win_input, input, sizeof(input) - 1);

        if (strcmp(input, "q") == 0)
            break;

        sem_wait(&chat_room->sem);
        int pos = (chat_room->msg_start + chat_room->msg_count) % MAX_MESSAGES;
        strncpy(chat_room->messages[pos].name, client_name, MAX_NAME_LEN - 1);
        chat_room->messages[pos].name[MAX_NAME_LEN - 1] = '\0';
        strncpy(chat_room->messages[pos].text, input, MAX_MSG_LEN - 1);
        chat_room->messages[pos].text[MAX_MSG_LEN - 1] = '\0';
        if (chat_room->msg_count < MAX_MESSAGES)
            chat_room->msg_count++;
        else
            chat_room->msg_start = (chat_room->msg_start + 1) % MAX_MESSAGES;
        sem_post(&chat_room->sem);
    }

    // Удаление клиента из списка перед выходом
    sem_wait(&chat_room->sem);
    chat_room->clients[client_idx].active = 0;
    chat_room->client_count--;
    sem_post(&chat_room->sem);

    pthread_cancel(thr_receive);
    pthread_join(thr_receive, NULL);

    munmap(chat_room, sizeof(chat_room_t));
    endwin();

    return 0;
}
