#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "m.h"

/** General methods */
#define GET_BYTE(x) (ramdisk[x])
// Get bit from left to right. i.e. 01234567
#define GET_BIT(x, y) (((GET_BYTE(x)) >> (7-y)) & 0x01)
// Create 1 on bit y (all others 0)
#define BIT_MASK_1(y) ((0x01 << (7-y)))
// Create 0 on bit y (all others 1)
#define BIT_MASK_0(y) (~BIT_MASK_1(y))
// Get integer pointer
#define GET_INT_PTR(X) ((int *)&ramdisk[X])
// Get integer
#define GET_INT(X) (*(int *)&ramdisk[X])
// Set Integer
#define SET_INT(X, Y); {int* temp=GET_INT_PTR(X); *temp=Y;}

/** Bitmap block */ 
#define BITMAPBLOCK_START (INODEBLOCK_END + 1)
#define BITMAPBLOCK_SIZE (BLOCK_SIZE * 4)
#define BITMAPBLOCK_END (BITMAPBLOCK_START + BITMAPBLOCK_SIZE - 1)
/** Bitmap helper methods*/
/** Total blocks possible is 2MB/256 = 8192. Subtract supepr/inode/bitmap (1+256+4) => 7931 blocks. In which 7931 is the total blocks for our partition. Also, 8192 happens to be the total bit length of our bitmap. Hence, our bitmap can use MOD operator to easily manipulate bits with the starting address of the block. (We ignore first 261 bits since that is reserved for our super+inode+bitmap blocks)
 */
// Given a location(block starting address) find the block number, i.e. location of the bit in bitmap
#define GET_BLOCK_NUMBER(X) (X / 256)
#define GET_BYTE_NUMBER(X) ((GET_BLOCK_NUMBER(X) / 8) + BITMAPBLOCK_START)
#define GET_BYTE_OFFSET(X) (GET_BLOCK_NUMBER(X) % 8)
#define SET_BIT_TO_1_BYTE(x, y) (GET_BYTE(x) = ((GET_BYTE(x)) | BIT_MASK_1(y)))
#define SET_BIT_TO_0_BYTE(x, y) (GET_BYTE(x) = ((GET_BYTE(x)) & BIT_MASK_0(y)))

/** Bitmap block methods */
#define SET_BITMAP_ALLOCATE_BLOCK(X) (SET_BIT_TO_1_BYTE((GET_BYTE_NUMBER(X)),(GET_BYTE_OFFSET(X))))
#define SET_BITMAP_FREE_BLOCK(X) (SET_BIT_TO_0_BYTE((GET_BYTE_NUMBER(X)),(GET_BYTE_OFFSET(X))))
#define SEE_BITMAP_ALLOCATE(X) ((GET_BIT(GET_BYTE_NUMBER(X), GET_BYTE_OFFSET(X)) == 1) ? 1 : 0)
#define SEE_BITMAP_FREE(X) ((GET_BIT(GET_BYTE_NUMBER(X), GET_BYTE_OFFSET(X)) == 0) ? 1 : 0)

/** MAXIMUM */
#define MAX_FILE_COUNT INODE_NUMBER
#define MAX_FILE_SIZE ((64*256)+(64*64*256)+256*8)

char dir[4] = "dir";
char reg[4] = "reg";
struct Ramdisk *rd;

void init_fs() {
    printf("INIT\n");
    // INIT FILESYSTEM:
    rd = (struct Ramdisk *)malloc(sizeof(struct Ramdisk));
    printf("RAMDISK Size=%d\n", (int)sizeof(struct Ramdisk));
    // Setting up root
    SET_INODE_TYPE_DIR(0);
    //printf("type=%s\n", rd->ib[0].type);
    SET_INODE_SIZE(0, 0);
    //printf("size=%d\n", rd->ib[0].size);
    union Block root;
    rd->pb[0]=root;
    rd->ib[0].blocks[0]=&root;
    INIT_FREEINODE;
    INIT_FREEBLOCK;
    DECR_FREEBLOCK; // -1 for ROOT
    DECR_FREEINODE; // -1 for ROOT
    // EOF init
}


// Write to file block/and get from. (NO REDIRECTION)
#define WRITE_TO_FILE(BLOCK, BYTES); {memcpy(rd->pb[BLOCK].reg.byte, BYTES, 256);}
#define GET_FROM_FILE(BLOCK) rd->pb[BLOCK].reg.byte

// Assign BLOCK location to #Inode's #index. => Assign location in inode (regular file block (0~8))
#define ASSIGN_LOCATION(INODE, INDEX, BLOCK); {rd->ib[INODE].blocks[INDEX] = &rd->pb[BLOCK];}
#define GET_FROM_LOCATION(INODE, INDEX) (*rd->ib[INODE].blocks[INDEX]).reg.byte

int main() {
    init_fs();
#ifdef debug
    PRINT_FREEINODE_COUNT;
    PRINT_FREEBLOCK_COUNT;
    // test set filename
    char n[16] = "non-root";
    SET_DIR_ENTRY_NAME(0, 0, n);
    PRINT_DIR_ENTRY_NAME(0, 0);
#endif
    // test write to reg file block
    unsigned char x[260] = "asdfasdfasdfasdfasdfasdf";
    WRITE_TO_FILE(1, x)
    printf("Print: %s\n", GET_FROM_FILE(1));

    // Assign location in inode (regular file block (0~8))
    ASSIGN_LOCATION(1, 0, 1);
    printf("Print: %s\n", GET_FROM_LOCATION(1, 0));
    
}

