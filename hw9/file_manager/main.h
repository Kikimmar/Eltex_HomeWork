#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <stdlib.h>
#include <curses.h>
#include <dirent.h>
#include <string.h>

#define MAX_FILES 1024

typedef struct 
{
    char* name;
    int is_dir;
} FileEntry;

typedef struct
{
    WINDOW* win;
    char path[PATH_MAX];
    FileEntry files [MAX_FILES];
    int count;
    int selected;
    int start_line;
} FileList;

void load_directory(FileList* first);
void free_files(FileList* flist);
void draw_filelist(FileList* flist, int height, int width, int active);
void move_selection(FileList* flist, int delta, int height);
