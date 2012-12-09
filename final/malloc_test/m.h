#define BLOCK_SIZE 256
#define RAMDISK_SIZE (2*1024*1024)

/** Superblock*/
#define SHOW_FREEINODE rd->sb.freeinode
#define PRINT_FREEINODE_COUNT {printf("FREEINODE = %d\n", SHOW_FREEINODE);}
#define INIT_FREEINODE SHOW_FREEINODE = MAX_FILE_COUNT
#define DECR_FREEINODE SHOW_FREEINODE -= 1
#define INCR_FREEINODE SHOW_FREEINODE += 1
#define SHOW_FREEBLOCK rd->sb.freeblock
#define PRINT_FREEBLOCK_COUNT {printf("FREEBLOCK = %d\n", SHOW_FREEBLOCK);}
#define INIT_FREEBLOCK SHOW_FREEBLOCK = PARTITION_NUMBER
#define DECR_FREEBLOCK SHOW_FREEBLOCK -= 1
#define INCR_FREEBLOCK SHOW_FREEBLOCK += 1

/** Inode block */
#define INODEBLOCK_NUMBER 256
#define INODEBLOCK_SIZE (BLOCK_SIZE * INODEBLOCK_NUMBER)
#define INODE_SIZE 64
#define INODE_NUMBER (INODEBLOCK_SIZE/INODE_SIZE)

#define SET_INODE_TYPE_DIR(INDEX); {memcpy(rd->ib[INDEX].type, dir, 4);}
#define SET_INODE_TYPE_REG(INDEX); {memcpy(rd->ib[INDEX].type, reg, 4);}
#define SET_INODE_SIZE(INDEX, SIZE); {rd->ib[INDEX].size = SIZE;}

/** Partition */
#define PARTITION_SIZE (RAMDISK_SIZE-(256*(1+256+4)))
#define PARTITION_NUMBER (PARTITION_SIZE/BLOCK_SIZE)

// Copy only 13, leave 14th for Null.
#define SET_DIR_ENTRY_NAME(BLOCK, ENTRY, NAME); {memcpy(rd->pb[BLOCK].dir.ent[ENTRY].filename, NAME, 13);}
#define GET_DIR_ENTRY_NAME(BLOCK, ENTRY) rd->pb[BLOCK].dir.ent[ENTRY].filename
#define PRINT_DIR_ENTRY_NAME(BLOCK, ENTRY); {printf("Filename = %s\n", GET_DIR_ENTRY_NAME(BLOCK, ENTRY));}

struct Superblock {
    int freeinode;
    int freeblock;
    unsigned char padding[248];
};

struct Inode {
    char type[4];
    int size;
    union Block *blocks[10];
    unsigned char padding[16];
};

struct Block_reg {
    unsigned char byte[256];
};

struct Dir_entry {
    // Contain in Block (type: dir)
    char filename[14];
    short inode_number;
};

struct Block_dir {
    struct Dir_entry ent[16];
};


struct Block_ptr {
    // Block of pointers, for large file. 4=ptr size;
    union Block *blocks[BLOCK_SIZE/4];
};


union Block {
    struct Block_reg reg;
    struct Block_dir dir;
    struct Block_ptr ptr;
};

struct Bitmap_block {
    unsigned char byte[256*4];
};

struct Ramdisk{
    struct Superblock sb;
    struct Inode ib[INODE_NUMBER];
    struct Bitmap_block bb;
    union Block pb[PARTITION_NUMBER];
};