#include <stdio.h>
#include <stdlib.h>

// All START and END are inclusive
#define RAMDISK_SIZE (2*1024*1024)
#define BLOCK_SIZE 256

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

/** Superblock */
#define SUPERBLOCK_START 0
#define SUPERBLOCK_END (SUPERBLOCK_START + BLOCK_SIZE - 1)

/** Inode block */
#define INODEBLOCK_START (SUPERBLOCK_END + 1)
#define INODEBLOCK_NUMBER 256
#define INODEBLOCK_SIZE (BLOCK_SIZE * INODEBLOCK_NUMBER)
#define INODEBLOCK_END (INODEBLOCK_START + INODEBLOCK_SIZE - 1)
#define INODE_SIZE 64
#define INODE_NUMBER (INODEBLOCK_SIZE/INODE_SIZE)
/** Each inode is NAME(16byte (15 char + 1 NULL)) + TYPE(4byte) + SIZE(4byte) + LOCATION(4*10byte)
 */

/** Inode block methods */
#define SET_TYPE_REG(x) ramdisk[x+16] = 'r'; ramdisk[x+17] = 'e'; ramdisk[x+18] = 'g'; ramdisk[x+19]='\0';
#define SET_TYPE_DIR(x) ramdisk[x+16] = 'd'; ramdisk[x+17] = 'i'; ramdisk[x+18] = 'r'; ramdisk[x+19]='\0';
#define SET_SIZE(x, y); {SET_INT(x+20, y);}
#define GET_SIZE(x) (GET_INT(x+20))


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

/** Partition */
#define PARTITION_START (BITMAPBLOCK_END + 1)
#define PARTITION_END (RAMDISK_SIZE - 1)
#define PARTITION_SIZE ((PARTITION_END + 1 - PARTITION_START))
#define PARTITION_NUMBER (PARTITION_SIZE/BLOCK_SIZE)

/** MAXIMUM */
#define MAX_FILE_COUNT INODE_NUMBER
#define MAX_FILE_SIZE ((64*256)+(64*64*256)+256*8)



unsigned char *ramdisk;
int freeblock = PARTITION_NUMBER;
int freeinode = INODE_NUMBER;

int main() {
	printf("Load\n");
    ramdisk = (unsigned char *)malloc(2097152);
    if (ramdisk==NULL) {
        printf("Malloc error\n");
    } else {
        printf("Malloc successful\n");
    }
    /**
    printf("Free block: %d\n", freeblock);
    printf("Free inode: %d\n", freeinode);
    printf("Max_file count: %d\n", MAX_FILE_COUNT);
    printf("Max_file size: %d\n", MAX_FILE_SIZE);
    */
    
    // Test set type and set integer.
    int x = 10;
    SET_TYPE_DIR(x);
    printf("%s\n", &ramdisk[26]);
    int j;
    for (j = x; j < (x+64); j++) {
        printf("%d=%c  ", ramdisk[j], ramdisk[j]);
    }
    printf("\n");
    
    //int *size = GET_INT_PTR(100);
    //*size = 2097152;
    
    SET_SIZE(x, 19234);
    printf("%d\n", GET_SIZE(x));
    
    for (j = x; j < (x+64); j++) {
        printf("%d=%c  ", ramdisk[j], ramdisk[j]);
    }
    printf("\n");
    
    int i;
    
    //for (i = 0; i < BITMAPBLOCK_START; i++) {
    //    ramdisk[i] = i % 256;
    //}

    //for (i = SUPERBLOCK_START; i < SUPERBLOCK_END; i++) {
    //    printf("%d: %d <=> \t", i, ramdisk[i]);
    //}
    //for (i = BITMAPBLOCK_START; i <= BITMAPBLOCK_END; i++) {
    //    printf("%d: %d <=> \t", i, ramdisk[i]);
    //}
    
    // Test get bit
    /**
    int j = BITMAPBLOCK_START-1;
    printf("%d\nBIT=", GET_BYTE(j));
    for (i = 0; i < 8; i++) {
        printf("%d", GET_BIT(j, i));

    }
    printf("\n");
    */
    
    /**
    int x = BITMAPBLOCK_START-1;
    int y = 8;
    printf("ORIG: %d\n", ramdisk[x]);
    SET_BIT_TO_1_BYTE(x, y);
    printf("Set %d bit to 1, bytes=", y);
    for (i = 0; i < 8; i++) {
        printf("%d", ((ramdisk[x] >> (7-i)) & 0x01));
    }
    printf("\n");
    SET_BIT_TO_0_BYTE(x, y);
    printf("Set %d bit to 1, bytes=", y);
    for (i = 0; i < 8; i++) {
        printf("%d", ((ramdisk[x] >> (7-i)) & 0x01));
    }
    printf("\n");
    */
    
    
/** test set allocate and free
    printf("%d bitmapblock start, %d bitmapblock end, %d partition start, %d partition end, %d partition size, %d partition number\n", BITMAPBLOCK_START, BITMAPBLOCK_END, PARTITION_START, PARTITION_END, PARTITION_SIZE, PARTITION_NUMBER);
    
    int f= 256*(261);
    printf("Address: %d, GET_BYTE_NUMBER=%d, GET_BYTE_OFFSET=%d\n", f, GET_BYTE_NUMBER(f), GET_BYTE_OFFSET(f));
    SET_BITMAP_ALLOCATE_BLOCK(f);
    printf("Print bitmap\n");
    int j;
    for (j=BITMAPBLOCK_START; j<=BITMAPBLOCK_END; j++) {
        printf("byte=%d\t", j);
        for (i = 0; i < 8; i++) {
            printf("%d", GET_BIT(j, i));
        }
        printf("\n");
    }
    printf("EOF PRINT BITMAP\n");
    
    printf("Is %d block allocate?=%d, free=%d?\n", f, SEE_BITMAP_ALLOCATE(f), SEE_BITMAP_FREE(f));
    
    SET_BITMAP_FREE_BLOCK(f);
    printf("Print bitmap\n");
    for (j=BITMAPBLOCK_START; j<=BITMAPBLOCK_END; j++) {
        printf("byte=%d\t", j);
        for (i = 0; i < 8; i++) {
            printf("%d", GET_BIT(j, i));
        }
        printf("\n");
    }
    printf("EOF PRINT BITMAP\n");
    printf("Is %d block allocate?=%d, free=%d?\n", f, SEE_BITMAP_ALLOCATE(f), SEE_BITMAP_FREE(f));
*/
    

}
