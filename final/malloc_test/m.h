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
char dir[4] = "dir";
char reg[4] = "reg";
#define SET_INODE_TYPE_DIR(INDEX); {memcpy(rd->ib[INDEX].type, dir, 4);}
#define SET_INODE_TYPE_REG(INDEX); {memcpy(rd->ib[INDEX].type, reg, 4);}
#define SET_INODE_SIZE(INDEX, SIZE); {rd->ib[INDEX].size = SIZE;}


/** General methods */
// Create 1 on bit y (all others 0)
#define BIT_MASK_1(y) ((0x01 << (7-y)))
// Create 0 on bit y (all others 1)
#define BIT_MASK_0(y) (~BIT_MASK_1(y))

/** Bitmap block */
/** Total blocks possible is 2MB/256 = 8192. Subtract supepr/inode/bitmap (1+256+4) => 7931 blocks. In which 7931 is the total blocks for our partition. Since we are only interested in whether a partition block is free or not. We can use the block number indicate the bit in the bitmap. I.e. Get which byte the location is in, and the offset. Then set it to 1.
 */
#define GET_BYTE_NUMBER(BLOCK) (BLOCK / 8)
#define GET_BYTE_OFFSET(BLOCK) (BLOCK % 8)
#define SET_BITMAP_ALLOCATE_BLOCK(BLOCK) (rd->bb.byte[GET_BYTE_NUMBER(BLOCK)] = (rd->bb.byte[GET_BYTE_NUMBER(BLOCK)] | BIT_MASK_1(GET_BYTE_OFFSET(BLOCK))))
#define SET_BITMAP_FREE_BLOCK(BLOCK) (rd->bb.byte[GET_BYTE_NUMBER(BLOCK)] = (rd->bb.byte[GET_BYTE_NUMBER(BLOCK)] & BIT_MASK_0(GET_BYTE_OFFSET(BLOCK))))
#define PRINT_BITMAP_BYTE(BYTE); {int zzzzzz; for(zzzzzz=0; zzzzzz < 8; zzzzzz++) {printf("%d", (rd->bb.byte[BYTE] >> (7-zzzzzz)) & 0x01);} printf("\n");}

#define PRINT_BITMAP_BIT_STATUS(block_number); {printf("Block %d: allocated?=%d, freed?=%d\n", block_number, SEE_BITMAP_ALLOCATE(block_number), SEE_BITMAP_FREE(block_number));}
#define SEE_BITMAP_ALLOCATE(BLOCK) (((rd->bb.byte[GET_BYTE_NUMBER(BLOCK)] >> (7-GET_BYTE_OFFSET(BLOCK))) == 1) ? 1 : 0)
#define SEE_BITMAP_FREE(BLOCK) (((rd->bb.byte[GET_BYTE_NUMBER(BLOCK)] >> (7-GET_BYTE_OFFSET(BLOCK))) == 0) ? 1 : 0)


/** Partition */
#define PARTITION_SIZE (RAMDISK_SIZE-(256*(1+256+4)))
#define PARTITION_NUMBER (PARTITION_SIZE/BLOCK_SIZE)
// Partition Methods
// Write to file block/and get from. (NO REDIRECTION)
#define WRITE_TO_FILE(BLOCK, BYTES); {memcpy(rd->pb[BLOCK].reg.byte, BYTES, 256);}
#define GET_FROM_FILE(BLOCK) rd->pb[BLOCK].reg.byte

// Assign BLOCK location to #Inode's #index. => Assign location in inode (regular file block (0~8))
#define ASSIGN_LOCATION(INODE, INDEX, BLOCK); {rd->ib[INODE].blocks[INDEX] = &rd->pb[BLOCK];}
#define GET_FROM_LOCATION(INODE, INDEX) (*rd->ib[INODE].blocks[INDEX]).reg.byte

// To assign Single redirection block to #Inode's 8 location. Use ASSIGN_LOCATION(INODE, 8, BLOCK). Use WRITE_TO_FILE(block, bytes) to directly write to block.
// Assign BLOCK to single redirection block
#define ASSIGN_LOCATION_SINGLE_RED(INODE, PTR_ENTRY, BLOCK); {rd->ib[INODE].blocks[8]->ptr.blocks[PTR_ENTRY] = &rd->pb[BLOCK];}
#define GET_FROM_LOCATION_SINGLE_RED(INODE, PTR_ENTRY) (*(*rd->ib[INODE].blocks[8]).ptr.blocks[PTR_ENTRY]).reg.byte

// Double redirection block: Use ASSIGN_LOCATION(Inode, 9, block) for first redirection block. Use WRITE_TO_FILE(block, bytes) to directly write to final block.
#define ASSIGN_LOCATION_DOUBLE_FST_RED(INODE, PTR_ENTRY, BLOCK); {rd->ib[INODE].blocks[9]->ptr.blocks[PTR_ENTRY] = &rd->pb[BLOCK];}
#define ASSIGN_LOCATION_DOUBLE_SND_RED(INODE, PTR_ENTRY1, PTR_ENTRY2, BLOCK) {rd->ib[INODE].blocks[9]->ptr.blocks[PTR_ENTRY1]->ptr.blocks[PTR_ENTRY2] = &rd->pb[BLOCK];}
#define GET_FROM_LOCATION_DOUBLE_RED(INODE, PTR_ENTRY1, PTR_ENTRY2) (*(*(*rd->ib[INODE].blocks[9]).ptr.blocks[PTR_ENTRY1]).ptr.blocks[PTR_ENTRY2]).reg.byte


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