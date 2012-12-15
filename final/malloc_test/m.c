#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "m.h"

/** MAXIMUM */
#define MAX_FILE_COUNT INODE_NUMBER
#define MAX_FILE_SIZE ((64*256)+(64*64*256)+256*8)

// For fd. Since the maximum file number is 1024. We assign it to be 1024.
// Put fd number to the fd_table.
struct fd *fd_table[1024];
struct Ramdisk *rd;

int find_free_block() {
    // Find a new free block, using first-fit. NOTE: before returning, we SET THE BITMAP = 1 !!!!!!
    if (SHOW_FREEBLOCK == 0) {
        printf("Find Free Block: FREEBLOCK=0, returning -1 \n");
        return -1;
    } else {
        int j = 0;
        for (j; j < PARTITION_NUMBER; j++) {
            if (SEE_BITMAP_FREE(j) == 1) {
                //printf("%d\n", j);
                // Set bitmap = 1
                SET_BITMAP_ALLOCATE_BLOCK(j);
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
                return j;
            }
        }
    }
    printf("No free block found, ERROR\n");
    return -1;
}
#define debug

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
#ifdef debug
                printf("**New inode inserted**\n ----To free location. super_inode = %d, Location index = %d, Dir_entry index = %d\n", super_inode, i, 0);
                PRINT_DIR_ENTRY(fb, 0);
#endif
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
#ifdef debug
                printf("**New inode inserted**\n ----To free location. super_inode = %d, Location index = %d, Ptr_entry = %d, Dir_entry index = %d\n", super_inode, i, 0, 0);
#endif
                return 1;
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
#ifdef debug
                printf("**New inode inserted**\n ----To free location. super_inode = %d, Location index = %d, Ptr_entry1 = %d, PTR_ENT2=%d, Dir_entry index = %d\n", super_inode, i, 0, 0, 0);
#endif
                return 1;
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
#ifdef debug
                        printf("**New inode inserted**\n ----To free location. super_inode = %d, Location index = %d, Dir_entry index = %d\n", super_inode, i, j);
#endif
                        // Insert inode into this location
                        SET_INODE_FROM_INODE_LOCATION_INODE(super_inode, i, j, new_inode);
                        SET_INODE_FROM_INODE_LOCATION_FILENAME(super_inode, i, j, filename);
                        PRINT_INODE_FROM_INODE_LOCATION(super_inode, i, j);
                        return 1;
                    }
#ifdef debug
                    printf("No free in #%d direct block\n", i);
#endif
                }
#ifdef debug
                printf("No free in ALL direct block\n");
