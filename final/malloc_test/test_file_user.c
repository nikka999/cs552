/* CS552 -- Intro to Operating Systems
         -- Richard West
   
   -- template test file for RAMDISK Filesystem Assignment.
   -- include a case for:
   -- two processes 
   -- largest number of files (should be 1024 max)
   -- largest single file (start with direct blocks [2048 bytes max], 
   then single-indirect [18432 bytes max] and finally double 
   indirect [1067008 bytes max])
   -- creating and unlinking files to avoid memory leaks
   -- each file operation
   -- error checking on invalid inputs
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "m.h"

// #define's to control what tests are performed,
// comment out a test if you do not wish to perform it

//#define TEST1
#define TEST2
#define TEST3
//#define TEST4
//#define TEST5

// #define's to control whether single indirect or
// double indirect block pointers are tested

#define TEST_SINGLE_INDIRECT
#define TEST_DOUBLE_INDIRECT


#define MAX_FILES 1023
#define BLK_SZ 256		/* Block size */
#define DIRECT 8		/* Direct pointers in location attribute */
#define PTR_SZ 4		/* 32-bit [relative] addressing */
#define PTRS_PB  (BLK_SZ / PTR_SZ) /* Pointers per index block */

static char pathname[80];

static char data1[DIRECT*BLK_SZ]; /* Largest data directly accessible */
static char data2[PTRS_PB*BLK_SZ];     /* Single indirect data size */
static char data3[PTRS_PB*PTRS_PB*BLK_SZ]; /* Double indirect data size */
static char addr[PTRS_PB*PTRS_PB*BLK_SZ+1]; /* Scratchpad memory */

