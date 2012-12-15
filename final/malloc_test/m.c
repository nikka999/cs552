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
    // DONT ALLOCATE PARTITION BLOCK FOR ROOT NOW. DO IT WHEN IT IS NEEDED
    INIT_FREEINODE;
    INIT_FREEBLOCK;
    // Mark bitmap for root
    SET_BITMAP_ALLOCATE_BLOCK(0);
    DECR_FREEBLOCK; // -1 for ROOT
    DECR_FREEINODE; // -1 for ROOT
    // EOF init
}


// ================================IMPLEMENT ME========================================================
int check_pathname (char *pathname, char* last, short* super_inode) {
    // Traverse pathname recursively find inode for subdir.
    int ptr = 0;
    // filename cannot exceed 13 char
    int char_count = -1;
    int cur = -1;
    int dir_inode = 0; // Set for root
    // Last occurance of '/'
    int last_dir = 0;
    while (cur != '\0') {
        if (char_count == 14) {
            // filename exceed 13 chars
            return -1;
        }
        cur = *(pathname + ptr);
        if (cur == '/') {
            last_dir = ptr;
            char_count = -1;
            
            printf("/ location=%d\n", ptr);
        }
        if (*(pathname + ptr + 1) == '\0') {
            // If next cell is NULL term, then copy the name over
            // current location is ptr;
            if (last_dir == 0) {
                // 1. Check for filename exist or not
                // 2. Get free ptr_entry
                
            }
        }
        ptr++;
        char_count++;
    }
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

int find_free_inode() {
    // Find a new free block, using first-fit.
    if (SHOW_FREEINODE == 0) {
        printf("Find Free inode: FREEINODE=0, returning -1 \n");
        return -1;
    } else {
        int j = 0;
        for (j; j < MAX_FILE_COUNT; j++) {
            int f = rd->ib[j].type[0];
            if (f == 0) {
                //printf("%d\n", j);
                return j;
            }
        }
    }
    printf("No free block found, ERROR\n");
    return -1;
}

int insert_inode(short super_inode, short new_inode, char *filename) {
    // Loop through super_inode LOC ptr.
    int i = 0; // Location blocks
    while(i < 10) {
        // If there is allocated block?
        if (GET_INODE_LOCATION_BLOCK(super_inode, i) == 0) {
            printf("i = %d, is empty\n", i);
            // Allocate partition blocks for super_inode
            int fb = find_free_block();
            // Assign free block to super_inode location i.
            SET_INODE_LOCATION_BLOCK(super_inode, i, fb);
            if (i >= 0 && i <= 7) {
                // 1~ 7 is direct block
                SET_DIR_ENTRY_NAME(fb, 0, filename);
                SET_DIR_ENTRY_INODE(fb, 0, new_inode);
                PRINT_DIR_ENTRY(fb, 0);
                return 1;
            } else if (i == 8) {
                // 8 is single redirection block
                // Treat previously allocated block as a ptr block
                // 1. Allocate another partition block for directory block
                int fb2 = find_free_block();
                // 2. Assign new block index[8].blocks[0];
                ASSIGN_LOCATION_SINGLE_RED(super_inode, 0, fb2);
                SET_DIR_ENTRY_NAME(fb2, 0, filename);
                SET_DIR_ENTRY_INODE(fb2, 0, new_inode);
            } else if (i == 9) {
                // 9 is double redirection block
                // Treat previously allocated block as a ptr block
                // 1. Allocate another partition block for 2nd ptr block
                int fb2 = find_free_block();
                // 2. Assign new block index[9].blocks[0];
                ASSIGN_LOCATION_DOUBLE_FST_RED(super_inode, 0, fb2);
                // 3. Allocate another partition block for directory block.
                int fb3 = find_free_block();
                ASSIGN_LOCATION_DOUBLE_SND_RED(super_inode, 0, 0, fb3);
                SET_DIR_ENTRY_NAME(fb3, 0, filename);
                SET_DIR_ENTRY_INODE(fb3, 0, new_inode);
            }
        } else {
            // Block already allocated. Loop though all blocks to find a free entry
            // First-fit here as well. 
            if (i >= 0 && i <= 7) {
                // 1~ 7 is direct block
                int j = 0;
                for (j; j < 16; j++) {
                    if (GET_INODE_FROM_INODE_LOCATION_INODE(super_inode, i, j) == 0) {
                        // We have a free location
                        printf("Free location. Inode = %d, Location index = %d, Dir_entry index = %d\n", super_inode, i, j);
                        // Insert inode into this location
                        SET_INODE_FROM_INODE_LOCATION_INODE(super_inode, i, j, new_inode);
                        SET_INODE_FROM_INODE_LOCATION_FILENAME(super_inode, i, j, filename);
                        PRINT_INODE_FROM_INODE_LOCATION(super_inode, i, j);
                        return 1;
                    }
                }
                // No free in 'i' direct block
            } else if (i == 8) {
                
                // 8 is single redirection block
            } else if (i == 9) {
                // 9 is double redirection block
            } 
        }
        i++;
    }
    // cannot find a free directory entry
    return -1;
}

int kcreat() {
    // kernel creat. Create a file
    int fi = find_free_inode();
    printf("Free inode = %d\n", fi);
    if (fi == -1) {
        return -1;
    }
    // Check pathname
    
    // Check if filename is already taken w/in the directory.
    
    
    return 1;
}

int check_for_filename_exist(int inode, char *filename) {
    int i=0;
    int j=0;
    for (i; i < 10; i++) {
        if (i == 8) {
            // Single redirection block
        } else if (i == 9) {
            // double redirection block
        } else {
            // Direct block pointer. 
            for (j; j < 16; j++) {
            }
        }
    }
}

int kmkdir(char *pathname) {
    // kernel mkdir. Create a DIR
    int fi = find_free_inode();
    printf("Free inode = %d\n", fi);
    if (fi == -1) {
        return -1;
    }
    printf("%s\n", pathname);
    
    // Check_pathname and get last entry.
    char *last = (char *)malloc(14);
    short super_inode;
#ifdef fin
    if (check_pathname(pathname, last, &super_inode) == -1) {
        // Pathname failed. 
        return -1;
    }
#endif
    last = "home";
    super_inode = 0;
    printf("%s, super=%d\n", last, super_inode);
    
    // Create directory
    // 1. Find a new free inode. DONE ABOVE
    // 2. Assign type as dir. size=0. DO NOT ALLOCATE NEW BLOCK.
    SET_INODE_TYPE_DIR(fi);
    SET_INODE_SIZE(fi, 0);
    // 3. Assign new inode to super inode.

    insert_inode(super_inode, fi, last);
   
}

int main() {
    init_fs();
    
    /**
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
    */
    
    // for testing
    int fb = find_free_block();
    SET_INODE_LOCATION_BLOCK(0, 0, fb);
    //
    
    char *pathname = "/home";
    kmkdir(pathname);
    
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