#endif
            } else if (i == 8) {
                // 8 is single redirection block
                int j = 0;
                for (j; j < (256/4); j++) {
                    // Loop through the redirection block, j is PTR_ENTRY
                    if (GET_INODE_LOCATION_BLOCK_SIN(super_inode, j) == 0) {
                        // If it equals 0, then there are NO Dir block allocated.
                        // 1. Allocate a new Partition block for Dir block.
                        int fb = find_free_block();
                        // 2. Assign new block index[8].blocks[j];
                        ASSIGN_LOCATION_SINGLE_RED(super_inode, j, fb);
                        SET_DIR_ENTRY_NAME(fb, 0, filename);
                        SET_DIR_ENTRY_INODE(fb, 0, new_inode);
#ifdef debug
                        printf("**New inode inserted**\n ----To free location. super_inode = %d, Location index = %d, Ptr_entry = %d, Dir_entry index = %d\n", super_inode, i, j, 0);
                        PRINT_INODE_FROM_INODE_LOCATION_SIN(super_inode, j, 0);
#endif
                        return 1;
                    } else {
                        // A dir block is present, loop through to find free location
                        int k = 0;
                        for (k; k < 16; k++) {
                            // Loop through the Dir Block
                            if (GET_INODE_FROM_INODE_LOCATION_SIN_INODE(super_inode, j, k) == 0) {
#ifdef debug
                            printf("**New inode inserted**\n ----To free location. super_inode = %d, Location index = %d, Ptr_entry = %d, Dir_entry index = %d\n", super_inode, i, j, k);
#endif
                            SET_INODE_FROM_INODE_LOCATION_SIN_INODE(super_inode, j, k, new_inode);
                            SET_INODE_FROM_INODE_LOCATION_SIN_FILENAME(super_inode, j, k, filename);
                            PRINT_INODE_FROM_INODE_LOCATION_SIN(super_inode, j, k);
                            return 1;
                            }
                        }
                    }
#ifdef debug
                    printf("No free in PTR_ENT = %d, single redirection block\n", j);
#endif
                }
#ifdef debug
                printf("No free in ALL single redirection block\n");
#endif
            } else if (i == 9) {
                // 9 is double redirection block
                int j = 0;
                for (j; j < (256/4); j++) {
                    // Loop through the first redirection block, j is PTR_ENT1
                    if (GET_INODE_LOCATION_BLOCK_DOB_FST(super_inode, j) == 0) {
                        // There is no second redirection block.
                        // 1. Find free block and Assign Second redirection block.
                        int fb = find_free_block();
                        ASSIGN_LOCATION_DOUBLE_FST_RED(super_inode, j, fb);
                        // 2. Find free block and Assign to Sec for Directory block.
                        int fb2 = find_free_block();
                        ASSIGN_LOCATION_DOUBLE_SND_RED(super_inode, j, 0, fb2);
                        // 3. Insert new inode
                        SET_INODE_FROM_INODE_LOCATION_DOB_INODE(super_inode, j, 0, 0, new_inode);
                        SET_INODE_FROM_INODE_LOCATION_DOB_FILENAME(super_inode, j, 0, 0, filename);
#ifdef debug
                        printf("**New inode inserted**\n ----To free location. super_inode = %d, Location index = %d, Ptr_entry1 = %d, PTR_ENT2=%d, Dir_entry index = %d\n", super_inode, i, j, 0, 0);
                        PRINT_INODE_FROM_INODE_LOCATION_DOB(super_inode, j, 0, 0);
#endif
                        return 1;
                    } else {
                        // There is a second redirection block.
                        int k = 0;
                        for (k; k < (256/4); k++) {
                            // Loop through the second redirection block, k is PTR_ENT2
                            if (GET_INODE_LOCATION_BLOCK_DOB_SND(super_inode, j, k) == 0) {
                                // There is no Directory block
                                // 1. Find free block and Assign to second redirection block.
                                int fb = find_free_block();
                                ASSIGN_LOCATION_DOUBLE_SND_RED(super_inode, j, k, fb);
                                // 2. Insert new inode
                                SET_INODE_FROM_INODE_LOCATION_DOB_INODE(super_inode, j, k, 0, new_inode);
                                SET_INODE_FROM_INODE_LOCATION_DOB_FILENAME(super_inode, j, k, 0, filename);
#ifdef debug
                                printf("**New inode inserted**\n ----To free location. super_inode = %d, Location index = %d, Ptr_entry1 = %d, PTR_ENT2=%d, Dir_entry index = %d\n", super_inode, i, j, k, 0);
                                PRINT_INODE_FROM_INODE_LOCATION_DOB(super_inode, j, k, 0);
#endif
                                return 1;
                            } else {
                                // There is a Directory block
                                int l = 0;
                                for (l; l < 16; l++) {
                                    // l is ENT
                                    // Loop through Directory block to find a free entry
                                    if (GET_INODE_FROM_INODE_LOCATION_DOB_INODE(super_inode, j, k, l) == 0) {
                                        SET_INODE_FROM_INODE_LOCATION_DOB_INODE(super_inode, j, k, l, new_inode);
                                        SET_INODE_FROM_INODE_LOCATION_DOB_FILENAME(super_inode, j, k, l, filename);
#ifdef debug
                                        printf("**New inode inserted**\n ----To free location. super_inode = %d, Location index = %d, Ptr_entry1 = %d, PTR_ENT2=%d, Dir_entry index = %d\n", super_inode, i, j, k, l);
                                        PRINT_INODE_FROM_INODE_LOCATION_DOB(super_inode, j, k, l);
#endif
                                        return 1;
                                    }
                                }
                            }
#ifdef debug
                            printf("No free in PTR_ENT1=%d, PTR_ENT2=%d double redirection block\n", j, k);
#endif
                        }
                    }
#ifdef debug
                    printf("No free in PTR_ENT1=%d double redirection block\n", j);
#endif
                }
#ifdef debug
                printf("No free in ALL double redirection block\n");
#endif
            }
        }
        i++;
    }
    // cannot find a free directory entry
    return -1;
}

