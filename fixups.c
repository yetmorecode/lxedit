#include "lx.h"

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
	int col = 19;
	switch(f->type & OSF_SOURCE_MASK) {
		case 0:
			mvwprintw(l->tree, row, col, "%s", "byte fixup");
			break;
		case 1:
			mvwprintw(l->tree, row, col, "%s","undefined1");
			break;
		case 2:
			mvwprintw(l->tree, row, col, "%s","segment fixup");
			break;
		case 3:
			mvwprintw(l->tree, row, col, "%s","32bit pointer");
			break;
		case 4:
			mvwprintw(l->tree, row, col, "%s","undefined4");
			break;
		case 5:
			mvwprintw(l->tree, row, col, "%s","16bit offset fixup");
			break;
		case 6:
			mvwprintw(l->tree, row, col, "%s","16:32bit pointer");
			break;
		case 7:
			mvwprintw(l->tree, row, col, "%s","32bit offset fixup");
			break;
		case 8:
			mvwprintw(l->tree, row, col, "%s","32bit relative");
			break;

	}
	col = 43;
	switch (f->flags & OSF_TARGET_MASK) {
	    case OSF_TARGET_INTERNAL:
		mvwprintw(l->tree, row, col, "%s", "internal ref");
		mvwprintw(l->tree, row, 60, "%08x (%x:%06x)",
			exe->object_records[f->object-1].addr + f->target_off,
			f->object,
			f->target_off);
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
	mvwprintw(l->tree, row, 39, "to");
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

    }
    return NULL;
}

lxedit_window *fixups_window_create() {
	static lxedit_window win;
	win.refresh = &fixups_refresh;
	win.input = &fixups_input;
	return &win;
}
