#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <unistd.h>
#include <pthread.h>
#include <ncurses.h>

#define CHAT_QUEUE "/chat_queue"
#define MAX_MESSAGE_SIZE 256

typedef struct
{
    int type;               // 0 - регистрация, 1 - сообщение, 2 - список клиентов
    char name[32];
    char queue_name[64];
    char text[MAX_MESSAGE_SIZE];
} chat_message_t;

char client_name[32];
char client_queue_name[64];

mqd_t client_mq;
mqd_t server_mq;

WINDOW *win_chat, *win_users, *win_input;

pthread_mutex_t ncurses_mutex = PTHREAD_MUTEX_INITIALIZER;

//=============================================================================
void *receive_thread(void *arg)
{
    chat_message_t msg;
    while(1)
    {
        ssize_t bytes = mq_receive(client_mq, (char*)&msg, sizeof(msg), NULL);
        if (bytes > 0)
        {
            pthread_mutex_lock(&ncurses_mutex);
            if(msg.type == 2)
            {
                // Очистка окна пользователей и рамка
                werase(win_users);
                box(win_users, 0, 0);
                mvwprintw(win_users, 0, 2, " User's ");

                // Выводим по строкам начиная с (1,1) чтобы не писать на рамку
                char temp_msg[MAX_MESSAGE_SIZE];
                strncpy(temp_msg, msg.text, sizeof(temp_msg)-1);
                temp_msg[sizeof(temp_msg)-1] = '\0';

                char *line = strtok(temp_msg, "\n");
                int row = 1;
                int max_y = getmaxy(win_users);
                while (line && row < max_y - 1)
                {
                    mvwprintw(win_users, row, 1, "%s", line);
                    line = strtok(NULL, "\n");
                    row++;
                }
                wrefresh(win_users);

                // После обновления других окон курсор перемещаем в input
                wmove(win_input, 1, 3);
                wrefresh(win_input);
            } 
            else 
            {
                // Вывод сообщения в окно чата, без налазов на рамку
                wprintw(win_chat, "[%s]: %s\n", msg.name, msg.text);
                wrefresh(win_chat);

                // Курсор в поле ввода
                wmove(win_input, 1, 3);
                wrefresh(win_input);
            }
            pthread_mutex_unlock(&ncurses_mutex);
        }
    }
    return NULL;
}

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
    snprintf(client_queue_name, sizeof(client_queue_name), "/%s_queue", client_name);

    struct mq_attr attr = {0};
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(chat_message_t);

    mq_unlink(client_queue_name);
    client_mq = mq_open(client_queue_name, O_CREAT | O_RDONLY, 0666, &attr);
    if(client_mq == (mqd_t)-1)
    {
        perror("mq_open client");
        exit(EXIT_FAILURE);
    }
    server_mq = mq_open(CHAT_QUEUE, O_WRONLY);
    if(server_mq == (mqd_t)-1)
    {
        perror("mq_open server");
        mq_close(client_mq);
        mq_unlink(client_queue_name);
        exit(EXIT_FAILURE);
    }

    // Отправка регистрации
    chat_message_t reg = {0};
    reg.type = 0;
    strncpy(reg.name, client_name, sizeof(reg.name)-1);
    strncpy(reg.queue_name, client_queue_name, sizeof(reg.queue_name)-1);
    mq_send(server_mq, (const char*)&reg, sizeof(reg), 0);

    // Инициализация ncurses
    initscr();
    cbreak();
    noecho();
    curs_set(1);

    int height, width;
    getmaxyx(stdscr, height, width);

    int user_width = 30;
    int input_height = 5;

    // Создаем окно для сообщений
    win_chat = newwin(height - input_height, width - user_width, 0, 0);
    scrollok(win_chat, TRUE);
    box(win_chat, 0, 0);
    mvwprintw(win_chat, 0, 2, " Chat messages ");
    wrefresh(win_chat);

    // Окно для списка пользователей
    win_users = newwin(height - input_height, user_width, 0, width - user_width);
    box(win_users, 0, 0);
    mvwprintw(win_users, 0, 2, " User's ");
    wrefresh(win_users);

    // Окно ввода
    win_input = newwin(input_height, width, height - input_height, 0);
    box(win_input, 0, 0);
    mvwprintw(win_input, 0, 2, " Input (type 'q' to quit) ");
    wmove(win_input, 1, 3);
    wrefresh(win_input);

    pthread_t thr_receive;
    pthread_create(&thr_receive, NULL, receive_thread, NULL);

    char input[MAX_MESSAGE_SIZE];

    // Основной цикл: курсор всегда только в окне ввода
    while (1)
    {
        pthread_mutex_lock(&ncurses_mutex);
        werase(win_input);
        box(win_input, 0, 0);
        mvwprintw(win_input, 0, 2, " Input (type 'q' to quit) ");
        mvwprintw(win_input, 1, 1, "> ");
        wmove(win_input, 1, 3);  // Курсор в поле ввода
        echo();
        wrefresh(win_input);
        pthread_mutex_unlock(&ncurses_mutex);

        wgetnstr(win_input, input, sizeof(input) - 1);

        if (strcmp(input, "q") == 0)
            break;

        chat_message_t msg = {0};
        msg.type = 1;
        strncpy(msg.name, client_name, sizeof(msg.name) - 1);
        strncpy(msg.text, input, sizeof(msg.text) - 1);

        mq_send(server_mq, (const char*)&msg, sizeof(msg), 0);
    }

    pthread_cancel(thr_receive);
    pthread_join(thr_receive, NULL);

    mq_close(client_mq);
    mq_unlink(client_queue_name);
    mq_close(server_mq);

    endwin();
    return 0;
}