#undef debug

int kcreat(char *pathname) {
    // kernel creat. Create a file
    int fi = find_free_inode();
    printf("Free inode = %d\n", fi);
    if (fi == -1) {
        return -1;
    }
    // Check pathname and get last entry.
    char *last = (char *)malloc(14);
    short super_inode;
#ifdef fin
    if (check_pathname(pathname, last, &super_inode) == -1) {
        // Pathname failed. 
        return -1;
    }
#endif
    last = "test";
    super_inode = 1;
    printf("%s, super=%d\n", last, super_inode);
    
    // Create file
    // 1. Find a new free inode. DONE ABOVE
    // 2. Assign type as reg. size=0. DO NOT ALLOCATE NEW BLOCK.
    SET_INODE_TYPE_REG(fi);
    SET_INODE_SIZE(fi, 0);
    // 3. Assign new inode to super inode.
    if (insert_inode(super_inode, fi, last) != 1) {
        return -1;
    }
    return 0;
}

int kmkdir(char *pathname) {
    // kernel mkdir. Create a DIR
    int fi = find_free_inode();
    printf("Free inode = %d\n", fi);
    if (fi == -1) {
        return -1;
    }
#ifdef debug
    printf("%s\n", pathname);
#endif
    
    // Check_pathname and get last entry
    char *last = (char *)malloc(14);
    short super_inode;
#ifdef fin
    if (check_pathname(pathname, last, &super_inode) == -1) {
        // Pathname failed. 
        return -1;
    }
#endif
    memcpy(last, "home", 5);
    super_inode = 0;
    printf("%s, super=%d\n", last, super_inode);
    
#ifdef debug
    int i = 0;
    for (i; i<14; i++) {
        printf("%c ", last[i]);
    }
#endif
    // Create directory
    // 1. Find a new free inode. DONE ABOVE
    // 2. Assign type as dir. size=0. DO NOT ALLOCATE NEW BLOCK.
    SET_INODE_TYPE_DIR(fi);
    SET_INODE_SIZE(fi, 0);
    // 3. Assign new inode to super inode.
    if (insert_inode(super_inode, fi, last) != 1) {
        return -1;
    }
    return 0;
}

int build_inode_structure(short inode, unsigned char *ist) {
    /**
     * This function can be use for kread, kwrite, kreaddir
     * build the entire inode structure first, then go to offest. 
     */
    // Traverse through all possible location
    int i = 0; // Location blocks
    int position = 0; // Position can also be used as a counter for size.
    while(i < 10) {
        // If there is allocated block?
        if (GET_INODE_LOCATION_BLOCK(inode, i) == 0) {
            printf("i = %d, is empty\n", i);
            // No allocated block at i, we can return.
            return position;
        } else {
            // Block already allocated. Loop though all blocks and build inode structure
            if (i >= 0 && i <= 7) {
                // 1~ 7 is direct block
                // Since there is a block allocated, we can just copy the entire block.
                memcpy((ist + position), GET_INODE_LOCATION_BLOCK(inode, i), 256);
                position += 256;
            } else if (i == 8) {
                // 8 is single redirection block
                int j = 0;
                for (j; j < (256/4); j++) {
                    // Loop through the redirection block, j is PTR_ENTRY
                    if (GET_INODE_LOCATION_BLOCK_SIN(inode, j) == 0) {
                        // If it equals 0, then there are NO Dir block allocated.
                        return position;
                    } else {
                        // A dir block is allocated
                        memcpy((ist + position), GET_INODE_LOCATION_BLOCK_SIN(inode, j), 256);
                        position += 256;
                    }
                }
            } else if (i == 9) {
                // 9 is double redirection block
                int j = 0;
                for (j; j < (256/4); j++) {
                    // Loop through the first redirection block, j is PTR_ENT1
                    if (GET_INODE_LOCATION_BLOCK_DOB_FST(inode, j) == 0) {
                        // There is no second redirection block.
                        return position;
                    } else {
                        // There is a second redirection block.
                        int k = 0;
                        for (k; k < (256/4); k++) {
                            // Loop through the second redirection block, k is PTR_ENT2
                            if (GET_INODE_LOCATION_BLOCK_DOB_SND(inode, j, k) == 0) {
                                // There is no Directory block
                                return position;
                            } else {
                                // There is a Directory block
                                memcpy((ist + position), GET_INODE_LOCATION_BLOCK_DOB_SND(inode, j, k), 256);
                                position += 256;
                            }
                        }
                    }
                }
            }
        }
        i++;
    }
    return position;
}

