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

int lx_fixup_parse(char *begin, lx_fixup *f) {
	static lx_fixup tmp;
	char *start = begin;
	if (f == NULL) {
	    f = &tmp;
	}
	f->type = *begin++;
	f->flags = *begin++;
	if (f->type & OSF_SFLAG_LIST) {
		f->src_off = *begin++;
	} else {
		f->src_off = *(short *)begin;
		begin += 2;
	}
	if (f->flags & OSF_TFLAG_OBJ_MOD_16BIT) {
	    f->object = *(short *)begin;
	    begin += 2;
	} else {
	    f->object = *begin++;
	}
	if ((f->flags & OSF_TARGET_MASK) == OSF_TARGET_INTERNAL) {
		if ((f->type & OSF_SOURCE_MASK) == OSF_SOURCE_SEG) {
			// no target
		} else if (f->flags & OSF_TFLAG_OFF_32BIT) {
			f->target_off = *(int *)begin;
			begin += 4;
		} else {
			f->target_off = *(short *)begin;
			begin += 2;
		}
	} else if ((f->flags & OSF_TARGET_MASK) == OSF_TARGET_EXT_ORD) {
		if (f->flags & OSF_TFLAG_OBJ_MOD_16BIT) {
			f->mod_ord = *(short *)begin;
			begin += 2;
		}  else {
			f->mod_ord = *begin++;
		}
		if (f->flags & OSF_TFLAG_ORDINAL_8BIT) {
			f->imp_ord = *begin++;
		} else if (f->flags & OSF_TFLAG_OFF_32BIT) {
			f->imp_ord = *(int*)begin;
			begin += 4;
		} else {
		    	f->imp_ord = *(short*)begin;
			begin += 2;
		}
		if (f->flags & OSF_TFLAG_ADDITIVE_VAL) {
			f->additive = *(int*)begin;
			begin += 4;
		}
	} else if ((f->flags & OSF_TARGET_MASK) == OSF_TARGET_EXT_NAME) {
		if (f->flags & OSF_TFLAG_OBJ_MOD_16BIT) {
			f->mod_ord = *(short *)begin;
			begin += 2;
		}  else {
			f->mod_ord = *begin++;
		}
		if (f->flags & OSF_TFLAG_OFF_32BIT) {
			f->name_off = *(int*)begin;
			begin += 4;
		} else {
		    	f->name_off = *(short*)begin;
			begin += 2;

		}
		if (f->flags & OSF_TFLAG_ADDITIVE_VAL) {
			f->additive = *(int*)begin;
			begin += 4;
		}
	} else {
		if (f->flags & OSF_TFLAG_OBJ_MOD_16BIT) {
			f->mod_ord = *(short *)begin;
			begin += 2;
		}  else {
			f->mod_ord = *begin++;
		}
		if (f->flags & OSF_TFLAG_ADDITIVE_VAL) {
			f->additive = *(int*)begin;
			begin += 4;
		}
	}
	if (f->type & OSF_SFLAG_LIST) {
		begin += 2*f->src_off;
	}
	return begin - start;
}

int lx_fixup_length(char *begin) {
	return lx_fixup_parse(begin, NULL);
}

exe *open_exe(char *name) {
    	exe *e;
	e = (exe *)malloc(sizeof(exe));
	char *cwd = getcwd(NULL, 0);
	char *filename = (char*)malloc(strlen(cwd) + strlen(name) + 2);
	sprintf(filename, "%s\\%s", cwd, name);
	e->name = filename;
	FILE *file = fopen(filename, "rb");
	if (file == NULL) {
	    return NULL;
	}
	e->stat = (struct stat*)malloc(sizeof(*e->stat));
	stat(filename, e->stat);
	int r = fread(&e->mz, sizeof(e->mz), 1, file);
	if (r != 1) {
	    return NULL;
	}
	if (e->mz.e_magic != 0x5a4d) {
	    curs_set(1);
	    return NULL;
	}
	r = fseek(file, e->mz.e_lfanew, SEEK_SET);
	if (r != 0) {
	    return NULL;
	}
	r = fread(&e->lx, sizeof(e->lx), 1, file);
	if (r != 1) {
	    return NULL;
	}

	e->object_records = (object_record *)malloc(e->lx.num_objects * sizeof(object_record));
	fseek(file, e->mz.e_lfanew + e->lx.objtab_off, SEEK_SET);
	fread(e->object_records, sizeof(object_record), e->lx.num_objects, file);

	e->object_map = (map_entry *)malloc(e->lx.num_pages * sizeof(map_entry));
	fseek(file, e->mz.e_lfanew + e->lx.objmap_off, SEEK_SET);
	for (int i = 0; i < e->lx.num_pages; i++) {
		if (e->lx.signature == OSF_FLAT_SIGNATURE) {
		    // LE
		    fread(e->object_map + i, sizeof(le_map_entry), 1, file);
		} else {
		    // LX
		    fread(e->object_map + i, sizeof(lx_map_entry), 1, file);
		}
	}

	e->fixup_map = (unsigned long*)malloc(sizeof(long)*(e->lx.num_pages+1));
	fseek(file, e->mz.e_lfanew + e->lx.fixpage_off, SEEK_SET);
	fread(e->fixup_map, sizeof(long), e->lx.num_pages+1, file);

	e->fixup_count = (int*)malloc(sizeof(int) * e->lx.num_pages);
	e->fixups = (lx_fixup ***)malloc(sizeof(void*) * e->lx.num_pages);
	char *fixup_data = (char*)malloc(e->lx.fixup_size);
	fseek(file, e->mz.e_lfanew + e->lx.fixrec_off, SEEK_SET);
	fread(fixup_data, e->lx.fixup_size - sizeof(int)*(e->lx.num_pages+1), 1, file);

	for (int i = 0; i < e->lx.num_pages; i++) {
		// Read fixups for page
		char *begin = fixup_data + e->fixup_map[i];
		char *end = fixup_data + e->fixup_map[i+1];
		e->fixup_count[i] = 0;
		while (begin < end) {
		    	begin += lx_fixup_length(begin);
			e->fixup_count[i]++;
		}
		e->fixups[i] = (lx_fixup**)malloc(e->fixup_count[i] * sizeof(lx_fixup*));
		begin = fixup_data + e->fixup_map[i];
		int j = 0;
		while (begin < end) {
		    	lx_fixup *f = (lx_fixup*)malloc(sizeof(lx_fixup));
			begin += lx_fixup_parse(begin, f);
			e->fixups[i][j++] = f;
		}
	}
	return e;
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

	lx->executables = (exe**)malloc((ac-1)*sizeof(exe*));
	lx->num_executables = ac-1;
	for (int i = 1; i < ac; i++) {
		lx->executables[i-1] = open_exe(av[i]);
	}
	layout->active_exe = 0;

	layout->status = newwin(1, layout->screen_width, layout->screen_height - 1, 0);
	layout->tree = newwin(layout->screen_height - 2, layout->screen_width, 1, 0);
	wbkgd(layout->status, COLOR_PAIR(COLOR_STATUS));

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
