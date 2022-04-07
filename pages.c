#include "lx.h"

void pages_refresh() {
	lxedit_layout *layout = lx->layout;
	wclear(layout->tree);
	int row = 1;
	exe *exe = lx->executables[layout->active_exe];
	object_record *r = &exe->object_records[layout->active_object];
	for (int i = 0; i < r->mapsize; i++) {
		int page = r->mapidx + i;
		if (page < layout->scroll_page) {
			continue;
		}
		if (page == layout->active_page) {
			wattron(layout->tree, A_BOLD);
		}
		if (exe->lx.signature == OSF_FLAT_SIGNATURE) {
			// LE
			le_map_entry *lemap = (le_map_entry*)(&exe->object_map[page-1]);
			int page_num = lemap->page_num[2] + (lemap->page_num[1] << 8) + (lemap->page_num[0] << 16);
			mvwprintw(layout->tree, row, 10, "~%d", page_num);
			mvwprintw(layout->tree, row, 28, "%06x - %06x",
				exe->lx.page_off + (page_num-1) * exe->lx.page_size,
				exe->lx.page_off + page_num * exe->lx.page_size);
			switch (lemap->flags) {
				case PAGE_VALID:
					mvwprintw(layout->tree, row, 16, "valid");
					break;
				case PAGE_ITERATED:
					mvwprintw(layout->tree, row, 16, "iter");
					break;
				case PAGE_INVALID:
					mvwprintw(layout->tree, row, 16, "inval");
					break;
				case PAGE_ZEROED:
					mvwprintw(layout->tree, row, 16, "zero");
					break;
				case PAGE_RANGE:
					mvwprintw(layout->tree, row, 16, "range");
					break;
				default:
					mvwprintw(layout->tree, row, 16, "%x", lemap->flags);
					break;
			}
			mvwprintw(layout->tree, row, 44, "%08x - %08x",
				r->addr + i * exe->lx.page_size,
				r->addr + (i+1) * exe->lx.page_size,
				exe->lx.page_size);
		} else {
			// LX
			lx_map_entry *lxmap = (lx_map_entry*)(&exe->object_map[page-1]);
			int offset = lxmap->page_offset;
			int size = lxmap->data_size;
			int flags = lxmap->flags;
			mvwprintw(layout->tree, row, 16, "%x", size);
			mvwprintw(layout->tree, row, 22, "%04x", flags);
			mvwprintw(layout->tree, row, 29, "%06x - %06x",
				exe->lx.page_off + offset,
				exe->lx.page_off + offset + size);
			mvwprintw(layout->tree, row, 46, "%08x - %08x",
				r->addr + offset,
				r->addr + offset + size);
		}
		mvwprintw(layout->tree, row, 73, "%d", exe->fixup_count[page-1]);
		mvwprintw(layout->tree, row++, 0, "Page #%d", page);
		wattroff(layout->tree, A_BOLD);
	}
	wrefresh(layout->tree);
	WINDOW *tree_head = newwin(1, layout->screen_width, 1, 0);
	wbkgd(tree_head, COLOR_PAIR(COLOR_STATUS));
	mvwprintw(tree_head, 0, 0, "Logical#");
	mvwprintw(tree_head, 0, 10, "Phy#");
	mvwprintw(tree_head, 0, 16, "Size");
	mvwprintw(tree_head, 0, 22, "Flags");
	mvwprintw(tree_head, 0, 29, "File Offset");
	mvwprintw(tree_head, 0, 46, "Memory");
	mvwprintw(tree_head, 0, 73, "Fixups");
	wrefresh(tree_head);
	delwin(tree_head);
	wclear(layout->status);
	mvwprintw(layout->status, 0, 0,
		"%s > Object #%d (%d pages)",
		exe->name, layout->active_object+1, r->mapsize);
	wrefresh(layout->status);

	return;
}

void *pages_input(int ch) {
	lxedit_layout *layout = lx->layout;
	exe *exe = lx->executables[layout->active_exe];
	object_record *r = &exe->object_records[layout->active_object];
	switch (ch) {
		case KEY_LEFT:
		case 'q':
			wclear(layout->tree);
			return objects_window;
		case KEY_RIGHT:
			wclear(layout->tree);
			wclear(layout->status);
			layout->active_fixup = 0;
			layout->scroll_fixup = 0;
			return fixups_window;
		case KEY_DOWN:
			if (layout->active_page == r->mapidx + r->mapsize - 1) {
				layout->active_page = r->mapidx;
				layout->scroll_page = r->mapidx;
			} else {
				layout->active_page++;
			}
			if (layout->active_page - layout->scroll_page >= layout->screen_height - 3) {
				layout->scroll_page++;
			}
			break;
		case KEY_UP:
			if (layout->active_page == r->mapidx) {
				layout->active_page = r->mapidx + r->mapsize - 1;
				layout->scroll_page = layout->active_page - layout->screen_height + 4;
			} else {
				layout->active_page--;
			}
			if (layout->active_page - layout->scroll_page < 0) {
				layout->scroll_page--;
			}
			break;
	}
	return NULL;
}

lxedit_window *pages_window_create() {
	static lxedit_window window;
	window.input = &pages_input;
	window.refresh = &pages_refresh;
	return &window;
}
