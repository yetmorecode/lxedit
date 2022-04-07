#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include "lx.h"

void lx_read(exe *e, uint16_t object, uint32_t offset, unsigned char *buf, int length) {
	long page_idx = (e->object_records[object-1].mapidx-1) + (offset / e->lx.page_size);
	long page_off = offset % e->lx.page_size;
	memcpy(buf, e->pages[page_idx] + page_off, length);
	return;
}

int lx_fixup_write(char *begin, lx_fixup *f) {
	char *start = begin;
	*begin = f->type;
	begin++;
	*begin = f->flags;
	begin++;
	if (f->type & OSF_SFLAG_LIST) {
	    *begin = f->src_off & 0xff;
	    begin++;
	} else {
	    *(short *)begin = f->src_off;
	    begin += 2;
	}
	if (f->flags & OSF_TFLAG_OBJ_MOD_16BIT) {
	    *(short *)begin = f->object;
	    begin += 2;
	} else {
	    *begin = f->object & 0xff;
	    begin++;
	}
	if ((f->flags & OSF_TARGET_MASK) == OSF_TARGET_INTERNAL) {
		if ((f->type & OSF_SOURCE_MASK) == OSF_SOURCE_SEG) {
			// no target
		} else if (f->flags & OSF_TFLAG_OFF_32BIT) {
			*(int *)begin = f->target_off;
			begin += 4;
		} else {
		    	*(short *)begin = f->target_off & 0xffff;
			begin += 2;
		}
	}
	// TODO external fixups
	// TODO source list
	return begin - start;
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

exe *lx_load(char *name) {
	exe *e = (exe *)malloc(sizeof(exe));
	memset(e, 0, sizeof(exe));
	char *cwd = getcwd(NULL, 0);
	char *filename = (char*)malloc(strlen(cwd) + strlen(name) + 2);
	sprintf(filename, "%s\\%s", cwd, name);
	e->name = filename;
	FILE *file = fopen(filename, "rb");
	if (file == NULL) {
	    return NULL;
	}

	// Stat file
	e->stat = (struct stat*)malloc(sizeof(*e->stat));
	stat(filename, e->stat);

	// Read MZ header
	int r = fread(&e->mz, sizeof(e->mz), 1, file);
	if (r != 1) {
	    return NULL;
	}
	if (e->mz.e_magic != 0x5a4d) {
	    curs_set(1);
	    return NULL;
	}

	// Read MZ relocations and code
	int code_size = e->mz.e_lfanew - sizeof(e->mz);
	if (code_size <= 0) {
	    return NULL;
	}
	e->mz_code = (uint8_t*)malloc(code_size);
	r = fread(e->mz_code, code_size, 1, file);
	if (r != 1) {
	    return NULL;
	}

	// Read LX header
	r = fread(&e->lx, sizeof(e->lx), 1, file);
	if (r != 1) {
	    return NULL;
	}

	// Read object records
	e->object_records = (object_record *)malloc(e->lx.num_objects * sizeof(object_record));
	fseek(file, e->mz.e_lfanew + e->lx.objtab_off, SEEK_SET);
	fread(e->object_records, sizeof(object_record), e->lx.num_objects, file);

	// Read object page map
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

	// Read name table
	long pos = e->lx.resname_off;
	lx_name *head = NULL;
	while (pos < e->lx.entry_off) {
		uint8_t l = 0;
		fread(&l, 1, 1, file);
		if (l > 0) {
			lx_name *n = (lx_name *)malloc(sizeof(lx_name));
			n->length = l;
			n->name = (char *)malloc(l+1);
			fread(n->name, l, 1, file);
			n->name[l] = 0;
			fread(&n->ordinal, sizeof(n->ordinal), 1, file);
			if (head) {
			    	n->next = head;
				n->prev = head->prev;
				head->prev->next = n;
				head->prev = n;
			} else {
			    head = n;
			    head->prev = n;
			    head->next = n;
			}
			pos += sizeof(uint8_t) + l + sizeof(uint16_t);
		} else {
		    pos++;
		    break;
		}
	}
	e->names = head;
	// Read entry table

	// Read fixup page table
	e->fixup_map = (uint32_t *)malloc(sizeof(uint32_t)*(e->lx.num_pages+1));
	fseek(file, e->mz.e_lfanew + e->lx.fixpage_off, SEEK_SET);
	fread(e->fixup_map, sizeof(uint32_t), e->lx.num_pages+1, file);

	// Read fixup records
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
			memset(f, 0, sizeof(lx_fixup));
			begin += lx_fixup_parse(begin, f);
			e->fixups[i][j++] = f;
		}
	}

	// Read pages
	e->pages = (uint8_t **)malloc(sizeof(uint8_t *) * e->lx.num_pages);
	for (int i=0; i < e->lx.num_pages; i++) {
	    // TODO support LX
	    int size = e->lx.page_size;
	    if (i == e->lx.num_pages - 1) {
		size = e->lx.l.last_page;
	    }
	    e->pages[i] = (uint8_t*)malloc(size);
	    fseek(file, e->lx.page_off + i*e->lx.page_size, SEEK_SET);
	    fread(e->pages[i], size, 1, file);
	}

	// Read watcom debug
	if (e->lx.debug_off && e->lx.debug_len) {
	    fseek(file, e->lx.debug_off, SEEK_SET);
	    e->debug_header = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
	    fread(e->debug_header, sizeof(Elf32_Ehdr), 1, file);
	    // TODO validate Elf
	    fseek(file, e->lx.debug_off + e->debug_header->e_shoff, SEEK_SET);
	    e->debug_sections = (Elf32_Shdr*)malloc(e->debug_header->e_shnum * sizeof(Elf32_Shdr));
	    fread(e->debug_sections, sizeof(Elf32_Shdr), e->debug_header->e_shnum, file);
	    e->debug_data = (uint8_t**)malloc(sizeof(uint8_t*)*e->debug_header->e_shnum);
	    for (int i=0; i < e->debug_header->e_shnum; i++) {
		Elf32_Shdr *s = &e->debug_sections[i];
		e->debug_data[i] = (uint8_t*)malloc(s->sh_size);
		fseek(file, e->lx.debug_off + s->sh_offset, SEEK_SET);
		fread(e->debug_data[i], s->sh_size, 1, file);
	    }
	}
	return e;
}

