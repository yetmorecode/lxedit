
#ifndef __LX_H
#define __LX_H

#include <curses.h>
#include <sys/types.h>
#include <sys/stat.h>

#define TITLE "lxedit - https://github.com/yetmorecode/lxedit"
#define COLOR_HEADER 1
#define COLOR_STATUS 2
#define COLOR_TREE 3
#define COLOR_TREE_HEADER 4
#define INDENT 1

typedef unsigned short USHORT;
typedef long LONG;

typedef struct _IMAGE_DOS_HEADER {  // DOS .EXE header
    USHORT e_magic;         // Magic number
    USHORT e_cblp;          // Bytes on last page of file
    USHORT e_cp;            // Pages in file
    USHORT e_crlc;          // Relocations
    USHORT e_cparhdr;       // Size of header in paragraphs
    USHORT e_minalloc;      // Minimum extra paragraphs needed
    USHORT e_maxalloc;      // Maximum extra paragraphs needed
    USHORT e_ss;            // Initial (relative) SS value
    USHORT e_sp;            // Initial SP value
    USHORT e_csum;          // Checksum
    USHORT e_ip;            // Initial IP value
    USHORT e_cs;            // Initial (relative) CS value
    USHORT e_lfarlc;        // File address of relocation table
    USHORT e_ovno;          // Overlay number
    USHORT e_res[4];
    USHORT e_oemid;         // OEM identifier (for e_oeminfo)
    USHORT e_oeminfo;       // OEM information; e_oemid specific
    USHORT e_res2[10];
    LONG  e_lfanew;        // File address of new exe header
} IMAGE_DOS_HEADER;


#define OSF_FLAT_RESERVED 20

typedef unsigned char unsigned_8;
typedef unsigned short unsigned_16;
typedef unsigned int unsigned_32;

typedef struct os2_flat_header {
    unsigned_16     signature;
    unsigned_8      byte_order;     /* the byte ordering of the .exe */
    unsigned_8      word_order;     /* the word ordering of the .exe */
    unsigned_32     level;          /* the exe format level */
    unsigned_16     cpu_type;       /* the cpu type */
    unsigned_16     os_type;        /* the operating system type */
    unsigned_32     version;        /* .exe version */
    unsigned_32     flags;          /* .exe flags */
    unsigned_32     num_pages;      /* # of pages in .exe */
    unsigned_32     start_obj;      /* starting object number */
    unsigned_32     eip;            /* starting value of eip */
    unsigned_32     stack_obj;      /* object # for stack pointer */
    unsigned_32     esp;            /* starting value of esp */
    unsigned_32     page_size;      /* .exe page size */
    union {
        unsigned_32     last_page;      /* size of last page - LE */
        unsigned_32     page_shift;     /* left shift for page offsets - LX */
    }               l;
    unsigned_32     fixup_size;     /* fixup section size */
    unsigned_32     fixup_cksum;    /* fixup section checksum */
    unsigned_32     loader_size;    /* loader section size */
    unsigned_32     loader_cksum;   /* loader section checksum */
    unsigned_32     objtab_off;     /* object table offset */
    unsigned_32     num_objects;    /* number of objects in .exe */
    unsigned_32     objmap_off;     /* object page map offset */
    unsigned_32     idmap_off;      /* iterated data map offset */
    unsigned_32     rsrc_off;       /* offset of resource table */
    unsigned_32     num_rsrcs;      /* number of resource entries */
    unsigned_32     resname_off;    /* offset of resident names table */
    unsigned_32     entry_off;      /* offset of entry table */
    unsigned_32     moddir_off;     /* offset of module directives table */
    unsigned_32     num_moddirs;    /* number of module directives */
    unsigned_32     fixpage_off;    /* offset of fixup page table */
    unsigned_32     fixrec_off;     /* offset of fixup record table */
    unsigned_32     impmod_off;     /* offset of import module name table */
    unsigned_32     num_impmods;    /* # of entries in import mod name tbl */
    unsigned_32     impproc_off;    /* offset of import procedure name table */
    unsigned_32     cksum_off;      /* offset of per-page checksum table */
    unsigned_32     page_off;       /* offset of enumerated data pages */
    unsigned_32     num_preload;    /* number of preload pages */
    unsigned_32     nonres_off;     /* offset of non-resident names table */
    unsigned_32     nonres_size;    /* size of non-resident names table */
    unsigned_32     nonres_cksum;   /* non-resident name table checksum */
    unsigned_32     autodata_obj;   /* object # of autodata segment */
    unsigned_32     debug_off;      /* offset of the debugging information */
    unsigned_32     debug_len;      /* length of the debugging info */
    unsigned_32     num_inst_preload;   /* # of instance pages in preload sect*/
    unsigned_32     num_inst_demand;    /*# instance pages in demand load sect*/
    unsigned_32     heapsize;       /* size of heap - for 16-bit apps */
    unsigned_32     stacksize;      /* size of stack OS/2 only */
    union {                         /* pad to 196 bytes. */
        unsigned_8      reserved[ OSF_FLAT_RESERVED ];
        struct {
            unsigned_8      reserved1[ 8 ];   /* +0xB0 */
            unsigned_32     winresoff;        /* +0xB8 Windows VxD version info resource offset */
            unsigned_32     winreslen;        /* +0xBC Windows VxD version info resource lenght */
            unsigned_16     device_ID;        /* +0xC0 Windows VxD device ID */
            unsigned_16     DDK_version;      /* +0xC2 Windows VxD DDK version (0x030A) */
        } vxd;
    } r;
} os2_flat_header;

