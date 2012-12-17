#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include "ramd.h"

MODULE_LICENSE("GPL");

/** MAXIMUM */
#define MAX_FILE_COUNT INODE_NUMBER
#define MAX_FILE_SIZE ((64*256)+(64*64*256)+256*8)

struct fd *fd_table[1024];
struct Ramdisk *rd;

char dir[4] = "dir";
char reg[4] = "reg";

// static unsigned char *ramdisk;
static struct proc_dir_entry *proc_entry;
static int ramdisk_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
static struct file_operations proc_operations;

struct Params {
	int fd;
	char* addr;
	int num_bytes;
};

int init_fs(void) {
    // INIT FILESYSTEM:
    rd = (struct Ramdisk *)vmalloc(sizeof(struct Ramdisk));
    printk("RAMDISK Size=%d\n", (int)sizeof(struct Ramdisk));
    // Setting up root
    SET_INODE_TYPE_DIR(0);
    //printf("type=%s\n", rd->ib[0].type);
    SET_INODE_SIZE(0, 0);
    //printf("size=%d\n", rd->ib[0].size);
    // DONT ALLOCATE PARTITION BLOCK FOR ROOT NOW. DO IT WHEN IT IS NEEDED
    INIT_FREEINODE;
    INIT_FREEBLOCK;
    DECR_FREEINODE; // -1 for ROOT
    // EOF init
	return 0;
}

int is_block_empty(union Block *blk) {
	struct Block_reg re = blk->reg;
	int i;
	for (i = 0; i < BLOCK_BYTES; i++) {
		if (re.byte[i] != 0)
			return 0;
	}
	return 1;
}

//searches a parent directory for a file's inode, return -1 if not found
int get_inode_index (int node, char *pathname) {
	int i,k,j,z;
	struct Inode inode;
	struct Block_dir bd;
	struct Block_ptr bp;
	union Block *blk, *dblk;
	inode = GET_INODE_BY_INDEX(node);
	if (!strcmp(inode.type, "reg"))
		return -1;
	for (i = 0; i < 8; i++) {
		if (inode.blocks[i] != 0)
			bd = inode.blocks[i]->dir;
		else
			continue;
		for (k = 0; k < 16; k++) {
			if(!strcmp(bd.ent[k].filename, pathname))
				return bd.ent[k].inode_number;
		}
	}
	for (i = 8; i < 10; i++) {
		if (inode.blocks[i] != 0)
			bp = inode.blocks[i]->ptr;
		else
			continue;
		for (k = 0; k < BLOCK_BYTES/4; k++) {
			blk = bp.blocks[k];
			if (blk != 0)
				bd = blk->dir;
			else
				continue;
			if (i == 8) {
				for (j = 0; j < 16; j++) {
					if(!strcmp(bd.ent[j].filename, pathname)) {
						return bd.ent[j].inode_number;
					}
				} 				
			}
			else {
				for (j = 0; j < BLOCK_BYTES/4; j++) {
					dblk = blk->ptr.blocks[j];
					if (dblk != 0)
						bd = dblk->dir;
					else
						continue;
					for (z = 0; z < 16; z++) {
						if(!strcmp(bd.ent[z].filename, pathname)) {
							return bd.ent[z].inode_number;
						}
					}
				}
			}
		}		
	}
	//fieldname not found
	return -1;
}

//checks if pathname exists, -1 if error, 0 if does not exists, >0 if does exists
int check_pathname (char *pathname, char* last, short* super_inode) {
    char name[14];
	char *slash;
	unsigned int size;
	// struct Inode inode;
	int node_index = 0;
	int current_index;
	// unsigned int dir = 0;
	memset(name, 0, 14);
    //validate that user inputed root
	if (pathname[0] != '/')
		return -1;
	size = strnlen(pathname, 100);
	// if size is 1, then the user only entered '/'
	if (size < 2)
		return -1;
	pathname++;
	while ((slash = strchr(pathname, '/')) != NULL) {
		size = slash-pathname;
		// check if someone typed two slash in row (i.e //)
		if (size < 1)
			return -1;
		// check if fieldname is greater than 14 chars
		if (size > 14)
			return -1;
		strncpy(name, pathname, size);
		name[size] = '\0';
		pathname = slash + 1;
		node_index = get_inode_index(node_index, name);
		if (node_index < 0)
			return -1;
	}
	//means the last character is a / (could use this to create new dir) or just error out
	if (*pathname == '\0')
		return -1;
		// dir = 1;
	//copy the specified name in final dir to name
	strncpy(name, pathname, 14);
	current_index = get_inode_index(node_index, name);
	//set last as the final file name
	strncpy(last, name, 14);
	*super_inode = node_index;
	//if returns something other than -1, it means that this pathname already exits
	if (current_index > 0)
		return current_index;
	return 0;
}

