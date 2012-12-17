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

int init_fs() {
    // INIT FILESYSTEM:
    rd = (struct Ramdisk *)malloc(sizeof(struct Ramdisk));
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
}

int is_block_empty(union Block *blk) {
	struct Block_reg re = blk->reg;
	int i;
	for (i = 0; i < BLOCK_SIZE; i++) {
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
		for (k = 0; k < BLOCK_SIZE/4; k++) {
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
				for (j = 0; j < BLOCK_SIZE/4; j++) {
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
		for (k = 0; k < BLOCK_SIZE/4; k++) {
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
				for (j = 0; j < BLOCK_SIZE/4; j++) {
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
	int size = 1, node = 0, found = 0;
	//allocate minimum space for array
	//root inode will always be 0
	short fotrace[1024];
	fotrace[0] = 0;
	found = recursive_inode_search(fotrace, &size, 0, inode);
	if (!found)
		return -1; //fieldname not found
	memcpy(trace, fotrace, size * sizeof(short));
	return size;
}

int recursive_inode_size_add(short inode, int size) {
	short parent_array[1024];
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
		for (k = 0; k < BLOCK_SIZE/4; k++) {
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
				for (j = 0; j < BLOCK_SIZE/4; j++) {
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

int find_free_block() {
    // Find a new free block, using first-fit. NOTE: before returning, we SET THE BITMAP = 1 !!!!!!
    if (SHOW_FREEBLOCK == 0) {
        return -1;
    } else {
        int j = 0;
        for (j; j < PARTITION_NUMBER; j++) {
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

int find_free_inode() {
    // Find a new free block, using first-fit.
    if (SHOW_FREEINODE == 0) {
        return -1;
    } else {
        int j = 0;
        for (j; j < MAX_FILE_COUNT; j++) {
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
    while(i < 10) {
        // If there is allocated block?
        if (GET_INODE_LOCATION_BLOCK(super_inode, i) == 0) {
            //printf("i = %d, is empty\n", i);
            // Allocate partition blocks for super_inode
            int fb = find_free_block();
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
                int fb2 = find_free_block();
                // 2. Assign new block index[8].blocks[0];
                ASSIGN_LOCATION_SINGLE_RED(super_inode, 0, fb2);
                SET_DIR_ENTRY_NAME(fb2, 0, filename);
                SET_DIR_ENTRY_INODE(fb2, 0, new_inode);
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
                        // Insert inode into this location
                        SET_INODE_FROM_INODE_LOCATION_INODE(super_inode, i, j, new_inode);
                        SET_INODE_FROM_INODE_LOCATION_FILENAME(super_inode, i, j, filename);
                        PRINT_INODE_FROM_INODE_LOCATION(super_inode, i, j);
                        return 1;
                    }
                }
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
                        return 1;
                    } else {
                        // A dir block is present, loop through to find free location
                        int k = 0;
                        for (k; k < 16; k++) {
                            // Loop through the Dir Block
                            if (GET_INODE_FROM_INODE_LOCATION_SIN_INODE(super_inode, j, k) == 0) {
                            SET_INODE_FROM_INODE_LOCATION_SIN_INODE(super_inode, j, k, new_inode);
                            SET_INODE_FROM_INODE_LOCATION_SIN_FILENAME(super_inode, j, k, filename);
                            PRINT_INODE_FROM_INODE_LOCATION_SIN(super_inode, j, k);
                            return 1;
                            }
                        }
                    }
                }
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
    PRINT_FREEINODE_COUNT;
    PRINT_FREEBLOCK_COUNT;
    // kernel creat. Create a file
    int fi = find_free_inode();
    if (fi < 0) {
        return -1;
    }
    // Check pathname and get last entry.
    char *last = (char *)kmalloc(14);
    short super_inode;
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
        return -1;
    }
    return 0;
}

int kmkdir(char *pathname) {
    // kernel mkdir. Create a DIR
    int fi = find_free_inode();
    if (fi == -1) {
        return -1;
    }

    
    // Check_pathname and get last entry
    char *last = (char *)kmalloc(14);
    short super_inode;

    if (check_pathname(pathname, last, &super_inode) != 0) {
        // Pathname failed.
        return -1;
    }    
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
	vfree(ramdisk);

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
	char blah[8] = "bigfoot";
	/* 
	 * Switch according to the ioctl called 
	 */
	switch (cmd) {
		case RD_MALLOC:
			// vmalloc for 2MB
			ramdisk = (unsigned char *)vmalloc(2097150);
			printk("<1>I finished vmalloc!\n");
			break;
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
			rc = kread(p.fd, addr, p.num_bytes);
			copy_to_user(p.addr, addr, p.num_bytes); 
			return rc;
			break;
		case RD_WRITE:
			copy_from_user(&p, (struct Params *)arg, sizeof(struct Params));
			printk("<1> got p.fd:%d, p.addr: %p, p.byte_size:%d\n", p.fd, p.addr, p.num_bytes);
			rc = kwrite(p.fd, addr, p.num_bytes);
			copy_to_user(p.addr, addr, p.num_bytes); 
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
			rc = kreaddir(p.fd, addr);
			copy_to_user(p.addr, addr, strlen(addr)+1); 
			return 0;
			break;
		}

	return 0;
}

module_init(init_routine);
module_exit(exit_routine);
