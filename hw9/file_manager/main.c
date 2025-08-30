#include "engine.h"

int main() 
{
    initscr();  // Инициализация ncurses
    cbreak();   // Без буфера
    noecho();   // Не показывать вводимые символы
    keypad(stdscr, TRUE);   // Включение стрелок и спец. клавишь
    curs_set(0);    // Скрыть курсор

    if (!has_colors()) 
    {
        endwin();
        printf("Терминал не поддерживает цвета\n");
        return 1;
    }
    start_color();

    init_pair(1, COLOR_WHITE, COLOR_BLUE);   // Файлы - белый на синем
    init_pair(3, COLOR_YELLOW, COLOR_BLUE);  // Папки - желтый на синем

    int height, width;
    getmaxyx(stdscr, height, width);

    int mid = width / 2;

    FileList left, right;
    left.win = newwin(height, mid, 0, 0);
    right.win = newwin(height, width - mid, 0, mid);

    // Инициализируем пути в абсолютные для корректной навигации
    if (realpath(".", left.path) == NULL)
        snprintf(left.path, sizeof(left.path), "/");
    if (realpath("..", right.path) == NULL)
        snprintf(right.path, sizeof(right.path), "/");

    load_directory(&left);
    load_directory(&right);

    int active = 0; // 0 - левая панель, 1 - правая

    // Отрисовка
    draw_filelist(&left, height, mid, active == 0);
    draw_filelist(&right, height, width - mid, active == 1);

    int ch;
    while ((ch = getch()) != 'q') 
    {
        FileList* fl = (active == 0) ? &left : &right;
        switch (ch)
        {
            case '\t':  // переключение панелей Tab
                active = 1 - active;
                break;
            case KEY_UP:
                move_selection(fl, -1, height);
                break;
            case KEY_DOWN:
                move_selection(fl, 1, height);
                break;
            case 10:   // Enter - перейти в выбранную папку
                if (fl->count > 0 && fl->files[fl->selected].is_dir)
                {
                    update_path(fl, fl->files[fl->selected].name);
                    load_directory(fl);
                }
                break;
        }
        draw_filelist(&left, height, mid, active == 0);
        draw_filelist(&right, height, width - mid, active == 1);
    }

    free_files(&left);
    free_files(&right);
    delwin(left.win);
    delwin(right.win);
    endwin();

    return 0;
}