//recursive search
int recursive_inode_search(short *array, int *size, short cnode, short tnode) {
	int i,k,j,z; 
	struct Inode inode;
	struct Block_dir bd;
	struct Block_ptr bp;
	union Block *blk, *dblk;
	inode = GET_INODE_BY_INDEX(cnode);
	if (!strcmp(inode.type, "reg"))
		return -1;
	for (i = 0; i < 8; i++) {
		if (inode.blocks[i] != 0)
			bd = inode.blocks[i]->dir;
		else
			continue;
		for (k = 0; k < 16; k++) {
			if(bd.ent[k].inode_number == tnode)
				return 1;
			else {
				array[*size] = bd.ent[k].inode_number;
				*size+=1;
				if (recursive_inode_search(array, size, bd.ent[k].inode_number, tnode) == 1)
					return 1;
				*size-=1;
				array[*size] = 0;
			}
				
		}
	}
	for (i = 8; i < 10; i++) {
		if (inode.blocks[i] != 0)
			bp = inode.blocks[i]->ptr;
		else
			continue;
		for (k = 0; k < BLOCK_BYTES/4; k++) {
			blk = bp.blocks[k];
			if (blk != 0)
				bd = blk->dir;
			else
				continue;
			if (i == 8) {
				for (j = 0; j < 16; j++) {
					if(bd.ent[j].inode_number == tnode) {
						return 1;
					}
					else {
						array[*size] = bd.ent[j].inode_number;
						*size+=1;
						if (recursive_inode_search(array, size, bd.ent[j].inode_number, tnode) == 1)
							return 1;
						*size-=1;
						array[*size] = 0;
					}
				} 				
			}
			else {
				for (j = 0; j < BLOCK_BYTES/4; j++) {
					dblk = blk->ptr.blocks[j];
					if (dblk != 0)
						bd = dblk->dir;
					else
						continue;					
					for (z = 0; z < 16; z++) {
						if(bd.ent[z].inode_number == tnode) {
							return 1;
						}
						else {
							array[*size] = bd.ent[j].inode_number;
							*size+=1;
							if (recursive_inode_search(array, size, bd.ent[z].inode_number, tnode) == 1)
								return 1;
							*size-=1;
							array[*size] = 0;
						}
					}
				}
			}
		}		
	}
	//node does not exist here
	return 0;
}

//exhaustive search to find
int parent_inode_index_trace(short inode, short *trace) {
	int size = 1, found = 0;
	//allocate minimum space for array
	//root inode will always be 0
	short fotrace[256];
	fotrace[0] = 0;
	found = recursive_inode_search(fotrace, &size, 0, inode);
	if (!found)
		return -1; //fieldname not found
	memcpy(trace, fotrace, size * sizeof(short));
	return size;
}

int recursive_inode_size_add(short inode, int size) {
	short parent_array[256];
	int array_size, i, t;
	array_size = parent_inode_index_trace(inode, parent_array);
	if (array_size < 0)
		return -1;
	for (i = 0; i < array_size; i++) {
		t = GET_INODE_SIZE(parent_array[i]);
		SET_INODE_SIZE(parent_array[i], t+size);
	}
	SET_INODE_SIZE(inode, size);
	return 0;
}

int delete_dir_entry(short node, char *pathname) {
	int i,k,j,z;
	struct Inode *inode;
	struct Block_dir *bd;
	struct Block_ptr *bp;
	union Block *blk, *dblk;
	inode = &(GET_INODE_BY_INDEX(node));
	if (!strcmp(inode->type, "reg"))
		return -1;
	for (i = 0; i < 8; i++) {
		if (inode->blocks[i] != 0)
			bd = &(inode->blocks[i]->dir);
		else
			continue;
		for (k = 0; k < 16; k++) {
			if(!strcmp(bd->ent[k].filename, pathname)) {
				memset(&(bd->ent[k]), 0, sizeof(struct Dir_entry));
				if(is_block_empty(inode->blocks[i])) {
					SET_BITMAP_FREE_BLOCK(GET_BLOCK_INDEX_PARTITION(inode->blocks[i]));
					INCR_FREEBLOCK;
					inode->blocks[i] = NULL;					
				}
				return 0;
			}
				
		}
	}
	for (i = 8; i < 10; i++) {
		if (inode->blocks[i] != 0)
			bp = &(inode->blocks[i]->ptr);
		else
			continue;
		for (k = 0; k < BLOCK_BYTES/4; k++) {
			if ((blk = bp->blocks[k]) != 0)
				bd = &(blk->dir);
			else
				continue;
			if (i == 8) {
				for (j = 0; j < 16; j++) {
					if(!strcmp(bd->ent[j].filename, pathname)) {
						memset(&(bd->ent[j]), 0, sizeof(struct Dir_entry));
						if(is_block_empty(blk)) {
							SET_BITMAP_FREE_BLOCK(GET_BLOCK_INDEX_PARTITION(blk));
							INCR_FREEBLOCK;
							bp->blocks[k] = NULL;					
						}
						break;
					}	
				}
				if(is_block_empty(inode->blocks[i])) {
					SET_BITMAP_FREE_BLOCK(GET_BLOCK_INDEX_PARTITION(inode->blocks[i]));
					INCR_FREEBLOCK;
					inode->blocks[i] = NULL;					
				}
				return 0; 				
			}
			else {
				for (j = 0; j < BLOCK_BYTES/4; j++) {
					if((dblk = blk->ptr.blocks[j]) != 0)
						bd = &(dblk->dir);					
					else
						continue;
					for (z = 0; z < 16; z++) {
						if(!strcmp(bd->ent[z].filename, pathname)) {
							memset(&(bd->ent[z]), 0, sizeof(struct Dir_entry));
							if(is_block_empty(dblk)) {
								SET_BITMAP_FREE_BLOCK(GET_BLOCK_INDEX_PARTITION(dblk));
								INCR_FREEBLOCK;
								blk->ptr.blocks[j] = NULL;					
							}
							break;
						}
					}
					if(is_block_empty(blk)) {
						SET_BITMAP_FREE_BLOCK(GET_BLOCK_INDEX_PARTITION(blk));
						INCR_FREEBLOCK;
						bp->blocks[k] = NULL;					
					}
					if(is_block_empty(inode->blocks[i])) {
						SET_BITMAP_FREE_BLOCK(GET_BLOCK_INDEX_PARTITION(inode->blocks[i]));
						INCR_FREEBLOCK;
						inode->blocks[i] = NULL;					
					}
					return 0;
				}
			}
		}		
	}
	//fieldname not found
	return -1;
}

