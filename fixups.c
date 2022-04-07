#include "lx.h"
#include <ctype.h>

void fixups_refresh() {
    lxedit_layout *l = lx->layout;
    int row = 0;
    exe *exe = lx->executables[l->active_exe];
    object_record *r = exe->object_records + l->active_object;

    for (int i = 0; i < exe->fixup_count[l->active_page-1]; i++) {
	if (i < l->scroll_fixup) {
	    continue;
	}
	if (l->active_fixup == i) {
	    wattron(l->tree, A_BOLD);
	}
	lx_fixup *f = exe->fixups[l->active_page-1][i];
	int col = 40;
	switch(f->type & OSF_SOURCE_MASK) {
		case 0:
			mvwprintw(l->tree, row, col, "%s", "byte");
			break;
		case 1:
			mvwprintw(l->tree, row, col, "%s","u1");
			break;
		case 2:
			mvwprintw(l->tree, row, col, "%s","seg");
			break;
		case 3:
			mvwprintw(l->tree, row, col, "%s","32bit ptr");
			break;
		case 4:
			mvwprintw(l->tree, row, col, "%s","u4");
			break;
		case 5:
			mvwprintw(l->tree, row, col, "%s","16bit off");
			break;
		case 6:
			mvwprintw(l->tree, row, col, "%s","16:32bit ptr");
			break;
		case 7:
			mvwprintw(l->tree, row, col, "%s","32bit off");
			break;
		case 8:
			mvwprintw(l->tree, row, col, "%s","32bit rel");
			break;

	}
	col = 16;
	switch (f->flags & OSF_TARGET_MASK) {
	    case OSF_TARGET_INTERNAL:
	        {
		mvwprintw(l->tree, row, col, "-> %08x (%x:%06x)",
			exe->object_records[f->object-1].addr + f->target_off,
			f->object,
			f->target_off);


		unsigned char buf[6] = "\x00\x00\x00\x00\x00";
		lx_read(exe, f->object, f->target_off, buf, sizeof(buf)-1);
		for (int i=0; i < sizeof(buf)-1; i++) {
		    mvwprintw(l->tree, row, 53 + i*3, "%02x ", buf[i]);
		    mvwprintw(l->tree, row, 53 + sizeof(buf)*3 - 2 + i, "%c ", isalpha(buf[i]) ? buf[i] : '.');
		}
		}
	    	break;
	    case OSF_TARGET_EXT_ORD:
		mvwprintw(l->tree, row, col, "%s", "import by ord");
	    	break;
	    case OSF_TARGET_EXT_NAME:
		mvwprintw(l->tree, row, col, "%s", "import by name");
	    	break;
	    case OSF_TARGET_INT_VIA_ENTRY:
		mvwprintw(l->tree, row, col, "%s", "ref via entry");
	    	break;

	}
	mvwprintw(l->tree, row, 6, "@%08x", r->addr + (l->active_page - r->mapidx)*exe->lx.page_size + f->src_off);
	mvwprintw(l->tree, row++, 0, "%04d", i+1);
	wattroff(l->tree, A_BOLD);
    }
    wrefresh(l->tree);

    wclear(l->status);
    mvwprintw(l->status, 0, 0, "%s > Object #%d > Page #%d (%x - %x)",
    	lx->executables[l->active_exe]->name,
	l->active_object+1,
	l->active_page,
	r->addr + (l->active_page - r->mapidx)*exe->lx.page_size,
	r->addr + (l->active_page - r->mapidx+1)*exe->lx.page_size);
    wrefresh(l->status);
}

void *fixups_input(int ch) {
    exe *exe = lx->executables[lx->layout->active_exe];
    lxedit_layout *l = lx->layout;
    switch(ch) {
	case KEY_LEFT:
	case 'q':
		return pages_window;
	case KEY_DOWN:
		if (l->active_fixup == exe->fixup_count[l->active_page-1]-1) {
		    l->active_fixup = 0;
		    l->scroll_fixup = 0;
		} else {
		    l->active_fixup++;
		}
		if (l->active_fixup - l->scroll_fixup > l->screen_height - 3) {
		    l->scroll_fixup++;
		}
		break;
	case KEY_UP:
		if (l->active_fixup == 0) {
		    l->active_fixup = exe->fixup_count[l->active_page-1]-1;
		    l->scroll_fixup = l->active_fixup - (l->screen_height - 3);
		} else {
		    l->active_fixup--;
		}
		if (l->active_fixup < l->scroll_fixup) {
		    l->scroll_fixup--;
		}
		break;
	case 'e':
		{
		char buf[20];
		int r = l->active_fixup - l->scroll_fixup + 1;
		WINDOW *input = newwin(1, sizeof(buf)-1, r, 19);
		wbkgd(input, COLOR_PAIR(COLOR_HEADER));
		echo();
		wrefresh(input);
		wgetnstr(input, buf, sizeof(buf)-1);
		uint16_t obj;
		uint32_t target;
		sscanf(buf, "%d:%x", &obj, &target);
		lx_fixup *f = exe->fixups[l->active_page-1][l->active_fixup];
		f->object = obj;
		f->target_off = target;
		noecho();
		delwin(input);
		wclear(l->tree);
		}
		break;

    }
    return NULL;
}

lxedit_window *fixups_window_create() {
	static lxedit_window win;
	win.refresh = &fixups_refresh;
	win.input = &fixups_input;
	return &win;
}