int kopen(char *pathname) {
#ifdef debug
    printf("%s\n", pathname);
#endif
    // Check_pathname and get last entry
    char *last = (char *)malloc(14);
    short super_inode;
#ifdef fin
    if (check_pathname(pathname, last, &super_inode) == -1) {
        // Pathname failed.
        return -1;
    }
#endif
    // 1. Find inode number for file within super_inode
    int temp_inode = 0;

    int inode = temp_inode;
    // 2. Check if fd_table is active. 
    if (fd_table[inode] == NULL) {
        struct fd* newfd;
        newfd = (struct fd *)malloc(sizeof(struct fd));
        newfd->inode = &rd->ib[inode];
        fd_table[inode] = newfd;
    } else {
        // Other file is accessing it.
        return -1;
    }
#ifdef debug
    printf("%d, %d, %s\n", fd_table[inode]->read_pos, fd_table[inode]->write_pos, fd_table[inode]->inode->type);
#endif
    
    // We return the inode index of the file, which also is the fd_table index.
    // Since there is no special requirement on the traditional linux incrementing fd (i.e. first fd is 1, second is 2...etc), we just use what is easiest. 
    return inode;
}

int kclose(int fd) {
    // Again, fd=inode index
    if (fd_table[fd] == NULL) {
        // Not an open file
        return -1;
    } else {
        fd_table[fd] = NULL;
    }
    return 0;
}

int read_file(short inode, int read_pos, int num_bytes, unsigned char *temp) {
    // Build the inode structure first.
    unsigned char *ist = (unsigned char *)malloc(MAX_FILE_SIZE);
    int size = build_inode_structure(inode, ist);
#ifdef debug
    printf("size = %d\n", size);
#endif
    if (size == 0) {
        return 0;
    }
    if (read_pos >= (size - 1)) {
        // Read_pos is greater then possible size
        return -1;
    }
    if ((read_pos + num_bytes) > (size - 1)) {
        // Not enough bytes for us to read, read what is possible.
        memcpy(temp, ist + read_pos, (size - read_pos));
        return (size - read_pos);
    } else {
        // Enough byte for us to read.
        memcpy(temp, ist + read_pos, num_bytes);
        return num_bytes;
    }
    // Error if reach here.
    return -1;
}

int kread(int fd, char *address, int num_bytes) {
    // Again, fd=inode index
    if (fd_table[fd] == NULL) {
        // Not an valid fd
        return -1;
    }
    // Check if it is a regular file
    if (memcmp(reg, GET_INODE_TYPE(fd), 3) == 0) {
        // Read num_bytes TO ADDRESS location
        unsigned char *temp = (unsigned char *)malloc(sizeof(num_bytes));
        int ret = read_file(fd, fd_table[fd]->read_pos, num_bytes, temp);
        if (ret == -1) {
            return -1;
        }
        if (ret == 0) {
            // no byte read
            return 0;
        }
        memcpy(address, temp, ret);
        
        // COPY TO USER SPACE
        // return number of bytes actually read
        return ret;
    } else {
        // Not a regular file
        return -1;
    }
}

