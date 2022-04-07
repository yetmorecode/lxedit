#include <curses.h>
#include <dos.h>
#include <fcntl.h>
#include <share.h>
#include <stdlib.h>
#include <direct.h>
#include <string.h>

#include "lx.h"

lxedit *lx;

lxedit_window *objects_window = objects_window_create();
lxedit_window *pages_window = pages_window_create();
lxedit_window *fixups_window = fixups_window_create();
lxedit_window *info_window = info_window_create();

void refresh_windows() {
	lx->layout->active_window->refresh();
	wrefresh(lx->layout->header);
	wrefresh(lx->layout->status);
}

void goodbye() {
	wclear(lx->layout->header);
	wclear(lx->layout->tree);
	wclear(lx->layout->status);
	mvwprintw(lx->layout->header, 0, 0, "Thanks for using " TITLE);
	WINDOW *del = newwin(lx->layout->screen_height-1, lx->layout->screen_width, 1, 0);
	refresh_windows();
	wrefresh(del);
	curs_set(1);
}

int main (int ac, char **av) {
	initscr();
	keypad(stdscr, TRUE);
	noecho();
	curs_set(0);
	start_color();
	refresh();

	init_pair(COLOR_HEADER, COLOR_WHITE, COLOR_RED);
	init_pair(COLOR_STATUS, COLOR_WHITE, COLOR_RED);
	init_pair(COLOR_TREE, COLOR_WHITE, COLOR_BLACK);

	lx = (lxedit*)malloc(sizeof(lxedit));
	lx->layout = (lxedit_layout*)malloc(sizeof(lxedit_layout));
	lxedit_layout *layout = lx->layout;
	getmaxyx(stdscr,
		layout->screen_height,
		layout->screen_width);

	layout->active_object = -1;
	layout->header = newwin(1, layout->screen_width, 0, 0);
	wbkgd(layout->header, COLOR_PAIR(COLOR_HEADER));
	mvwprintw(layout->header, 0, layout->screen_width/2 - strlen(TITLE)/2, TITLE);


	layout->status = newwin(1, layout->screen_width, layout->screen_height - 1, 0);
	layout->tree = newwin(layout->screen_height - 2, layout->screen_width, 1, 0);
	wbkgd(layout->status, COLOR_PAIR(COLOR_STATUS));

	lx->executables = (exe**)malloc((ac-1)*sizeof(exe*));
	lx->num_executables = ac-1;
	for (int i = 1; i < ac; i++) {
		lx->executables[i-1] = (exe *)lx_load(av[i]);
	}

	layout->active_exe = 0;
	static int left_count = 0;
	layout->active_window = objects_window;
	while (1) {
		refresh_windows();
		int ch = getch();
		if (ch == KEY_LEFT) {
			left_count++;
		} else {
			left_count = 0;
		}
		if (ch == 'q' || (ch == KEY_LEFT && left_count > 1)) {
			if (layout->active_window == objects_window) {
				goodbye();
				return 0;
			}
		}
		lxedit_window *next = (lxedit_window*)layout->active_window->input(ch);
		if (next) {
			layout->active_window = next;
			left_count = 0;
		}
	}
	curs_set(1);
	return 0;
}