void lx_save(exe *e) {
    FILE *file = fopen(e->name, "wb");
    // Write MZ header
    fwrite(&e->mz, sizeof(e->mz), 1, file);
    // Write MZ code
    fwrite(e->mz_code, e->mz.e_lfanew - sizeof(e->mz), 1, file);
    // Write LX header
    fwrite(&e->lx, sizeof(e->lx), 1, file);
    // Write object map
    fwrite(e->object_records, sizeof(object_record), e->lx.num_objects, file);
    // Write page map
    for (int i=0; i < e->lx.num_pages; i++) {
	fwrite(e->object_map + i, sizeof(le_map_entry), 1, file);
    }
    // Write name table
    // Write entry table
    lx_name *n = e->names;
    do {
	fwrite(&n->length, sizeof(uint8_t), 1, file);
	fwrite(n->name, n->length, 1, file);
	fwrite(&n->ordinal, sizeof(uint16_t), 1, file);
	n = n->next;
    } while (n != e->names);
    uint8_t end = 0;
    // End of name table
    fwrite(&end, sizeof(end), 1, file);
    // End of entry table (empty)
    fwrite(&end, sizeof(end), 1, file);

    // Write fixup map
    fwrite(e->fixup_map, sizeof(uint32_t), e->lx.num_pages + 1, file);

    // Write fixup records
    char *buf = (char *)malloc(e->lx.fixup_size);
    char *begin = buf;
    for (int i=0; i < e->lx.num_pages; i++) {
	for (int j=0; j < e->fixup_count[i]; j++) {
	    begin += lx_fixup_write(begin, e->fixups[i][j]);
	}
    }
    fwrite(buf, begin - buf, 1, file);

    // import table
    fwrite(&end, sizeof(end), 1, file);

    // Write pages
    fseek(file, e->lx.page_off, SEEK_SET);
    for (int i=0; i < e->lx.num_pages-1; i++) {
	fwrite(e->pages[i], e->lx.page_size, 1, file);
    }
    fwrite(e->pages[e->lx.num_pages-1], e->lx.l.last_page, 1, file);

    fclose(file);
}

