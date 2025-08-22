#include "main.h"

int main()
{
	initscr();
	cbreak();
    noecho();
	keypad(stdscr, TRUE);
	curs_set(0);
    start_color();

	int height, width;
	getmaxyx(stdscr, height, width);

	init_pair(1, COLOR_WHITE, COLOR_BLACK);
	init_pair(2, COLOR_CYAN, COLOR_BLACK);

	FileList left, right;
    left.win = newwin(height, width / 2, 0, 0);
    right.win = newwin(height, width - width / 2, 0, width / 2);
    
    // Можно менять стартовые дирректории
    strcpy(left.path, ".");
    strcpy(right.path, "..");

    load_directory(&left);
    load_directory(&right);
    
    int active = 0;  // 0 - левое окно, 1 - правое

    draw_filelist(&left, height, width / 2, active == 0);
    draw_filelist(&right, height, width - width / 2, active == 1);

    int ch;

    while ((ch = getch()) != 'q') 
    {
        switch(ch)
        {
            case '\t': // TAB - переключение между окнами
                active = 1 - active;
                break;
            case KEY_UP:
                if (active == 0) move_selection(&left, -1, height);
                else move_selection(&right, -1, height);
                break;
            case KEY_DOWN:
                if (active == 0) move_selection(&left, 1, height);
                else move_selection(&right, 1, height);
                break;
            case 10:
                {
                    FileList* fl = active == 0 ? &left : &right;
                    if (fl->files[fl->selected].is_dir)
                    {
                        // освободить старые имена
                        free_files(fl);
                        // обновить путь
                        if (strcmp(fl->path, "/") != 0)
                            strcat(fl->path, "/");
                        strcat(fl->path, fl->files[fl->selected].name);
                        load_directory(fl);
                    }
                }
                break;
        }
        draw_filelist(&left, height, width / 2, active == 0);
        draw_filelist(&right, height, width - width / 2, active == 1);
    }
    free_files(&left);
    free_files(&right);

    delwin(left.win);
    delwin(right.win);

	endwin();
	return 0;    
}

//=============================================================================
void load_directory(FileList* flist)
{
    DIR* dir = opendir(flist->path);
    if(!dir)
    {
        flist->count = 0;
        return;
    }
    struct dirent* entry;
    flist->count = 0;
    while((entry = readdir(dir)) != NULL && flist->count < MAX_FILES)
    {
        if(strcmp(entry->d_name, ".") == 0) continue;
        flist->files[flist->count].name = strdup(entry->d_name);
        flist->files[flist->count].is_dir = (entry->d_type == DT_DIR);
        flist->count++;
    }
    closedir(dir);
    flist->selected = 0;
    flist->start_line = 0;
}

//=============================================================================

void free_files(FileList* flist)
{
    for (int i = 0; i < flist->count; i++)
        free(flist->files[i].name);
    flist->count = 0;
}

//=============================================================================

void draw_filelist(FileList* flist, int height, int width, int active)
{
    werase(flist->win);
    box(flist->win, 0, 0);
    mvwprintw(flist->win, 0, 2, " %s ", flist->path);
    for (int i = 0; i < height - 2; i++)
    {
        int idx= flist->start_line + i;
        if (idx >= flist->count) break;
        if (idx == flist->selected && active)
            wattron(flist->win, A_REVERSE);
        if (flist->files[idx].is_dir)
            wattron(flist->win, COLOR_PAIR(2));
        else
            wattron(flist->win, COLOR_PAIR(1));

        mvwprintw(flist->win, i + 1, 1, "%-*s", width - 2, flist->files[idx].name);

        if (flist->files[idx].is_dir)
            wattroff(flist->win, COLOR_PAIR(2));
        else
            wattroff(flist->win, COLOR_PAIR(1));

        if (idx == flist->selected && active)
        {
            wattroff(flist->win, A_REVERSE);
        }
    }
    wrefresh(flist->win);
}

//=============================================================================

void move_selection(FileList* flist, int delta, int height)
{
    int newsel = flist->selected + delta;
    if (newsel < 0) newsel = 0;
    if (newsel >= flist->count) newsel = flist->count - 1;
    
    flist->selected = newsel;

    if (flist->selected < flist->start_line)
        flist->start_line = flist->selected;
    else if (flist->selected >= flist->start_line + height - 2)
    {
        flist->start_line = flist->selected - (height - 3);
        if (flist->start_line < 0) flist->start_line = 0;
    }
}