#define OSF_DEF_PAGE_SIZE       4096
#define OSF_FLAT_SIGNATURE      0x454C      // 'LE'
#define OSF_FLAT_LX_SIGNATURE   0x584C      // 'LX'
#define OSF_386_BYTE_ORDER      0
#define OSF_386_WORD_ORDER      0
#define OSF_EXE_LEVEL           0

#define OSF_CPU_286             1
#define OSF_CPU_386             2
#define OSF_CPU_486             3

#define OSF_OS_LEVEL            1    // OS/2
#define OSF_WIN386_LEVEL        4    // Windows 386 (VxD)

/******************************************************************************
 *
 *    The flags field low order word goes as follows:
 *
 *  x x x x  x x x x      x x x x      x x x x
 *  |   |      | | |          | |        |   |
 *  |   |      | | |          | |        |   +-> single data flag
 *  |   |      | | |          | |        +-----> if DLL, per-process
 *  |   |      | | |          | |                initialization
 *  |   |      | | |          | +--------------> no internal fixups.
 *  |   |      | | |          +----------------> no external fixups.
 *  |   |      +-+-+---------------------------> PM compatibility flags
 *  |   +------------------------------------------> errors during link
 *  |                            (not executable)
 *  +----------------------------------------------> 1=DLL, 0=program file
 *
 *    The flags field high order word goes as follows:
 *
 *  x x x x  x x x x      x x x x      x x x x
 *    |                                    | |
 *    |                                    | +-> prot. mem. lib. mod
 *    |                                    +---> device driver.
 *    +--------------------------------------------> DLL per-proc termination
 *****************************************************************************/

#define OSF_SINGLE_DATA               0x0001
#define OSF_INIT_INSTANCE             0x0004
#define OSF_INTERNAL_FIXUPS_DONE      0x0010
#define OSF_EXTERNAL_FIXUPS_DONE      0x0020
#define OSF_NOT_PM_COMPATIBLE         0x0100
#define OSF_PM_COMPATIBLE             0x0200
#define OSF_PM_APP                    0x0300
#define OSF_LINK_ERROR                0x2000
#define OSF_IS_DLL                    0x8000
#define OSF_IS_PROT_DLL           0x00010000UL
#define OSF_DEVICE_DRIVER         0x00020000UL
#define OSF_PHYS_DEVICE           0x00020000UL
#define OSF_VIRT_DEVICE           0x00028000UL
#define OSF_TERM_INSTANCE         0x40000000UL

#define VXD_DEVICE_DRIVER_3x      0x00008020UL
#define VXD_DEVICE_DRIVER_STATIC  0x00028000UL
#define VXD_DEVICE_DRIVER_DYNAMIC 0x00038000UL

