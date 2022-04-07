#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include "lx.h"

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

exe *lx_load_executable(char *name) {
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
	return e;
}

