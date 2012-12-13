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
    // This is DIR block for future subfile for root.
    rd->pb[0]=root;
    rd->ib[0].blocks[0]=&root;
    INIT_FREEINODE;
    INIT_FREEBLOCK;
    // Mark bitmap for root
    SET_BITMAP_ALLOCATE_BLOCK(0);
    DECR_FREEBLOCK; // -1 for ROOT
    DECR_FREEINODE; // -1 for ROOT
    // EOF init
}
int find_free_block() {
    // Find a new free block, using first-fit.
    if (SHOW_FREEBLOCK == 0) {
        printf("Find Free Block: FREEBLOCK=0, returning -1 \n");
        return -1;
    } else {
        int j = 0;
        for (j; j < PARTITION_NUMBER; j++) {
            
            if (SEE_BITMAP_FREE(j) == 1) {
                //printf("%d\n", j);
                return j;
            }
        }
    }
    printf("No free block found, ERROR\n");
    return -1;
}

int check_for_last_write(int size, int ptr_count, unsigned char *file, int fb) {
    if ((size - ptr_count) <=  256) {
        // To prevent garbage in our partition block.
        // NOTE: Since we know the inode size (for reading), we can skip this step
        // but to keep our partition clean, lets do it anyway.
        unsigned char temp[256];
        memcpy(temp, &file[ptr_count], (size - ptr_count));
        WRITE_TO_FILE(fb, temp);
        return 1;
    }
    return -1;
}

int set_inode_reg_file(unsigned char *file, short inode, int size) {
    //printf("Size=?%ld, Max_size?=%d\n", size, MAX_FILE_SIZE);
    if (size > MAX_FILE_SIZE) {
        return -1;
    }
    
    // 1st Block: 0~255; 2nd Block: 256~511; 3rd Block: 512~767; 4th Block: 768~1023; 5th Block: 1024~1279; 6th Block: 1280~1535; 7th Block: 1536~1791; 8th Block: 1792~2047; TOTAL:2048
    int number_blocks = size / 256;
    if ((size % 256) != 0) {
        // Increment if not multiply of 256
        number_blocks += 1;
    }
    int ptr_count = 0;

    printf("Number of blocks=%d\n", number_blocks);

    PRINT_FREEBLOCK_COUNT;
    if (number_blocks > SHOW_FREEBLOCK) {
        return -1;
    }
    int i = 1;
    while (i <= number_blocks) {
        if (i<=8) {
            //printf("In first if, i=%d\n", i);
            // Direct Block Regular File.
            // 1. Find a free block;
            int fb = find_free_block();
            SET_BITMAP_ALLOCATE_BLOCK(fb);
            // 2. Assign block to inode index X-1, and write to block
            ASSIGN_LOCATION(inode, i-1, fb);
            DECR_FREEBLOCK;
            
            if (check_for_last_write(size, ptr_count, file, fb) == 1){
                PRINT_FREEBLOCK_COUNT;
                return 1;
            }
            WRITE_TO_FILE(fb, file);
        } else if (i > 8 && i <= 72) {
            // Single Redirection block, are block #9~72
            if(i==9) {
                // If the first single redirection block, we need to allocate another block for that.
                // 1. Find a free block;
                int fb = find_free_block();
                SET_BITMAP_ALLOCATE_BLOCK(fb);
                // 2. Assign block to inode index 8
                ASSIGN_LOCATION(inode, 8, fb);
                // 3. Find another free block;
                fb = find_free_block();
                SET_BITMAP_ALLOCATE_BLOCK(fb);
                // 4. Assign block to single redirection index 0
                ASSIGN_LOCATION_SINGLE_RED(inode, 0, fb);
                //printf("In else-if-if, i=%d\n", i);
                DECR_FREEBLOCK; // Single redirection block
                DECR_FREEBLOCK; // First regular block for the single redirection.
                
                if (check_for_last_write(size, ptr_count, file, fb) == 1){
                    PRINT_FREEBLOCK_COUNT;
                    return 1;
                }
                WRITE_TO_FILE(fb, file);
            } else {
                // Else we already have the single redirection block, just allocate another block for direct file.
                // 1. Find a free block;
                int fb = find_free_block();
                SET_BITMAP_ALLOCATE_BLOCK(fb);
                // 2. Assign block to single redirection index 1~63
                //printf("In else-if-else, i=%d\n", i-9);
                ASSIGN_LOCATION_SINGLE_RED(inode, i-9, fb);
                DECR_FREEBLOCK;
                
                if (check_for_last_write(size, ptr_count, file, fb) == 1){
                    PRINT_FREEBLOCK_COUNT;
                    return 1;
                }
                WRITE_TO_FILE(fb, file);
            }
        } else if (i > 72 && i <= 4168) {
            // Double Redirection block, are block #73~4168
            if ((i-72)%64 == 1) {
                //printf("In elseif-if, i=%d\n", i);
                int fb;
                if (i==73) {
                    // We need to allocate a double redirection block first for i=73
                    // 1. Find a free block;
                    fb = find_free_block();
                    SET_BITMAP_ALLOCATE_BLOCK(fb);
                    // 2. Assign block to inode index 9
                    ASSIGN_LOCATION(inode, 9, fb);
                    DECR_FREEBLOCK;
                }
                // No matter i=73 or not, we still need to allocate 2nd redirection block for when (i-72)%64=1; (Mod 64 because one ptr block can only have 64 ptrs. When %64=1, we know that this is a new block.)
                // 1. Find a free block;
                fb = find_free_block();
                SET_BITMAP_ALLOCATE_BLOCK(fb);
                DECR_FREEBLOCK;
                // 2. Assign 2nd redir block to 1st redriction block index 0~63. USE: (i-72)/64 to determine index
                ASSIGN_LOCATION_DOUBLE_FST_RED(inode, (i-72)/64, fb);
                // 3. Find a new block.
                // 4. Assign block to 2nd redirection (index=0)
                //printf("In elseif-if2, i=%d\n", (i-72)/64);
                fb = find_free_block();
                SET_BITMAP_ALLOCATE_BLOCK(fb);
                
                ASSIGN_LOCATION_DOUBLE_SND_RED(inode, (i-72)/64, 0, fb);
                DECR_FREEBLOCK;
                
                if (check_for_last_write(size, ptr_count, file, fb) == 1){
                    PRINT_FREEBLOCK_COUNT;
                    return 1;
                }
                WRITE_TO_FILE(fb, file);
            } else {
                // 1. Find new block
                int fb = find_free_block();
                SET_BITMAP_ALLOCATE_BLOCK(fb);
                // 2. Find the location it should be inserted. (index=9, 1st=(i-74)/64, snd=(i-73)%64) (location should be 1~63)
                //printf("2nd redir, 1st=%d, 2nd=%d\n", (i-74)/64, (i-73)%64);
                // 3. Assign block to 2nd redir block;
                ASSIGN_LOCATION_DOUBLE_SND_RED(inode, (i-74)/64, (i-73)%64, fb);
                DECR_FREEBLOCK;
                if (check_for_last_write(size, ptr_count, file, fb) == 1){
                    PRINT_FREEBLOCK_COUNT;
                    return 1;
                }
                WRITE_TO_FILE(fb, file);
            }
        }
        i++;
        ptr_count += 256;
    }
    PRINT_FREEBLOCK_COUNT;
    return 1;
}