int kwrite(int fd, char *address, int num_bytes) {
    // Again, fd=inode index
    if (fd_table[fd] == NULL) {
        // Not an valid fd
        return -1;
    }
    // Check if it is a regular file
    if (memcmp(reg, GET_INODE_TYPE(fd), 3) == 0) {
        // write num_bytes From ADDRESS location
        
        
        // return number of bytes actually write
    } else {
        // Not a regular file
        return -1;
    }
}

int klseek(int fd, int offset) {
    // Again, fd=inode index
    if (fd_table[fd] == NULL) {
        // Not an valid fd
        return -1;
    }
    // Check if it is a regular file
    if (memcmp(reg, GET_INODE_TYPE(fd), 3) == 0) {
        // Assuming setting both read and write file position.
        fd_table[fd]->read_pos = offset;
        fd_table[fd]->write_pos = offset;
        // returning the new position
    } else {
        // Not a regular file
        return -1;
    }
}

int kunlink(char *pathname) {

}

int read_dir_entry(short inode, int read_pos, struct Dir_entry *temp_add) {
    // Read 1 dir_entry
    // Build the inode structure first.
    unsigned char *ist = (unsigned char *)malloc(MAX_FILE_SIZE);
    int size = build_inode_structure(inode, ist);
#ifdef debug
    printf("size = %d\n", size);
#endif
    
    if (size == 0) {
        return 0;
    }
    
    if (read_pos >= (size - 1)) {
        // Read_pos is greater then possible size
        return -1;
    }
    while ((read_pos + 16) <= size) {
        // Read an entry
        struct Dir_entry *d = (struct Dir_entry *)malloc(sizeof(struct Dir_entry));
        memcpy(d, ist + read_pos, 16);
        if (d->inode_number == 0) {
            // An empty Dir_entry
            read_pos += 16;
        } else {
            memcpy(temp_add, d, 16);
            // set the position to the next entry
            fd_table[inode]->read_pos = (read_pos + 16);
            return 1;
        }
    }
    // Nothing read, return EOF
    return 0;
}

int kreaddir(int fd, char *address) {
    // Again, fd=inode index
    if (fd_table[fd] == NULL) {
        // Not an valid fd
        return -1;
    }
    // Check if it is a DIR file
    if (memcmp(dir, GET_INODE_TYPE(fd), 3) == 0) {
        // read 1 dir_entry from fd
        struct Dir_entry *temp_add = (struct Dir_entry *)malloc(sizeof(struct Dir_entry));
        int ret = read_dir_entry(fd, fd_table[fd]->read_pos, temp_add);
        if (ret == -1) {
            return -1;
        }
        if (ret == 0) {
            // no dir entry
            return 0;
        }
#ifdef debug
        printf("\nret = %d\n", ret);
        printf("%d\n", temp_add->inode_number);
#endif
        memcpy(address, temp_add, 16);
        
        // Need copy_to_user to finish the copying.
        
        // 1 is success
        return 1;
    } else {
        // Not a Dir file. 
        return -1;
    }
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
    kmkdir(pathname);
    char *path2 = "/home/test";
    kcreat(path2);

    
    //unsigned char *ist = (unsigned char *)malloc(MAX_FILE_SIZE);
    //int size = build_inode_structure(0, ist);
    //int ij = 0;
    //printf("size = %d\n", size);
    //for (ij; ij < size; ij++) {
    //    if (ij % 256 == 0) {
    //        printf("\n one block end pos=%d\n", ij);
    //    }
    //    printf("%c ", ist[ij]);
    //}

    
    int i = kopen(path2);
    char *add= (char *)malloc(16);
    kreaddir(0, add);
    
    printf("read dir returned: \n");
    int ij = 0;
    for (ij; ij < 16; ij++) {
        printf("%d ", add[ij]);
    }
    printf("\n");
    
    //printf("%d\n", (struct Dir_entry *)add.inode_number);
    kclose(i);
    
#ifdef debug2
    PRINT_FREEINODE_COUNT;
    PRINT_FREEBLOCK_COUNT;

    // test set filename
    char n[16] = "non-root";
    SET_DIR_ENTRY_NAME(0, 0, n);
    //PRINT_DIR_ENTRY_NAME(0, 0);

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

