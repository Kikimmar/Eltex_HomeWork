#include <ncurses.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>

#define MAX_FILES 1024
#define PATH_MAX 4096

typedef struct {
    char* name;
    int is_dir;
} FileEntry;

typedef struct {
    WINDOW* win;
    char path[PATH_MAX];
    FileEntry files[MAX_FILES];
    int count;
    int selected;
    int start_line;
} FileList;


int compare_entries(const void* a, const void* b);
void load_directory(FileList* flist);
void free_files(FileList* flist);
void update_path(FileList* flist, const char* subdir);
void draw_filelist(FileList* flist, int height, int width, int active);
void move_selection(FileList* flist, int delta, int height);