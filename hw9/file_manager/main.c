#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <stdlib.h>
#include <curses.h>

void sig_winch(int signo)
{
    struct winsize size;
    ioctl(fileno(stdout), TIOCGWINSZ, (char*) &size);
    resizeterm(size.ws_row, size.ws_col);
}

int main()
{
   initscr();
   signal(SIGWINCH, sig_winch);
   cbreak();
   noecho();

   int height = LINES;
   int width = COLS;

   WINDOW* win = newwin(height, width, 2, 2);

   box(win, 0, 0);
   wrefresh(win);

   getch();
   delwin(win);    

   endwin();
   return 0;
}
