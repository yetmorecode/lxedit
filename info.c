#include "lx.h"

void info_refresh() {
    lxedit_layout *l = lx->layout;
    wclear(l->tree);
    int row = 0;
    exe *exe = lx->executables[lx->layout->active_exe];
    mvwprintw(l->tree, row++, 0, "%s", exe->name);
    mvwprintw(l->tree, row, 0, "%6x - %6x", 0, sizeof(exe->mz));
    mvwprintw(l->tree, row++, 16, "MZ header");
    mvwprintw(l->tree, row, 0, "%6x - %6x", sizeof(exe->mz), exe->mz.e_cparhdr*16);
    mvwprintw(l->tree, row++, 16, "MZ relocations");
    mvwprintw(l->tree, row, 0, "%6x - %6x", exe->mz.e_cparhdr*16, (exe->mz.e_cp-1)*512 + exe->mz.e_cblp);
    mvwprintw(l->tree, row++, 16, "MZ code");
    mvwprintw(l->tree, row, 0, "%6x - %6x", exe->mz.e_lfanew, exe->mz.e_lfanew + sizeof(exe->lx));
    mvwprintw(l->tree, row++, 16, "LX header");
    mvwprintw(l->tree, row, 0, "%6x - %6x", exe->mz.e_lfanew + sizeof(exe->lx), exe->mz.e_lfanew + sizeof(exe->lx) + exe->lx.loader_size);
    mvwprintw(l->tree, row++, 16, "LX loader");
    mvwprintw(l->tree, row, 0, "%6x - %6x", exe->mz.e_lfanew + exe->lx.objtab_off, exe->mz.e_lfanew + exe->lx.objtab_off + exe->lx.num_objects*sizeof(object_record));
    mvwprintw(l->tree, row++, 18, "object table");
    mvwprintw(l->tree, row, 0, "%6x - %6x",
    	exe->mz.e_lfanew + exe->lx.objmap_off,
    	exe->mz.e_lfanew + exe->lx.objmap_off +
    	exe->lx.num_pages * (exe->lx.signature == OSF_FLAT_SIGNATURE ? sizeof(le_map_entry) : sizeof(lx_map_entry)));
    mvwprintw(l->tree, row++, 18, "page table");
    if (exe->lx.num_rsrcs) {
	mvwprintw(l->tree, row, 0, "%6x - %6x", exe->mz.e_lfanew + exe->lx.rsrc_off, exe->mz.e_lfanew + exe->lx.rsrc_off + 16*exe->lx.num_rsrcs);
	mvwprintw(l->tree, row++, 18, "resource table");
    }
    mvwprintw(l->tree, row, 0, "%6x - %6x", exe->mz.e_lfanew + exe->lx.resname_off, exe->mz.e_lfanew + exe->lx.entry_off);
    mvwprintw(l->tree, row++, 18, "name table");
    mvwprintw(l->tree, row, 0, "%6x - %6x", exe->mz.e_lfanew + exe->lx.entry_off, exe->mz.e_lfanew + exe->lx.entry_off + 1);
    mvwprintw(l->tree, row++, 18, "entry table");
    if (exe->lx.cksum_off) {
    	mvwprintw(l->tree, row, 0, "%6x - %6x", exe->mz.e_lfanew + exe->lx.cksum_off, exe->mz.e_lfanew + exe->lx.cksum_off);
    	mvwprintw(l->tree, row++, 18, "checksum table");
    }
    mvwprintw(l->tree, row, 0, "%6x - %6x", exe->mz.e_lfanew + exe->lx.fixpage_off, exe->mz.e_lfanew + exe->lx.fixpage_off + (exe->lx.num_pages+1)*4);
    mvwprintw(l->tree, row++, 18, "fixup table");
    mvwprintw(l->tree, row, 0, "%6x - %6x", exe->mz.e_lfanew + exe->lx.fixrec_off, exe->mz.e_lfanew + exe->lx.impmod_off);
    mvwprintw(l->tree, row++, 18, "fixup records");
    if (exe->lx.impproc_off > exe->lx.impmod_off) {
	mvwprintw(l->tree, row, 0, "%6x - %6x", exe->mz.e_lfanew + exe->lx.impmod_off, exe->mz.e_lfanew + exe->lx.impproc_off);
	mvwprintw(l->tree, row++, 18, "import mods");
    }
    mvwprintw(l->tree, row, 0, "%6x - %6x", exe->mz.e_lfanew + exe->lx.impproc_off, exe->mz.e_lfanew + exe->lx.fixpage_off + exe->lx.fixup_size);
    mvwprintw(l->tree, row++, 18, "import proc");
    mvwprintw(l->tree, row, 0, "%6x - %6x", exe->lx.page_off, exe->lx.page_off + (exe->lx.num_pages-1)*exe->lx.page_size + exe->lx.l.last_page);
    mvwprintw(l->tree, row++, 16, "LX pages");
    if (exe->stat != NULL) {
	    mvwprintw(l->tree, row, 0, "         %6x", exe->stat->st_size);
	    mvwprintw(l->tree, row++, 16, "EOF");
    }

    int col2 = l->screen_width / 4 * 2 - 6;
    row = 0;
    mvwprintw(l->tree, row++, col2, "signature:   %c%c", exe->lx.signature & 0xff, (exe->lx.signature >> 8) & 0xff);
    mvwprintw(l->tree, row++, col2, "order (b/w): %02x/%02x", exe->lx.byte_order, exe->lx.word_order);
    mvwprintw(l->tree, row++, col2, "level:       %08x", exe->lx.level);
    mvwprintw(l->tree, row++, col2, "cpu/os:      %x/%x", exe->lx.cpu_type, exe->lx.os_type);
    mvwprintw(l->tree, row++, col2, "version:     %08x", exe->lx.version);
    mvwprintw(l->tree, row++, col2, "flags:       %08x", exe->lx.flags);
    mvwprintw(l->tree, row++, col2, "num_objects: %x (%d)", exe->lx.num_objects, exe->lx.num_objects);
    mvwprintw(l->tree, row++, col2, "eip:         %x:%08x", exe->lx.start_obj, exe->lx.eip);
    mvwprintw(l->tree, row++, col2, "esp:         %x:%08x", exe->lx.stack_obj, exe->lx.esp);
    mvwprintw(l->tree, row++, col2, "autodataobj: %x", exe->lx.autodata_obj);
    mvwprintw(l->tree, row++, col2, "num_pages:   %x (%d)", exe->lx.num_pages, exe->lx.num_pages);
    mvwprintw(l->tree, row++, col2, "page_size:   %x (%d)", exe->lx.page_size, exe->lx.page_size);
    mvwprintw(l->tree, row++, col2, "page_shift:  %d", exe->lx.l.page_shift);
    mvwprintw(l->tree, row++, col2, "debug_len:   %x", exe->lx.debug_len);
    mvwprintw(l->tree, row++, col2, "num_rsrcs:   %x (%d)", exe->lx.num_rsrcs, exe->lx.num_rsrcs);
    mvwprintw(l->tree, row++, col2, "num_moddirs: %x (%d)", exe->lx.num_moddirs, exe->lx.num_moddirs);
    mvwprintw(l->tree, row++, col2, "num_impmods: %x (%d)", exe->lx.num_impmods, exe->lx.num_impmods);
    mvwprintw(l->tree, row++, col2, "num_preload: %x (%d)", exe->lx.num_preload, exe->lx.num_preload);
    mvwprintw(l->tree, row++, col2, "nonres_size: %x (%d)", exe->lx.nonres_size, exe->lx.nonres_size);
    mvwprintw(l->tree, row++, col2, "inst_preloa: %x (%d)", exe->lx.num_inst_preload, exe->lx.num_inst_preload);
    mvwprintw(l->tree, row++, col2, "inst_demand: %x (%d)", exe->lx.num_inst_demand, exe->lx.num_inst_demand);
    mvwprintw(l->tree, row++, col2, "heapsize:    %x", exe->lx.heapsize);
    mvwprintw(l->tree, row++, col2, "stacksize:   %x", exe->lx.stacksize);

    int col3 = l->screen_width / 4 * 3;
    row = 0;
    mvwprintw(l->tree, row++, col3, "magic:    %c%c", exe->mz.e_magic & 0xff, (exe->mz.e_magic >> 8) & 0xff);
    mvwprintw(l->tree, row++, col3, "cplp:     %04x", exe->mz.e_cblp);
    mvwprintw(l->tree, row++, col3, "cp:       %04x", exe->mz.e_cp);
    mvwprintw(l->tree, row++, col3, "crlc:     %04x", exe->mz.e_crlc);
    mvwprintw(l->tree, row++, col3, "cparhdr:  %04x (%x)", exe->mz.e_cparhdr, exe->mz.e_cparhdr*16);
    mvwprintw(l->tree, row++, col3, "minalloc: %04x", exe->mz.e_minalloc);
    mvwprintw(l->tree, row++, col3, "maxalloc: %04x", exe->mz.e_maxalloc);
    mvwprintw(l->tree, row++, col3, "ss:sp     %04x:%04x", exe->mz.e_ss, exe->mz.e_sp);
    mvwprintw(l->tree, row++, col3, "csum:     %04x", exe->mz.e_csum);
    mvwprintw(l->tree, row++, col3, "cs:ip     %04x:%04x", exe->mz.e_cs, exe->mz.e_ip);
    mvwprintw(l->tree, row++, col3, "lfarlc:   %04x", exe->mz.e_lfarlc);
    mvwprintw(l->tree, row++, col3, "ovno:     %04x", exe->mz.e_ovno);
    mvwprintw(l->tree, row++, col3, "res:      %04x%04x", exe->mz.e_res[0], exe->mz.e_res[1]);
    mvwprintw(l->tree, row++, col3, "          %04x%04x", exe->mz.e_res[2], exe->mz.e_res[3]);
    mvwprintw(l->tree, row++, col3, "oemid:    %04x", exe->mz.e_oemid);
    mvwprintw(l->tree, row++, col3, "oeminfo:  %04x", exe->mz.e_oeminfo);
    mvwprintw(l->tree, row++, col3, "res2:     %04x%04x", exe->mz.e_res2[0], exe->mz.e_res2[1]);
    mvwprintw(l->tree, row++, col3, "          %04x%04x", exe->mz.e_res2[2], exe->mz.e_res2[3]);
    mvwprintw(l->tree, row++, col3, "          %04x%04x", exe->mz.e_res2[4], exe->mz.e_res2[5]);
    mvwprintw(l->tree, row++, col3, "          %04x%04x", exe->mz.e_res2[6], exe->mz.e_res2[7]);
    mvwprintw(l->tree, row++, col3, "          %04x%04x", exe->mz.e_res2[8], exe->mz.e_res2[9]);
    mvwprintw(l->tree, row++, col3, "lfanew:   %08x", exe->mz.e_lfanew);
    wrefresh(l->tree);
}

void *info_input(int ch) {
	switch(ch) {
	    case KEY_LEFT:
	    case 'q':
	    	wclear(lx->layout->tree);
	    	return objects_window;
	}
	return NULL;
}

lxedit_window *info_window_create() {
    static lxedit_window win;
    win.input = &info_input;
    win.refresh = &info_refresh;
    return &win;
}