exe *lx_merge(exe *to, exe *from, bool use_entry) {
    exe *n;
    n = (exe *)malloc(sizeof(exe));
    n->stat = NULL;
    // Set new name
    char *cwd = getcwd(NULL, 0);
    char *shortname = to->name + strlen(to->name) - 1;
    while (*shortname && *shortname != '\\') {
       	shortname--;
    }
    n->name = (char*)malloc(strlen(cwd) + strlen(shortname) + 10);
    sprintf(n->name, "%s%s", cwd, shortname);

    // Copy MZ and LE headers from first exe
    memcpy(&n->mz, &to->mz, sizeof(n->mz));
    memcpy(&n->lx, &to->lx, sizeof(n->lx));
    n->mz_code = to->mz_code;

    // Copy Objects
    n->object_records = (object_record *)malloc(sizeof(object_record) * (to->lx.num_objects + from->lx.num_objects));
    n->lx.num_objects = to->lx.num_objects + from->lx.num_objects;
    int object_shift = to->lx.num_objects;
    if (use_entry) {
	n->lx.start_obj = from->lx.start_obj + object_shift;
	//n->lx.stack_obj = from->lx.stack_obj + object_shift;
	//n->lx.autodata_obj = from->lx.autodata_obj + object_shift;
	n->lx.eip = from->lx.eip;
	//n->lx.esp = from->lx.esp;
    }
    // Copy objects from first exe
    memcpy(n->object_records, to->object_records, sizeof(object_record) * to->lx.num_objects);
    // Copy objects from second exe
    memcpy(n->object_records + to->lx.num_objects, from->object_records, sizeof(object_record) *from->lx.num_objects);
    uint32_t vbase_shift = to->object_records[to->lx.num_objects-1].addr + to->object_records[to->lx.num_objects-1].size;
    vbase_shift = (vbase_shift / from->object_records[0].addr) * from->object_records[0].addr;
    // Shift pages in second exe objects
    int page_shift = to->lx.num_pages;
    for (int i=0; i < from->lx.num_objects; i++) {
	n->object_records[to->lx.num_objects + i].addr += vbase_shift;
	n->object_records[to->lx.num_objects + i].mapidx += page_shift;
    }

    // Pages
    n->lx.num_pages = to->lx.num_pages + from->lx.num_pages;
    n->lx.l.last_page = from->lx.l.last_page;
    // Shift LX by added object records
    int lx_shift = from->lx.num_objects * sizeof(object_record);

    // Copy page table (object map)
    n->lx.objmap_off += lx_shift;
    n->object_map = (map_entry *)malloc(n->lx.num_pages * sizeof(map_entry));
    for (int i=0; i < to->lx.num_pages; i++) {
	memcpy(n->object_map + i, to->object_map + i, sizeof(map_entry));
    }
    for (int i=0; i < from->lx.num_pages; i++) {
	memcpy(n->object_map + i + page_shift, from->object_map + i, sizeof(map_entry));
	map_entry *e = n->object_map + i + page_shift;
	le_map_entry *le = (le_map_entry *)e;
	int page_num = le->page_num[2] + (le->page_num[1] << 8) + (le->page_num[0] << 16);
	page_num += page_shift;
	le->page_num[2] = page_num & 0xff;
	le->page_num[1] = (page_num & 0xff00) >> 8;
	le->page_num[0] = (page_num & 0xff0000) >> 16;
    }
    // Shift LX by added page records
    lx_shift += from->lx.num_pages * sizeof(le_map_entry);

    n->lx.resname_off += lx_shift;
    n->names = to->names;
    // TODO: merge entries
    n->lx.entry_off += lx_shift;
    n->lx.fixpage_off += lx_shift;

    // Copy fixup page table
    n->fixup_map = (uint32_t *)malloc(sizeof(uint32_t) * (n->lx.num_pages+1));
    memcpy(n->fixup_map, to->fixup_map, sizeof(uint32_t) * (to->lx.num_pages + 1));
    memcpy(n->fixup_map + page_shift + 1, from->fixup_map + 1, sizeof(uint32_t) * from->lx.num_pages);
    for (int i=1; i < from->lx.num_pages + 1; i++) {
	uint32_t old_off = from->fixup_map[i] - from->fixup_map[0];
	n->fixup_map[page_shift + i] = old_off + n->fixup_map[page_shift];
    }
    n->fixup_count = (int *)malloc(sizeof(int) * n->lx.num_pages);
    memcpy(n->fixup_count, to->fixup_count, to->lx.num_pages * sizeof(int));
    memcpy(n->fixup_count + page_shift, from->fixup_count, from->lx.num_pages * sizeof(int));
    // Shift LX by added fixup table
    lx_shift += from->lx.num_pages * sizeof(uint32_t);
    n->lx.fixup_size += from->lx.num_pages * sizeof(uint32_t);
    n->lx.fixrec_off += lx_shift;

    // Copy (parsed) fixup records
    n->fixups = (lx_fixup ***)malloc(sizeof(lx_fixup **) * n->lx.num_pages);
    for (int i=0; i < to->lx.num_pages; i++) {
	n->fixups[i] = (lx_fixup **)malloc(sizeof(lx_fixup *) * to->fixup_count[i]);
	memcpy(n->fixups[i], to->fixups[i], sizeof(lx_fixup *) * to->fixup_count[i]);
    }
    for (int i=0; i < from->lx.num_pages; i++) {
	n->fixups[page_shift + i] = (lx_fixup **)malloc(sizeof(lx_fixup *) * from->fixup_count[i]);
	memcpy(n->fixups[page_shift + i], from->fixups[i], sizeof(lx_fixup *) * from->fixup_count[i]);
	for (int j=0; j < from->fixup_count[i]; j++) {
	    lx_fixup *f= n->fixups[page_shift + i][j];
	    // Shift fixup object numbers
	    if (f->object > 0) {
		f->object += object_shift;
	    }
	    if (f->mod_ord > 0) {
		f->mod_ord += object_shift;
	    }
	    if (f->imp_ord > 0) {
		f->imp_ord += object_shift;
	    }
	}
    }

    // Shift LX by added fixup records
    lx_shift += from->lx.impmod_off - from->lx.fixrec_off;
    n->lx.fixup_size += from->lx.impmod_off - from->lx.fixrec_off;
    n->lx.impmod_off += lx_shift;
    n->lx.impproc_off += lx_shift;
    n->lx.page_off += lx_shift;

    // Copy pages
    n->pages = (uint8_t **)malloc(sizeof(uint8_t*) * n->lx.num_pages);
    memcpy(n->pages, to->pages, sizeof(uint8_t*) * (to->lx.num_pages-1));
    uint8_t *page = (uint8_t*)malloc(n->lx.page_size);
    memset(page, 0, n->lx.page_size);
    memcpy(page, to->pages[to->lx.num_pages-1], to->lx.l.last_page);
    n->pages[to->lx.num_pages-1] = page;
    memcpy(n->pages + page_shift, from->pages, sizeof(uint8_t*) * from->lx.num_pages);

    return n;
}