int recursive_pathname_size_decr(char *pathname, int b_size) {
	char name[14];
	char *slash;
	unsigned int size;
	int rc;
	// struct Inode inode;
	int node_index = 0;
	// unsigned int dir = 0;
	memset(name, 0, 14);
    //validate that user inputed root
	if (pathname[0] != '/')
		return -1;
	size = strnlen(pathname, 100);
	// if size is 1, then the user only entered '/'
	if (size < 2)
		return -1;
	pathname++;
	SET_INODE_SIZE(node_index, (GET_INODE_SIZE(node_index)-b_size));	
	while ((slash = strchr(pathname, '/')) != NULL) {
		size = slash-pathname;
		// check if someone typed two slash in row (i.e //)
		if (size < 1)
			return -1;
		// check if fieldname is greater than 14 chars
		if (size > 14)
			return -1;
		strncpy(name, pathname, size);
		name[size] = '\0';
		pathname = slash + 1;
		node_index = get_inode_index(node_index, name);
		if (node_index < 0)
			return -1;
		SET_INODE_SIZE(node_index, (GET_INODE_SIZE(node_index)-b_size));
	}
	//means the last character is a / (could use this to create new dir) or just error out
	if (*pathname == '\0')
		return -1;
		// dir = 1;
	//copy the specified name in final dir to name
	strncpy(name, pathname, 14);
	rc = delete_dir_entry(node_index, name);
	return rc;	
}

int find_free_block(void) {
    // Find a new free block, using first-fit. NOTE: before returning, we SET THE BITMAP = 1 !!!!!!
    if (SHOW_FREEBLOCK == 0) {
        return -1;
    } else {
        int j = 0;
        for (j = 0; j < PARTITION_NUMBER; j++) {
            if (SEE_BITMAP_FREE(j) == 1) {
                // Set bitmap = 1
                SET_BITMAP_ALLOCATE_BLOCK(j);
                DECR_FREEBLOCK;
                return j;
            }
        }
    }
    return -1;
}

int find_free_inode(void) {
    // Find a new free block, using first-fit.
    if (SHOW_FREEINODE == 0) {
        return -1;
    } else {
        int j = 0;
        for (j = 0; j < MAX_FILE_COUNT; j++) {
            int f = rd->ib[j].type[0];
            if (f == 0) {
                DECR_FREEINODE;
                return j;
            }
        }
    }
    return -1;
}