int main () {
    
    // INIT FS HERE
    init_fs();
    
  int retval, i;
  int fd;
  int index_node_number;

  /* Some arbitrary data for our files */
  memset (data1, '1', sizeof (data1));
  memset (data2, '2', sizeof (data2));
  memset (data3, '3', sizeof (data3));


#ifdef TEST1

  /* ****TEST 1: MAXIMUM file creation**** */

  /* Assumes the pre-existence of a root directory file "/"
     that is neither created nor deleted in this test sequence */

  /* Generate MAXIMUM regular files */
  for (i = 0; i < MAX_FILES + 1; i++) { // go beyond the limit
    sprintf (pathname, "/file%d", i);
    
    retval = kcreat (pathname);
    
    if (retval < 0) {
      fprintf (stderr, "kcreate: File creation error! status: %d\n", 
	       retval);
      
      if (i != MAX_FILES)
	exit (1);
    }
    
    memset (pathname, 0, 80);
  }   

  /* Delete all the files created */
  for (i = 0; i < MAX_FILES; i++) { 
    sprintf (pathname, "/file%d", i);
    
    retval = kunlink (pathname);
    
    if (retval < 0) {
      fprintf (stderr, "kunlink: File deletion error! status: %d\n", 
	       retval);
      
      exit (1);
    }
    
    memset (pathname, 0, 80);
  }

#endif // TEST1
  
#ifdef TEST2

  /* ****TEST 2: LARGEST file size**** */

  
  /* Generate one LARGEST file */
  retval = kcreat ("/bigfile");

  if (retval < 0) {
    fprintf (stderr, "kcreat: File creation error! status: %d\n", 
	     retval);

    exit (1);
  }

    printf("kcreat succesful\n");
    
  retval =  kopen ("/bigfile"); /* Open file to write to it */
  
  if (retval < 0) {
    fprintf (stderr, "kopen: File open error! status: %d\n", 
	     retval);

    exit (1);
  }

    printf("kopen succesful\n");
  fd = retval;			/* Assign valid fd */

  /* Try writing to all direct data blocks */
  retval = kwrite (fd, data1, sizeof(data1));
  
  if (retval < 0) {
    fprintf (stderr, "kwrite: File write STAGE1 error! status: %d\n", 
	     retval);

    exit (1);
  }

    printf("kwrite direct succesful\n");
#ifdef TEST_SINGLE_INDIRECT
  
  /* Try writing to all single-indirect data blocks */
  retval = kwrite (fd, data2, sizeof(data2));
    
  if (retval < 0) {
    fprintf (stderr, "kwrite: File write STAGE2 error! status: %d\n", 
	     retval);

    exit (1);
  }

    printf("kwrite 1 indirect succesful\n");
#ifdef TEST_DOUBLE_INDIRECT

  /* Try writing to all double-indirect data blocks */
  retval = kwrite (fd, data3, sizeof(data3));
  printf("kwrite 2 actually writes = %d, suppose to write = %ld\n", retval, sizeof(data3));
  if (retval < 0) {
    fprintf (stderr, "kwrite: File write STAGE3 error! status: %d\n", 
	     retval);

    exit (1);
  }
    
    printf("PASSED TEST2\n");

#endif // TEST_DOUBLE_INDIRECT

#endif // TEST_SINGLE_INDIRECT

#endif // TEST2

#ifdef TEST3

  /* ****TEST 3: Seek and Read file test**** */
  retval = klseek (fd, 0);	/* Go back to the beginning of your file */
  if (retval < 0) {
    fprintf (stderr, "klseek: File seek error! status: %d\n", 
	     retval);

    exit (1);
  }

  /* Try reading from all direct data blocks */
  retval = kread (fd, addr, sizeof(data1));
  
  if (retval < 0) {
    fprintf (stderr, "kread: File read STAGE1 error! status: %d\n", 
	     retval);

    exit (1);
  }
  /* Should be all 1s here... */
  printf ("Data at addr: %s\n", addr);
    
    printf("PASSED TEST3-Direct Block read\n");
    
#ifdef TEST_SINGLE_INDIRECT

  /* Try reading from all single-indirect data blocks */
  retval = kread (fd, addr, sizeof(data2));
  
  if (retval < 0) {
    fprintf (stderr, "kread: File read STAGE2 error! status: %d\n", 
	     retval);

    exit (1);
  }
  /* Should be all 2s here... */
  printf ("Data at addr: %s\n", addr);
    
    printf("PASSED TEST3-1 Redirect Block read\n");
#ifdef TEST_DOUBLE_INDIRECT

  /* Try reading from all double-indirect data blocks */
  retval = kread (fd, addr, sizeof(data3));
  
  if (retval < 0) {
    fprintf (stderr, "kread: File read STAGE3 error! status: %d\n", 
	     retval);

    exit (1);
  }
  /* Should be all 3s here... */
  printf ("Data at addr: %s\n", addr);
printf("PASSED TEST3-2 Redirect Block read\n");
#endif // TEST_DOUBLE_INDIRECT

#endif // TEST_SINGLE_INDIRECT

  /* Close the bigfile */
  retval = kclose (fd);
  
  if (retval < 0) {
    fprintf (stderr, "kclose: File close error! status: %d\n", 
	     retval);

    exit (1);
  }

  /* Remove the biggest file */

  retval = kunlink ("/bigfile");
	
  if (retval < 0) {
    fprintf (stderr, "kunlink: /bigfile file deletion error! status: %d\n", 
	     retval);
    
    exit (1);
  }

#endif // TEST3

#ifdef TEST4
  
  /* ****TEST 4: Make directory and read directory entries**** */
  retval = kmkdir ("/dir1");
    
  if (retval < 0) {
    fprintf (stderr, "kmkdir: Directory 1 creation error! status: %d\n", 
	     retval);

    exit (1);
  }

  retval = kmkdir ("/dir1/dir2");
    
  if (retval < 0) {
    fprintf (stderr, "kmkdir: Directory 2 creation error! status: %d\n", 
	     retval);

    exit (1);
  }

  retval = kmkdir ("/dir1/dir3");
    
  if (retval < 0) {
    fprintf (stderr, "kmkdir: Directory 3 creation error! status: %d\n", 
	     retval);

    exit (1);
  }

  retval =  kopen ("/dir1"); /* Open directory file to read its entries */
  
  if (retval < 0) {
    fprintf (stderr, "kopen: Directory open error! status: %d\n", 
	     retval);

    exit (1);
  }

  fd = retval;			/* Assign valid fd */

  memset (addr, 0, sizeof(addr)); /* Clear scratchpad memory */

  while ((retval = kreaddir (fd, addr))) { /* 0 indicates end-of-file */

    if (retval < 0) {
      fprintf (stderr, "kreaddir: Directory read error! status: %d\n", 
	       retval);
      
      exit (1);
    }
      /*
      printf("user\n");
      int zz = 0;
      for (zz; zz < 16; zz++) {
          printf("%d ", *((unsigned char *)addr + zz));
      }
      printf("\n");
       */
    index_node_number = atoi(&addr[14]);
    printf ("Contents at addr: [%s,%d]\n", addr, index_node_number);
  }

#endif // TEST4

#ifdef TEST5

  /* ****TEST 5: 2 process test**** */
  
  if((retval = fork())) {

    if(retval == -1) {
      fprintf(stderr, "Failed to fork\n");
      
      exit(1);
    }

    /* Generate 300 regular files */
    for (i = 0; i < 300; i++) { 
      sprintf (pathname, "/file_p_%d", i);
      
      retval = kcreat (pathname);
      
      if (retval < 0) {
	fprintf (stderr, "(Parent) kcreate: File creation error! status: %d\n", 
		 retval);

	exit(1);
      }
    
      memset (pathname, 0, 80);
    }
    
  }
  else {
    /* Generate 300 regular files */
    for (i = 0; i < 300; i++) { 
      sprintf (pathname, "/file_c_%d", i);
      
      retval = kcreat (pathname);
      
      if (retval < 0) {
	fprintf (stderr, "(Child) kcreate: File creation error! status: %d\n", 
		 retval);

	exit(1);
      }
    
      memset (pathname, 0, 80);
    }
  }

#endif // TEST5


  fprintf(stdout, "Congratulations,  you have passed all tests!!\n");

  return 0;
}