int check_for_last_read(int size, int ptr_count, unsigned char *file, int fb) {
    if ((size - ptr_count) <=  256) {
        // To prevent garbage in our partition block.
        // NOTE: Since we know the inode size (for reading), we can skip this step
        // but to keep our partition clean, lets do it anyway.
        unsigned char temp[256];
        memcpy(temp, &file[ptr_count], (size - ptr_count));
        WRITE_TO_FILE(fb, temp);
        return 1;
    }
    return -1;
}

int get_inode_reg_file(short inode, int size, unsigned char *file) {
    //printf("Size=?%ld, Max_size?=%d\n", size, MAX_FILE_SIZE);
    if (size > MAX_FILE_SIZE) {
        return -1;
    }
    int number_blocks = size / 256;
    if ((size % 256) != 0) {
        // Increment if not multiply of 256
        number_blocks += 1;
    }
    int ptr_count = 0;
    int i = 1;
    while (i <= number_blocks) {
        if (i<=8) {
            //printf("In first if, i=%d\n", i);
            // Direct Block Regular File.
            if ((size-ptr_count)<= 256) {
                memcpy(file + ptr_count, GET_FROM_LOCATION(inode, (i-1)), (size-ptr_count));
                return 1;
            }
            memcpy(file + ptr_count, GET_FROM_LOCATION(inode, (i-1)), 256);
        } else if (i > 8 && i <= 72) {
            // Single Redirection block, are block #9~72
            if ((size-ptr_count)<= 256) {
                memcpy(file + ptr_count, GET_FROM_LOCATION_SINGLE_RED(inode, (i-9)), (size-ptr_count));
                return 1;
            }
            memcpy(file + ptr_count, GET_FROM_LOCATION_SINGLE_RED(inode, (i-9)), 256);
        } else if (i > 72 && i <= 4168) {
            // Double Redirection block, are block #73~4168
            if ((size-ptr_count)<= 256) {
                memcpy(file + ptr_count, GET_FROM_LOCATION_DOUBLE_RED(inode, (i-74)/64, (i-73)%64), (size-ptr_count));
                return 1;
            }
            memcpy(file + ptr_count, GET_FROM_LOCATION_DOUBLE_RED(inode, (i-74)/64, (i-73)%64), 256);
        }
        i++;
        ptr_count += 256;
    }
    return 1;
}

int main() {
    init_fs();
    
    // Test write from inode
    unsigned char file[500] ="0123456789------0123456789------0123456789------0123456789------0123456789------0123456789------0123456789------0123456789------0123456789------0123456789------0123456789------0123456789------0123456789------0123456789------0123456789------0123456789------====";
    int nnnnnn = set_inode_reg_file(file, 1,  sizeof(file));
    printf("Size=?%d, Allocated?=%d\n", (int)sizeof(file), nnnnnn);
    printf("%s\n", GET_FROM_LOCATION(1, 0));
    // Test read from inode
    int file_size = 500;
    unsigned char *file_read = (unsigned char *)malloc(file_size);
    int nnnnnm = get_inode_reg_file(1, file_size, file_read);
    printf("GET_FILE=\n%s\n", file_read);
    
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