int insert_inode(short super_inode, short new_inode, char *filename) {
    // Loop through super_inode LOC ptr.
    int i = 0; // Location blocks
	int fb, fb2, fb3;
    while(i < 10) {
        // If there is allocated block?
        if (GET_INODE_LOCATION_BLOCK(super_inode, i) == 0) {
            //printf("i = %d, is empty\n", i);
            // Allocate partition blocks for super_inode
            fb = find_free_block();
            // Assign free block to super_inode location i.
            SET_INODE_LOCATION_BLOCK(super_inode, i, fb);
            if (i >= 0 && i <= 7) {
                // 1~ 7 is direct block
                SET_DIR_ENTRY_NAME(fb, 0, filename);
                SET_DIR_ENTRY_INODE(fb, 0, new_inode);
                return 1;
            } else if (i == 8) {
                // 8 is single redirection block
                // Treat previously allocated block as a ptr block
                // 1. Allocate another partition block for directory block
                fb2 = find_free_block();
                // 2. Assign new block index[8].blocks[0];
                ASSIGN_LOCATION_SINGLE_RED(super_inode, 0, fb2);
                SET_DIR_ENTRY_NAME(fb2, 0, filename);
                SET_DIR_ENTRY_INODE(fb2, 0, new_inode);
                return 1;
            } else if (i == 9) {
                // 9 is double redirection block
                // Treat previously allocated block as a ptr block
                // 1. Allocate another partition block for 2nd ptr block
                fb2 = find_free_block();
                // 2. Assign new block index[9].blocks[0];
                ASSIGN_LOCATION_DOUBLE_FST_RED(super_inode, 0, fb2);
                // 3. Allocate another partition block for directory block.
                fb3 = find_free_block();
                ASSIGN_LOCATION_DOUBLE_SND_RED(super_inode, 0, 0, fb3);
                SET_DIR_ENTRY_NAME(fb3, 0, filename);
                SET_DIR_ENTRY_INODE(fb3, 0, new_inode);
                return 1;
            }
        } else {
            // Block already allocated. Loop though all blocks to find a free entry
            // First-fit here as well. 
            if (i >= 0 && i <= 7) {
                // 1~ 7 is direct block
                int j = 0;
                for (j = 0; j < 16; j++) {
                    if (GET_INODE_FROM_INODE_LOCATION_INODE(super_inode, i, j) == 0) {
                        // We have a free location
                        // Insert inode into this location
                        SET_INODE_FROM_INODE_LOCATION_INODE(super_inode, i, j, new_inode);
                        SET_INODE_FROM_INODE_LOCATION_FILENAME(super_inode, i, j, filename);
                        // PRINT_INODE_FROM_INODE_LOCATION(super_inode, i, j);
                        return 1;
                    }
                }
            } else if (i == 8) {
                // 8 is single redirection block
                int j = 0;
                for (j = 0; j < (256/4); j++) {
                    // Loop through the redirection block, j is PTR_ENTRY
                    if (GET_INODE_LOCATION_BLOCK_SIN(super_inode, j) == 0) {
                        // If it equals 0, then there are NO Dir block allocated.
                        // 1. Allocate a new Partition block for Dir block.
                        fb = find_free_block();
                        // 2. Assign new block index[8].blocks[j];
                        ASSIGN_LOCATION_SINGLE_RED(super_inode, j, fb);
                        SET_DIR_ENTRY_NAME(fb, 0, filename);
                        SET_DIR_ENTRY_INODE(fb, 0, new_inode);
                        return 1;
                    } else {
                        // A dir block is present, loop through to find free location
                        int k = 0;
                        for (k = 0; k < 16; k++) {
                            // Loop through the Dir Block
                            if (GET_INODE_FROM_INODE_LOCATION_SIN_INODE(super_inode, j, k) == 0) {
                            SET_INODE_FROM_INODE_LOCATION_SIN_INODE(super_inode, j, k, new_inode);
                            SET_INODE_FROM_INODE_LOCATION_SIN_FILENAME(super_inode, j, k, filename);
                            // PRINT_INODE_FROM_INODE_LOCATION_SIN(super_inode, j, k);
                            return 1;
                            }
                        }
                    }
                }
            } else if (i == 9) {
                // 9 is double redirection block
                int j = 0;
                for (j = 0; j < (256/4); j++) {
                    // Loop through the first redirection block, j is PTR_ENT1
                    if (GET_INODE_LOCATION_BLOCK_DOB_FST(super_inode, j) == 0) {
                        // There is no second redirection block.
                        // 1. Find free block and Assign Second redirection block.
                        fb = find_free_block();
                        ASSIGN_LOCATION_DOUBLE_FST_RED(super_inode, j, fb);
                        // 2. Find free block and Assign to Sec for Directory block.
                        fb2 = find_free_block();
                        ASSIGN_LOCATION_DOUBLE_SND_RED(super_inode, j, 0, fb2);
                        // 3. Insert new inode
                        SET_INODE_FROM_INODE_LOCATION_DOB_INODE(super_inode, j, 0, 0, new_inode);
                        SET_INODE_FROM_INODE_LOCATION_DOB_FILENAME(super_inode, j, 0, 0, filename);
                        return 1;
                    } else {
                        // There is a second redirection block.
                        int k = 0;
                        for (k = 0; k < (256/4); k++) {
                            // Loop through the second redirection block, k is PTR_ENT2
                            if (GET_INODE_LOCATION_BLOCK_DOB_SND(super_inode, j, k) == 0) {
                                // There is no Directory block
                                // 1. Find free block and Assign to second redirection block.
                                fb = find_free_block();
                                ASSIGN_LOCATION_DOUBLE_SND_RED(super_inode, j, k, fb);
                                // 2. Insert new inode
                                SET_INODE_FROM_INODE_LOCATION_DOB_INODE(super_inode, j, k, 0, new_inode);
                                SET_INODE_FROM_INODE_LOCATION_DOB_FILENAME(super_inode, j, k, 0, filename);
                                return 1;
                            } else {
                                // There is a Directory block
                                int l = 0;
                                for (l = 0; l < 16; l++) {
                                    // l is ENT
                                    // Loop through Directory block to find a free entry
                                    if (GET_INODE_FROM_INODE_LOCATION_DOB_INODE(super_inode, j, k, l) == 0) {
                                        SET_INODE_FROM_INODE_LOCATION_DOB_INODE(super_inode, j, k, l, new_inode);
                                        SET_INODE_FROM_INODE_LOCATION_DOB_FILENAME(super_inode, j, k, l, filename);
                                        return 1;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        i++;
    }
    // cannot find a free directory entry
    return -1;
}

int kcreat(char *pathname) {
   	// PRINT_FREEINODE_COUNT;
    // PRINT_FREEBLOCK_COUNT;
    // kernel creat. Create a file
	char *last;
	short super_inode;    
    int fi = find_free_inode();
    if (fi < 0) {
        return -1;
    }
    // Check pathname and get last entry.
	last = (char *)kmalloc(14, GFP_KERNEL);
    if (check_pathname(pathname, last, &super_inode) == -1) {
        // Pathname failed. 
        return -1;
    }    
    // Create file
    // 1. Find a new free inode. DONE ABOVE
    // 2. Assign type as reg. size=0. DO NOT ALLOCATE NEW BLOCK.
    SET_INODE_TYPE_REG(fi);
    SET_INODE_SIZE(fi, 0);
    // 3. Assign new inode to super inode.
    if (insert_inode(super_inode, fi, last) != 1) {
		kfree(last);
        return -1;
    }
	kfree(last);
    return 0;
}

int kmkdir(char *pathname) {
	char *last;
	short super_inode;    
    // kernel mkdir. Create a DIR
    int fi = find_free_inode();
    if (fi == -1) {
        return -1;
    }

    
    // Check_pathname and get last entry
    last = (char *)kmalloc(14, GFP_KERNEL);

    if (check_pathname(pathname, last, &super_inode) != 0) {
        // Pathname failed.
		kfree(last);
        return -1;
    }    
    // Create directory
    // 1. Find a new free inode. DONE ABOVE
    // 2. Assign type as dir. size=0. DO NOT ALLOCATE NEW BLOCK.
    SET_INODE_TYPE_DIR(fi);
    SET_INODE_SIZE(fi, 0);
    // 3. Assign new inode to super inode.
    if (insert_inode(super_inode, fi, last) != 1) {
		kfree(last);
		return -1;
    }
	kfree(last);
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
            //printf("i = %d, is empty\n", i);
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
                for (j = 0; j < (256/4); j++) {
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
                for (j = 0; j < (256/4); j++) {
                    // Loop through the first redirection block, j is PTR_ENT1
                    if (GET_INODE_LOCATION_BLOCK_DOB_FST(inode, j) == 0) {
                        // There is no second redirection block.
                        return position;
                    } else {
                        // There is a second redirection block.
                        int k = 0;
                        for (k = 0; k < (256/4); k++) {
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
    // Check_pathname and get last entry
    char *last = (char *)kmalloc(14, GFP_KERNEL);
    short super_inode;
	int inode;
    if ((inode = check_pathname(pathname, last, &super_inode)) < 1) {
        // Pathname failed.
        return -1;
    }
    // 1. Find inode number for file within super_inode
    // int temp_inode = 0;

    // int inode = temp_inode;
    // 2. Check if fd_table is active. 
    if (fd_table[inode] == NULL) {
        struct fd* newfd;
        newfd = (struct fd *)kmalloc(sizeof(struct fd), GFP_KERNEL);
        newfd->inode = &rd->ib[inode];
        fd_table[inode] = newfd;
		kfree(newfd);
    } else {
        // Other file is accessing it.
        return -1;
    }
	kfree(last);
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
		kfree(fd_table[fd]);
		fd_table[fd] = NULL;
    }
    return 0;
}

int read_file(short inode, int read_pos, int num_bytes, unsigned char *temp) {
    // Build the inode structure first.
    unsigned char *ist = (unsigned char *)kmalloc(MAX_FILE_SIZE, GFP_KERNEL);
    int size = build_inode_structure(inode, ist);
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
        // Increment fd position
        fd_table[inode]->read_pos = size;
		kfree(ist);
        return (size - read_pos);
    } else {
        // Enough byte for us to read.
        memcpy(temp, ist + read_pos, num_bytes);
        // Increment fd position
        fd_table[inode]->read_pos = read_pos + num_bytes;
		kfree(ist);
        return num_bytes;
    }
    // Error if reach here.
	kfree(ist);
    return -1;
}

int kread(int fd, char *address, int num_bytes) {
    // Again, fd=inode index
	int ret, ret2, pos, second_redir;
	unsigned char *temp;
    if (fd_table[fd] == NULL) {
        // Not an valid fd
        return -1;
    }
    // Check if it is a regular file
    if (memcmp(reg, GET_INODE_TYPE(fd), 3) == 0) {
        // Read num_bytes TO ADDRESS location
        second_redir = 8*256;
        temp = (unsigned char *)kmalloc(second_redir, GFP_KERNEL);
        pos = 0;
        ret = 0;
        while (pos < num_bytes) {
            int bytes = 0;
            if ((num_bytes - pos) <= second_redir) {
                bytes = num_bytes - pos;
            } else {
                bytes = second_redir;
            }
            
            ret2 = read_file(fd, fd_table[fd]->read_pos, bytes, temp);
            //printf("pos = %d, bytes=%d, num_bytes = %d, ret2= %d\n", pos, bytes, num_bytes, ret2);
            if (ret2 == -1) {
                return -1;
            }
            if (ret2 == 0) {
                // no byte read
                return 0;
            }
            //printf("pos = %d, bytes=%d, num_bytes = %d, ret2= %d\n", pos, bytes, num_bytes, ret2);
            memcpy(address + pos, temp, bytes);
            //memset(temp, 0, bytes);
            pos += bytes;
            ret += ret2;
            //printf("pos = %d, bytes=%d, num_bytes = %d, ret2= %d\n", pos, bytes, num_bytes, ret2);
        }
        kfree(temp);
        // My test end

        // COPY TO USER SPACE
        // return number of bytes actually read
        return ret;
    } else {
        // Not a regular file
        return -1;
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

int write_to_fs(short inode, unsigned char *ist, int new_size) {
	int fb, fb2, fb3;
	int i, j, k, size, position = 0;
	for (i = 0; i < 10; i++) {
		//check to see if current block is allocated already
		if (GET_INODE_LOCATION_BLOCK(inode, i) == 0) {        
			//current block is not allocated, find free block and assign it to current index
			fb = find_free_block();
			ASSIGN_LOCATION(inode, i, fb);
		}
		if (i < 8) {
			//if it is a direct block, then can comense write
			if ((size = new_size - position) > 256)
				size = 256;
			WRITE_TO_LOCATION(inode, i, (ist + position), size);
            position += size;				
			//check to see whether or not this is last block
			if ((new_size - position) < 256)
				return 1;
		}
		else {
			for (j = 0; j < BLOCK_BYTES/4; j++) {
				//check to see if first indirection is allocated
				if (GET_INODE_LOCATION_BLOCK_GENERIC_FST(inode, i, j) == 0) {
					//if not, find free block and assign
					fb2 = find_free_block();
					ASSIGN_LOCATION_GENERIC_RED(inode, i, j, fb2);
				}
				if (i == 8) {
					if ((size = new_size - position) > 256)
						size = 256;
					//begin write to 1st indirection block
					WRITE_TO_LOCATION_SINGLE_RED(inode, j, (ist + position), size);
                    position += size;
					//check to see whether or not this is last block
					if ((new_size - position) < 256)
						return 1;
				}
				else if(i == 9) {
					for (k = 0; k < BLOCK_BYTES/4; k++) {
						if (GET_INODE_LOCATION_BLOCK_DOB_SND(inode, j, k) == 0) {
							fb3 = find_free_block();
							ASSIGN_LOCATION_DOUBLE_SND_RED(inode, j, k, fb3);
						}
						if ((size = new_size - position) > 256)
							size = 256;
                        WRITE_TO_LOCATION_DOUBLE_RED(inode, j, k, (ist + position), size);
                        position += size;
						//check to see whether or not this is last block
						if ((new_size - position) < 256)
							return 1;				
					}					
				}
			}
		}
	}
	//unable to fill all the data
	return -1;
}
int write_file(short inode, int write_pos, int num_bytes, unsigned char *temp) {
    // Build the inode structure first.
    unsigned char *ist = (unsigned char *)kmalloc(MAX_FILE_SIZE, GFP_KERNEL);
    int size = build_inode_structure(inode, ist);
    if ((write_pos + num_bytes) > MAX_FILE_SIZE) {
        // Not enough bytes for us to write, write what is possible.
        memcpy(ist + write_pos, temp, (MAX_FILE_SIZE - write_pos));
        // Actually write back to filesystem
        write_to_fs(inode, ist, MAX_FILE_SIZE);
        // Set Inode new size
		recursive_inode_size_add(inode, MAX_FILE_SIZE);		
        // SET_INODE_SIZE(inode, MAX_FILE_SIZE);
        // Increment fd position
        fd_table[inode]->write_pos = MAX_FILE_SIZE;
		kfree(ist);
        return (MAX_FILE_SIZE - write_pos);
    } else {
        // Enough byte for us to read.
        memcpy(ist + write_pos, temp, num_bytes);
        // Actually write back to filesystem
        write_to_fs(inode, ist, write_pos + num_bytes);
        // Set new size for inode and recursively for all parent dirs		
		recursive_inode_size_add(inode, write_pos + num_bytes);
        // SET_INODE_SIZE(inode, write_pos + num_bytes);
        // Increment fd position
        fd_table[inode]->write_pos = write_pos + num_bytes;
		kfree(ist);
        return num_bytes;
    }
    // Error if reach here.
    return -1;
}

int kwrite(int fd, char *address, int num_bytes) {
	int ret, ret2, pos, second_redir;
	unsigned char *temp;
    // Again, fd=inode index
    if (fd_table[fd] == NULL) {
        // Not an valid fd
        return -1;
    }
    // Check if it is a regular file
    if (memcmp(reg, GET_INODE_TYPE(fd), 3) == 0) {
        // write num_bytes From ADDRESS location
        // COPY FROM USERSPACE
        second_redir = 256*8;
        temp = (unsigned char *)kmalloc(second_redir, GFP_KERNEL);
        pos = 0;
        ret = 0;
        while (pos < num_bytes) {
            int bytes = 0;
            if ((num_bytes - pos) <= second_redir) {
                bytes = num_bytes - pos;
            } else {
                bytes = second_redir;
            }
            
            memcpy(temp, address + pos, bytes);
            ret2 = write_file(fd, fd_table[fd]->write_pos, bytes, temp);
            pos += bytes;
            ret += ret2;
            memset(temp, 0, bytes);
            if (ret2 == -1) {
                return -1;
            }
        }
        kfree(temp);
        
        /// END OF TESTING
        
        if (ret == -1) {
            return -1;
        }
        if (ret == 0) {
            // no byte write
            return 0;
        }
        
        // return number of bytes actually write
        return ret;
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
        //printf("offset=%d\n", offset);
        fd_table[fd]->read_pos = offset;
        fd_table[fd]->write_pos = offset;
        // returning the new position
        return fd_table[fd]->read_pos = offset;
    } else {
        // Not a regular file
        return -1;
    }
}

int unlink_file(int inode) {
    int i = 0;
	int k,j;
    while(i < 10) {
        if (GET_INODE_LOCATION_BLOCK(inode, i) == 0) {
            // No allocated block at i. Nothing to free
            return 1;
        } else {
            // Allocated block at i.
            if (i >= 0 && i <= 7) {
                // Remove File Block
				memset(GET_INODE_LOCATION_BLOCK(inode, i)->reg.byte, 0, 256);
				SET_BITMAP_FREE_BLOCK(GET_BLOCK_INDEX_PARTITION(GET_INODE_LOCATION_BLOCK(inode, i)));
				INCR_FREEBLOCK;
                
            } else if (i == 8) {
				for (k = 0; k < BLOCK_BYTES/4; k++) {
					if(GET_INODE_LOCATION_BLOCK_SIN(inode, k) == 0)
						break;
					memset(GET_INODE_LOCATION_BLOCK_SIN(inode, k)->reg.byte, 0, 256);
					SET_BITMAP_FREE_BLOCK(GET_BLOCK_INDEX_PARTITION(GET_INODE_LOCATION_BLOCK_SIN(inode, k)));
					INCR_FREEBLOCK;
                    
				}
				memset(GET_INODE_LOCATION_BLOCK(inode, i)->reg.byte, 0, 256);
				SET_BITMAP_FREE_BLOCK(GET_BLOCK_INDEX_PARTITION(GET_INODE_LOCATION_BLOCK(inode, i)));
				INCR_FREEBLOCK;
				// Remove All File Blocks
                // Then Remove Single redirection block
            } else if (i == 9) {
				for (k = 0; k < BLOCK_BYTES/4; k++) {
					if(GET_INODE_LOCATION_BLOCK_DOB_FST(inode, k) == 0)
						break;
					for (j = 0; j < BLOCK_BYTES/4; j++) {
						if(GET_INODE_LOCATION_BLOCK_DOB_SND(inode, k, j) == 0)
							break;
						memset(GET_INODE_LOCATION_BLOCK_DOB_SND(inode, k, j)->reg.byte, 0, 256);
						SET_BITMAP_FREE_BLOCK(GET_BLOCK_INDEX_PARTITION(GET_INODE_LOCATION_BLOCK_DOB_SND(inode, k, j)));
						INCR_FREEBLOCK;
                    }
                    
					memset(GET_INODE_LOCATION_BLOCK_DOB_FST(inode, k)->reg.byte, 0, 256);
					SET_BITMAP_FREE_BLOCK(GET_BLOCK_INDEX_PARTITION(GET_INODE_LOCATION_BLOCK_DOB_FST(inode, k)));
					INCR_FREEBLOCK;
                    
				}
				memset(GET_INODE_LOCATION_BLOCK(inode, i)->reg.byte, 0, 256);
				SET_BITMAP_FREE_BLOCK(GET_BLOCK_INDEX_PARTITION(GET_INODE_LOCATION_BLOCK(inode, i)));
				INCR_FREEBLOCK;
                // Remove All File Blocks
                // Then Remove All Second Double redirection blocks
                // Then Remove First double redirection block
            }
        }
        i++;
    }
    return 1;
}

int kunlink(char *pathname) {
	int retp, inode, reg_size;
	short super_inode;
	char *last;
    if (pathname[0] == '/' && pathname[1] == '\0') {
        // trying to unlink root
        return -1;
    }
    last = (char *)kmalloc(14, GFP_KERNEL);
    retp = check_pathname(pathname, last, &super_inode);
    if (retp == 0 || retp == -1) {
        // does not exist file or error
        return -1;
    }
    if (retp > 0) {
		if (fd_table[retp] != NULL) {
	        // fd is already open
			kfree(last);
	        return -1;
	    }
        // File exist, we can strart to remove
        // Get inode number
        inode = retp;
        if (memcmp(dir, GET_INODE_TYPE(inode), 3) == 0) {
            // Check if it is a DIR file
            if (GET_INODE_SIZE(inode) != 0) {
                // removing non-empty directory.
				kfree(last);
                return -1; 
            } else {
                // file size = 0
                // 1. Remove dir
                memset(&(rd->ib[inode]), 0, sizeof(struct Inode));			
				delete_dir_entry(super_inode, last);
				INCR_FREEINODE;
                // 2. Go to super_inode and remove inode entry
				kfree(last);
                return 0;
                
            }
        }
        if (memcmp(reg, GET_INODE_TYPE(inode), 3) == 0) {
            // Check if it is a reg file
            // 1. Get file size
			reg_size = GET_INODE_SIZE(inode);
            // 2. remove file
			unlink_file(inode);
			memset(&(rd->ib[inode]), 0, sizeof(struct Inode));			
            // 3. Go to super_inode and remove inode entry
			recursive_pathname_size_decr(pathname, reg_size);
			INCR_FREEINODE;
            // 4. Traverse filesystem and minus file_size on all super inodes.
            // PRINT_FREEINODE_COUNT;
            // PRINT_FREEBLOCK_COUNT;
			kfree(last);
            return 0;
        }
    }
	return -1;
}

int read_dir_entry(short inode, int read_pos, struct Dir_entry *temp_add) {
    // Read 1 dir_entry
    // Build the inode structure first.
    unsigned char *ist = (unsigned char *)kmalloc(MAX_FILE_SIZE, GFP_KERNEL);
    int size = build_inode_structure(inode, ist);    
    if (size == 0) {
        return 0;
    }
    
    if (read_pos >= (size - 1)) {
        // Read_pos is greater then possible size
        return -1;
    }
    while ((read_pos + 16) <= size) {
        // Read an entry
        struct Dir_entry *d = (struct Dir_entry *)kmalloc(sizeof(struct Dir_entry), GFP_KERNEL);
        memcpy(d, ist + read_pos, 16);
        if (d->inode_number == 0) {
            // An empty Dir_entry
            read_pos += 16;
        } else {
            memcpy(temp_add, d, 16);
            // set the position to the next entry
            fd_table[inode]->read_pos = (read_pos + 16);
			kfree(ist);
			kfree(d);
            return 1;
        }
    }
    // Nothing read, return EOF
	kfree(ist);
    return 0;
}

int small_itoa(struct Dir_entry *temp_add) {
    // convert inode number into string.
    short num = temp_add->inode_number;
    //printf("num = %d", num);
    int ten = num/10;
    int dig = num%10;
    // 48 = ascii '0'
    *((unsigned char *)temp_add + 14) = (48 + ten);
    *((unsigned char *)temp_add + 15) = (48 + dig);
	return 0;
}

#define USING_ATOI
// using ATOI only to accomdate test script 4
int kreaddir(int fd, char *address) {
    // Again, fd=inode index
    if (fd_table[fd] == NULL) {
        // Not an valid fd
        return -1;
    }
    // Check if it is a DIR file
    if (memcmp(dir, GET_INODE_TYPE(fd), 3) == 0) {
        // read 1 dir_entry from fd
        struct Dir_entry *temp_add = (struct Dir_entry *)kmalloc(sizeof(struct Dir_entry), GFP_KERNEL);
        int ret = read_dir_entry(fd, fd_table[fd]->read_pos, temp_add);
        if (ret == -1) {
            return -1;
        }
        if (ret == 0) {
            // no dir entry
			kfree(temp_add);
            return 0;
        }
#ifdef USING_ATOI
        small_itoa(temp_add);
#endif
// #ifdef debug
//         printf("\nret = %d\n", ret);
//         printf("%d\n", temp_add->inode_number);
//         int zz = 0;
//         for (zz; zz < 16; zz++) {
//             printf("%d ", *((unsigned char *)temp_add + zz));
//         }
//         printf("\n");
// #endif
        memcpy(address, temp_add, 16);
        
        // Need copy_to_user to finish the copying.
        
        // 1 is success
		kfree(temp_add);
        return 1;
    } else {
        // Not a Dir file. 
        return -1;
    }
}

static int __init init_routine(void) {
	printk("<1> Loading RAMDISK Module\n");

	proc_operations.ioctl = ramdisk_ioctl;

	// Create /proc entry first. 666 for Read/Write
	proc_entry = create_proc_entry("ramdisk", 0666, NULL);
	if (!proc_entry) {
		printk("<1> Error creating /proc entry.\n");
		return 1;
	}
	// Working version
	proc_entry->proc_fops = &proc_operations;
	init_fs();
// Trying read write
//	proc_entry->read_proc = read_proc;
//	proc_entry->write_proc = write_proc;

	// vmalloc for 2MB
	// ramdisk = (char *)vmalloc(2097150);

	return 0;
}

static void __exit exit_routine(void) {
	printk("<1> Exiting RAMDISK Module\n");
	// Free ramdisk
	vfree(rd);

	// Remove /proc entry
	remove_proc_entry("ramdisk", NULL);
	return;
}

/** 
* Ramdisk entry point
*/

static int ramdisk_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg) {
	int fd, rc;
	// int size;
	unsigned long size;
	char *pathname;
	char *addr;
	struct Params p;
	/* 
	 * Switch according to the ioctl called 
	 */
	switch (cmd) {
//		case RD_MALLOC:
			// vmalloc for 2MB
		//	ramdisk = (unsigned char *)vmalloc(2097150);
//			printk("<1>I finished vmalloc!\n");
//			break;
		case RD_CREAT:
			size = strnlen_user((char *)arg, 50);
			pathname = (char *)kmalloc(size,GFP_KERNEL);
			copy_from_user(pathname, (char *)arg, size);
			rc = kcreat(pathname);
			printk("<1> kernel got: %s\n",pathname);
			printk("<1> the len is %lu\n", size);
			kfree(pathname);
			return rc;
			break;
		case RD_MKDIR:
			size = strnlen_user((char *)arg, 50);
			pathname = (char *)kmalloc(size,GFP_KERNEL);
			copy_from_user(pathname, (char *)arg, size);
			rc = kmkdir(pathname);
			printk("<1> kernel got: %s\n",pathname);
			printk("<1> the len is %lu\n", size);
			kfree(pathname);
			return rc;
			break;
		case RD_OPEN:
			size = strnlen_user((char *)arg, 50);
			pathname = (char *)kmalloc(size,GFP_KERNEL);
			copy_from_user(pathname, (char *)arg, size);
			rc = kopen(pathname);
			printk("<1> kernel got: %s\n",pathname);
			printk("<1> the len is %lu\n", size);
			kfree(pathname);
			return rc;
			break;
		case RD_CLOSE:
			get_user(fd, (int *)arg);
			rc = kclose(fd);
			printk("<1> kernel: the fd is %d\n", fd);
			return rc;
			break;
		case RD_READ:
			copy_from_user(&p, (struct Params *)arg, sizeof(struct Params));
			printk("<1> got p.fd:%d, p.addr: %p, p.byte_size:%d\n", p.fd, p.addr, p.num_bytes);
			addr = (char *)kmalloc(p.num_bytes, GFP_KERNEL);
			rc = kread(p.fd, addr, p.num_bytes);
			copy_to_user(p.addr, addr, p.num_bytes);
			kfree(addr);
			return rc;
			break;
		case RD_WRITE:
			copy_from_user(&p, (struct Params *)arg, sizeof(struct Params));
			printk("<1> got p.fd:%d, p.addr: %p, p.byte_size:%d\n", p.fd, p.addr, p.num_bytes);
			addr = (char *)kmalloc(p.num_bytes, GFP_KERNEL);			
			rc = kwrite(p.fd, addr, p.num_bytes);
			copy_to_user(p.addr, addr, p.num_bytes);
			kfree(addr);
			return rc;
			break;
		case RD_LSEEK:
			copy_from_user(&p, (struct Params *)arg, sizeof(struct Params));
			printk("<1> got p.fd:%d, p.byte_size:%d\n", p.fd, p.num_bytes);
			rc = klseek(p.fd, p.num_bytes);
			return rc;
			break;
		case RD_UNLINK:
			size = strnlen_user((char *)arg, 50);
			pathname = (char *)kmalloc(size,GFP_KERNEL);
			copy_from_user(pathname, (char *)arg, size);
			rc = kunlink(pathname);
			printk("<1> kernel got: %s\n",pathname);
			printk("<1> the len is %lu\n", size);
			kfree(pathname);
			return rc;
			break;
		case RD_READDIR:
			copy_from_user(&p, (struct Params *)arg, sizeof(struct Params));
			printk("<1> got p.fd:%d, p.addr: %p\n", p.fd, p.addr);
			addr = (char *)kmalloc(256, GFP_KERNEL);
			rc = kreaddir(p.fd, addr);
			copy_to_user(p.addr, addr, strlen(addr)+1); 
			kfree(addr);
			return 0;
			break;
		}

	return 0;
}

module_init(init_routine);
module_exit(exit_routine);
