#include "lx.h"
#include <string.h>
#include <stdlib.h>
#include <direct.h>

void *objects_input(int ch) {
	lxedit_layout *layout = lx->layout;
	switch (ch) {
	case KEY_RIGHT:
	case 10:
		if (layout->active_object != -1) {
			exe *exe = lx->executables[layout->active_exe];
			object_record *r = &exe->object_records[layout->active_object];
			layout->active_page = r->mapidx;
			layout->scroll_page = r->mapidx;
			return pages_window;
		} else {
		    return info_window;
		}
		break;
	case KEY_UP:
		if (layout->active_object == 0) {
			// go to parent exe
			layout->active_object = -1;
		} else if (layout->active_object == -1) {
			// goto next exe last object
			if (layout->active_exe == 0) {
				layout->active_exe = lx->num_executables-1;
			} else {
				layout->active_exe--;
			}
			layout->active_object = lx->executables[layout->active_exe]->lx.num_objects-1;
		} else {
			layout->active_object--;
		}
		break;
	case KEY_DOWN:
		if (layout->active_object == -1) {
			layout->active_object = 0;
		} else {
			if (layout->active_object >= lx->executables[layout->active_exe]->lx.num_objects-1) {
				// goto next exe
				layout->active_object = -1;
				if (layout->active_exe == lx->num_executables - 1){
					layout->active_exe = 0;
				} else {
					layout->active_exe++;
				}
			} else {
				// goto next object
				layout->active_object++;
			}
		}
		break;
	case 'm':
		{
		exe **new_executables = (exe **)malloc(sizeof(exe*));
		exe *e = lx->executables[0];
		for (int i=1; i < lx->num_executables; i++) {
		    e = lx_merge(e, lx->executables[i], layout->active_exe == i);
		}
		new_executables[0] = e;
		lx_save(e);
		lx->executables = new_executables;
		lx->num_executables = 1;
		layout->active_exe = 0;
		}
		break;
	case 's':
		lx_save(lx->executables[layout->active_exe]);
		break;
	default:
		break;
	}
	return NULL;
}

void objects_refresh() {
	int row = 0;
	lxedit_layout *layout = lx->layout;
	exe *exe, *active_exe;
	active_exe = lx->executables[layout->active_exe];
	werase(layout->tree);
	for (int j = 0; j < lx->num_executables; j++) {
		exe = lx->executables[j];
		if (j == layout->active_exe && layout->active_object == -1) {
			wattron(layout->tree, A_BOLD);
		}
		mvwprintw(layout->tree, row, 0, "%s", exe->name);
		mvwprintw(layout->tree, row++, 51, "%d pages", exe->lx.num_pages);
		wattroff(layout->tree, A_BOLD);
		for (int i = 0; i < exe->lx.num_objects; i++) {
			if (j == layout->active_exe && layout->active_object == i) {
				wattron(layout->tree, A_BOLD);
			}
			object_record *r = &exe->object_records[i];
			mvwprintw(layout->tree, row, INDENT, "Object #%d", i);
			mvwprintw(layout->tree, row, 13, "%s%s%s",
				r->flags & OBJ_READABLE ? "r" : "-",
				r->flags & OBJ_WRITEABLE ? "w" : "-",
				r->flags & OBJ_EXECUTABLE ? "x" : "-");
			mvwprintw(layout->tree, row, 51, "%3d - %3d (%d)", r->mapidx, r->mapidx + r->mapsize - 1, r->mapsize);
			mvwprintw(layout->tree, row++, 18, "%08x - %08x [%7x]", r->addr, r->addr + r->size, r->size);
			wattroff(layout->tree, A_BOLD);
		}
		row++;
	}
	wrefresh(layout->tree);
	exe = lx->executables[layout->active_exe];
	char *shortname = exe->name + strlen(exe->name)-1;
	while (*shortname && *shortname != '\\') {
		shortname--;
	}
	werase(layout->status);
	if (exe->stat) {
	mvwprintw(layout->status, 0, 0, "%s | %.1f%s | fixups: %dkb | pages: %dkb",
		*shortname == '\\' ? shortname+1 : shortname,
		exe->stat->st_size > 1024*1024 ? 1.0*exe->stat->st_size/1024/1024: 1.0*exe->stat->st_size/1024,
		exe->stat->st_size > 1024*1024 ? "mb" : "kb",
		exe->fixup_map[exe->lx.num_pages] / 1024,
		(exe->lx.page_size * (exe->lx.num_pages-1) + exe->lx.l.last_page)/1024);
	} else {
	mvwprintw(layout->status, 0, 0, "%s | fixups: %dkb | pages: %dkb",
		*shortname == '\\' ? shortname+1 : shortname,
		exe->fixup_map[exe->lx.num_pages] / 1024,
		(exe->lx.page_size * (exe->lx.num_pages-1) + exe->lx.l.last_page)/1024);

	}
	wrefresh(layout->status);
}

lxedit_window *objects_window_create() {
	static lxedit_window window;
	window.refresh = &objects_refresh;
	window.input = &objects_input;
	return &window;
}
