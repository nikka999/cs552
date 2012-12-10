#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "m.h"

/** MAXIMUM */
#define MAX_FILE_COUNT INODE_NUMBER
#define MAX_FILE_SIZE ((64*256)+(64*64*256)+256*8)

// For fd. Since the maximum file number is 1024. We assign it to be 1024.
// Put fd number to the fd_table.
short fd_table[1024];
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

int set_inode_reg_file(unsigned char *file, long int size) {
    //printf("Size=?%ld, Max_size?=%d\n", size, MAX_FILE_SIZE);
    if (size > MAX_FILE_SIZE) {
        return -1;
    }
    return 0;
    // 1st Block: 0~255; 2nd Block: 256~511; 3rd Block: 512~767; 4th Block: 768~1023; 5th Block: 1024~1279; 6th Block: 1280~1535; 7th Block: 1536~1791; 8th Block: 1792~2047; TOTAL:2048
    int number_blocks = size % 256;
    if (number_blocks > SHOW_FREEBLOCK_COUNT) {
        return -1;
    }
    int i = 1;
    while (i <= number_blocks) {
        if (i<=8) {
            // Direct Block Regular File.
            // 1. Find a free block;
            // 2. Assign block to inode index X-1, and write to block
            // 3. Decrement freeblock count;
            DECR_FREEBLOCK;
        } else if (i > 8 && i <= 72) {
            // Single Redirection block, are block #9~72
        } else if (i > 72 && i <= 4168) {
            // Double Redirection block, are block #73~4168
        }
        i++;
    }
    
}

int main() {
    init_fs();
    
    unsigned char file[ MAX_FILE_SIZE +1];
    int x = set_inode_reg_file(file, sizeof(file));
    printf("Size=?%d, Allocated?=%d\n", (int)sizeof(file), x);
    
#ifdef debug
    PRINT_FREEINODE_COUNT;
    PRINT_FREEBLOCK_COUNT;

    // test set filename
    char n[16] = "non-root";
    SET_DIR_ENTRY_NAME(0, 0, n);
    PRINT_DIR_ENTRY_NAME(0, 0);

    // test write to reg file block
    unsigned char x[260] = "testing=testing2=testing3";
    WRITE_TO_FILE(1, x)
    printf("Print direct block write: %s\n", GET_FROM_FILE(1));

    // Assign location in inode (regular file block (0~7))
    ASSIGN_LOCATION(1, 0, 1);
    printf("Print direct block Inode: %s\n", GET_FROM_LOCATION(1, 0));
    
    // Single redirection
    ASSIGN_LOCATION(1, 8, 2);
    ASSIGN_LOCATION_SINGLE_RED(1, 0, 3);
    WRITE_TO_FILE(3, x);
    printf("Print single redir block: %s\n", GET_FROM_LOCATION_SINGLE_RED(1, 0));
    
    // Double redirection
    ASSIGN_LOCATION(1, 9, 4);
    ASSIGN_LOCATION_DOUBLE_FST_RED(1, 0, 5);
    WRITE_TO_FILE(6, x);
    ASSIGN_LOCATION_DOUBLE_SND_RED(1, 0, 0, 6);
    printf("Print double redir block: %s\n", GET_FROM_LOCATION_DOUBLE_RED(1, 0, 0));

    
    // Test bitmap
    int byte=0;
    int block_number =7;
    SET_BITMAP_ALLOCATE_BLOCK(block_number);
    
    PRINT_BITMAP_BIT_STATUS(block_number);
    
    PRINT_BITMAP_BYTE(byte);
    
    SET_BITMAP_FREE_BLOCK(block_number);
    
    PRINT_BITMAP_BIT_STATUS(block_number);
    
    PRINT_BITMAP_BYTE(byte);
#endif    

    // Test freeing a block. directly
    


}

