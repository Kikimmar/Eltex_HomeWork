#include "engine.h"

//=============================================================================
// Функция сравнения для сортировки: папки первыми, потом файлы, все по алфавиту
int compare_entries(const void* a, const void* b)
{
    FileEntry* fa = (FileEntry*)a;
    FileEntry* fb = (FileEntry*)b;

    // Папка идет раньше файла
    if (fa->is_dir != fb->is_dir) return fb->is_dir - fa->is_dir;

    // Сравнение имен строками по алфавиту
    return strcasecmp(fa->name, fb->name);
}

//=============================================================================
// Загрузка и сортировка файлов из дирректории
void load_directory(FileList* flist)
{
    DIR* dir = opendir(flist->path);
    if (!dir)
    {
        flist->count = 0;
        return;
    }

    // Освобождение памяти от предыдущего списка
    for (int i = 0; i < flist->count; i++)
        free(flist->files[i].name);
    flist->count = 0;

    struct dirent* entry;
    struct stat st;
    char fullpath[PATH_MAX];

    while ((entry = readdir(dir)) != NULL && flist->count < MAX_FILES) 
    {
        if (strcmp(entry->d_name, ".") == 0) continue;  // пропускаем "."

        flist->files[flist->count].name = strdup(entry->d_name);

        // Безопасное форматирование пути с проверкой длины
        int ret = snprintf(fullpath, sizeof(fullpath), "%s/%s", flist->path, entry->d_name);
        if (ret < 0 || ret >= (int)sizeof(fullpath)) 
        {
            // Ошибка или путь слишком длинный, пропускаем этот файл
            free(flist->files[flist->count].name);
            continue;
        }

        if (stat(fullpath, &st) == 0 && S_ISDIR(st.st_mode))
            flist->files[flist->count].is_dir = 1;
        else
            flist->files[flist->count].is_dir = 0;

        flist->count++;
    }
    closedir(dir);

    // Сортировка по папкам и алфавиту
    qsort(flist->files, flist->count, sizeof(FileEntry), compare_entries);
    flist->selected = 0;
    flist->start_line = 0;
}

//=============================================================================
//Освобождение памяти списка файлов
void free_files(FileList* flist) 
{
    for (int i = 0; i < flist->count; i++)
        free(flist->files[i].name);
    flist->count = 0;
}

//=============================================================================
// Обновление пути с адекватной обработкой ".." и абсолютным путем
void update_path(FileList* flist, const char* subdir)
{
    char newpath[PATH_MAX];

    if (strcmp(subdir, "..") == 0) 
    {
        if (strcmp(flist->path, "/") == 0) 
        {
            // Уже в корне — остаёмся там
            strcpy(newpath, "/");
        } 
        else 
        {
            // Обрезаем последний компонент пути
            char* last_slash = strrchr(flist->path, '/');
            if (last_slash) 
            {
                if (last_slash == flist->path)
                    *(last_slash + 1) = '\0';  // корень "/"
                else 
                {
                    size_t len = last_slash - flist->path;
                    strncpy(newpath, flist->path, len);
                    newpath[len] = '\0';
                }
            } 
            else 
            {
                strcpy(newpath, "/");
            }
        }
    } 
    else 
    {
        // Формируем новый путь с поддиректорией
        int ret = snprintf(newpath, sizeof(newpath), "%s/%s", flist->path, subdir);
        if (ret < 0 || ret >= (int)sizeof(newpath))
        {
            // Если путь слишком длинный, оставляем старый
            strcpy(newpath, flist->path);
        }
    }

    // Нормализуем путь (убираем "..", лишние слеши)
    char resolved[PATH_MAX];
    if (realpath(newpath, resolved))
    {
        strncpy(flist->path, resolved, PATH_MAX - 1);
        flist->path[PATH_MAX - 1] = '\0';
    }
    else
    {
        // Если realpath не сработал, путь не меняем
    }
}


// Отрисовка панели
void draw_filelist(FileList* flist, int height, int width, int active)
{
    werase(flist->win);
    box(flist->win, 0, 0);
    wattron(flist->win, A_BOLD);
    mvwprintw(flist->win, 0, 2, " %s ", flist->path);
    wattroff(flist->win, A_BOLD);

    for (int i = 0; i < height - 2; i++)
    {
        int idx = flist->start_line + i;
        if (idx >= flist->count) break;
        if (active && idx == flist->selected) wattron(flist->win, A_REVERSE);
        if (flist->files[idx].is_dir)
            wattron(flist->win, COLOR_PAIR(3));
        else
            wattron(flist->win, COLOR_PAIR(1));

        mvwprintw(flist->win, i + 1, 1, "%-*.*s", width - 2, width - 2, flist->files[idx].name);

        if (flist->files[idx].is_dir)
            wattroff(flist->win, COLOR_PAIR(3));
        else
            wattroff(flist->win, COLOR_PAIR(1));

        if (active && idx == flist->selected)
            wattroff(flist->win, A_REVERSE);
    }
    wrefresh(flist->win);
}

//=============================================================================
// Перемещение выделения с прокруткой
void move_selection(FileList* flist, int delta, int height)
{
    int max_visible = height - 2;
    int new_sel = flist->selected + delta;

    if (new_sel < 0) new_sel = 0;
    if (new_sel >= flist->count) new_sel = flist->count - 1;

    flist->selected = new_sel;

    if (flist->selected < flist->start_line)
        flist->start_line = flist->selected;
    else if (flist->selected >= flist->start_line + max_visible)
        flist->start_line = flist->selected - max_visible + 1;
}