typedef struct object_record {
    unsigned_32     size;       /* object virtual size */
    unsigned_32     addr;       /* base virtual address */
    unsigned_32     flags;
    unsigned_32     mapidx;     /* page map index */
    unsigned_32     mapsize;    /* number of entries in page map */
    unsigned_32     reserved;
} object_record;

/******************************************************************************
 *
 *    The flags field low order word is used as follows:
 *
 *  x x x x  x x x x      x x x x      x x x x
 *  | | | |    | | |      | | | |      | | | |
 *  | | | |    | | |      | | | |      | | | +-> readable object
 *  | | | |    | | |      | | | |      | | +---> writeable object
 *  | | | |    | | |      | | | |      | +-----> executable object
 *  | | | |    | | |      | | | |      +-------> resource object
 *  | | | |    | | |      | | | +--------------> discardable object
 *  | | | |    | | |      | | +----------------> sharable object
 *  | | | |    | | |      | +------------------> object has preload pages
 *  | | | |    | | |      +--------------------> object has invalid pages
 *  | | | |    | | +---------------------------> permanent and swappable
 *  | | | |    | +-----------------------------> permanent and resident
 *  | | | |    +-------------------------------> perm. and long lockable
 *  | | | |
 *  | | | +----------------------------------------> 16:16 alias required
 *  | | +------------------------------------------> big/default bit setting
 *  | +--------------------------------------------> conforming for code
 *  +----------------------------------------------> object io privilege level
 *
 *    The flags field high order word is used as follows:
 *
 *  x x x x  x x x x      x x x x      x x x x
 *
 *****************************************************************************/

#define OBJ_READABLE        0x0001
#define OBJ_WRITEABLE       0x0002
#define OBJ_EXECUTABLE      0x0004
#define OBJ_RESOURCE        0x0008
#define OBJ_DISCARDABLE     0x0010
#define OBJ_SHARABLE        0x0020
#define OBJ_HAS_PRELOAD     0x0040
#define OBJ_HAS_INVALID     0x0080
#define OBJ_PERM_SWAPPABLE  0x0100  /* LE */
#define OBJ_HAS_ZERO_FILL   0x0100  /* LX */
#define OBJ_PERM_RESIDENT   0x0200
#define OBJ_PERM_CONTIGUOUS 0x0300  /* LX */
#define OBJ_PERM_LOCKABLE   0x0400
#define OBJ_ALIAS_REQUIRED  0x1000
#define OBJ_BIG             0x2000
#define OBJ_CONFORMING      0x4000
#define OBJ_IOPL            0x8000

typedef struct le_map_entry {  /* LE */
    unsigned_8  page_num[3];    /* 24-bit page number in .exe file */
    unsigned_8  flags;
} le_map_entry;

typedef struct lx_map_entry { /* LX */
    unsigned_32     page_offset;        /* offset from Preload page start
                                            shifted by page_shift in hdr */
    unsigned_16     data_size;          /* # bytes in this page */
    unsigned_16     flags;
} lx_map_entry;

typedef union {
    le_map_entry    le;
    lx_map_entry    lx;
} map_entry;

#define PAGE_VALID      0
#define PAGE_ITERATED   1
#define PAGE_INVALID    2
#define PAGE_ZEROED     3
#define PAGE_RANGE      4


typedef struct flat_bundle_prefix {
    unsigned_8      b32_cnt;
    unsigned_8      b32_type;
    unsigned_16     b32_obj;
} flat_bundle_prefix;

typedef struct flat_null_prefix {
    unsigned_8      b32_cnt;
    unsigned_8      b32_type;
} flat_null_prefix;

/* values for the b32_type field */
enum bundle_types {
    FLT_BNDL_EMPTY  = 0,
    FLT_BNDL_ENTRY16,
    FLT_BNDL_GATE16,
    FLT_BNDL_ENTRY32,
    FLT_BNDL_ENTRYFWD
};

typedef struct flat_bundle_entry32 {
    unsigned_8      e32_flags;      /* flag bits are same as in OS/2 1.x */
    unsigned_32     e32_offset;
} flat_bundle_entry32;

