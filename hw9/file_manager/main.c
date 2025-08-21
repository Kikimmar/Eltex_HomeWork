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

	WINDOW* wnd;
	WINDOW* subwnd;

	initscr();
	signal(SIGWINCH, sig_winch);
	cbreak();
	curs_set(FALSE);
	start_color();
	refresh();

	int height, width;
	getmaxyx(stdscr, height, width);

	init_pair(1, COLOR_BLUE, COLOR_GREEN);
	init_pair(2, COLOR_YELLOW, COLOR_BLUE);

	wnd = newwin(height, width, 0, 0);
	wattron(wnd, COLOR_PAIR(1));
	box(wnd, '|', '-');
	//mvwprintw(wnd, 0, 0, "Main window");

	subwnd = derwin(wnd, 16, 16, 1, 1);
	wattron(subwnd, COLOR_PAIR(2));
	wattron(subwnd, A_BOLD);
	mvwprintw(subwnd, 0, 0, "Subwindow");
	
	wrefresh(subwnd);
	wrefresh(wnd);

	getch();

	delwin(subwnd);
	delwin(wnd);
	refresh();

	endwin();
	return 0;
}