typedef struct flat_bundle_gate16 {
    unsigned_8      e32_flags;      /* flag bits are same as in OS/2 1.x */
    unsigned_16     offset;
    unsigned_16     callgate;
} flat_bundle_gate16;

/*
 * other, unused bundle types are:
 */

typedef struct flat_bundle_entry16 {
    unsigned_8      e32_flags;      /* flag bits are same as in OS/2 1.x */
    unsigned_16     e32_offset;
} flat_bundle_entry16;

typedef struct flat_bundle_entryfwd {
    unsigned_8      e32_flags;      /* flag bits are same as in OS/2 1.x */
    unsigned_16     modord;
    unsigned_32     value;
} flat_bundle_entryfwd;

typedef struct flat_res_table {
    unsigned_16    type_id;
    unsigned_16    name_id;
    unsigned_32    res_size;
    unsigned_16    object;
    unsigned_32    offset;
} flat_res_table;

/* fixup record source flags */

#define OSF_SOURCE_MASK             0x0F
#define OSF_SOURCE_BYTE             0x00
#define OSF_SOURCE_UNDEFINED1       0x01
#define OSF_SOURCE_SEG              0x02
#define OSF_SOURCE_PTR_32           0x03
#define OSF_SOURCE_UNDEFINED4       0x04
#define OSF_SOURCE_OFF_16           0x05
#define OSF_SOURCE_PTR_48           0x06
#define OSF_SOURCE_OFF_32           0x07
#define OSF_SOURCE_OFF_32_REL       0x08

#define OSF_SFLAG_FIXUP_TO_ALIAS    0x10
#define OSF_SFLAG_LIST              0x20

/* fixup record target flags */

#define OSF_TARGET_MASK             0x03
#define OSF_TARGET_INTERNAL         0x00
#define OSF_TARGET_EXT_ORD          0x01
#define OSF_TARGET_EXT_NAME         0x02
#define OSF_TARGET_INT_VIA_ENTRY    0x03

#define OSF_TFLAG_ADDITIVE_VAL      0x04
#define OSF_TFLAG_INT_CHAIN         0x08
#define OSF_TFLAG_OFF_32BIT         0x10
#define OSF_TFLAG_ADD_32BIT         0x20
#define OSF_TFLAG_OBJ_MOD_16BIT     0x40
#define OSF_TFLAG_ORDINAL_8BIT      0x80

typedef struct {
    	// source type
	unsigned char type;
	// target flags
	unsigned char flags;
	// source offset or source list count
	unsigned short src_off;
	// internal fixup target data
	unsigned short object;
	unsigned long target_off;
	// external fixup target data
	unsigned short mod_ord;
	unsigned short imp_ord;
	unsigned long name_off;
	unsigned long additive;
	// source list
	unsigned short *source_list;
} lx_fixup;

typedef struct {
	char *name;
	struct stat *stat;
	IMAGE_DOS_HEADER mz;
	os2_flat_header lx;
	object_record *object_records;
	map_entry *object_map;
	unsigned long *fixup_map;
	// by page and fixup index
	lx_fixup ***fixups;
	// by page
	int *fixup_count;
	char **pages;
} exe;

typedef struct {
	WINDOW *win;
	void *(*input)(int key);
	void (*refresh)();
} lxedit_window;

typedef struct {
	int screen_width;
	int screen_height;
	WINDOW *header;
	WINDOW *tree;
	WINDOW *tree_head;
	WINDOW *status;
	int active_exe;
	int active_object;
	int show_object;
	int active_page;
	int scroll_page;
	lxedit_window *active_window;
	int active_fixup;
	int scroll_fixup;

} lxedit_layout;

typedef struct {
	lxedit_layout *layout;
	int num_executables;
	exe **executables;
} lxedit;

extern lxedit *lx;
extern lxedit_window *objects_window;
extern lxedit_window *pages_window;
extern lxedit_window *fixups_window;
extern lxedit_window *info_window;

lxedit_window *objects_window_create(void);
lxedit_window *pages_window_create(void);
lxedit_window *fixups_window_create(void);
lxedit_window *info_window_create(void);

#endif // __LX_H
