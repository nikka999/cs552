#include <stdint.h>
 
void kmain(void)
{
   extern uint32_t magic;
   extern void *mbd;
 
   if ( magic != 0x2BADB002 )
   {
      /* Something went not according to specs. Print an error */
      /* message and halt, but do *not* rely on the multiboot */
      /* data structure. */
   }
 
   /* You could either use multiboot.h */
   /* (http://www.gnu.org/software/grub/manual/multiboot/multiboot.html#multiboot_002eh) */
   /* or do your offsets yourself. The following is merely an example. */ 
   //char * boot_loader_name =(char*) ((long*)mbd)[16];
 
   /* Print a letter to screen to see everything is working: */
   unsigned char *videoram = (char *)0xB8000;
   videoram[0] = 78; /* character 'A' */
   videoram[1] = 0x07; /* light grey (7) on black (0). */
   videoram[2] = 105; /* character 'A' */
   videoram[3] = 0x07; /* light grey (7) on black (0). */
   videoram[4] = 107; /* character 'A' */
   videoram[5] = 0x07; /* light grey (7) on black (0). */
   videoram[6] = 107; /* character 'A' */
   videoram[7] = 0x07; /* light grey (7) on black (0). */
   videoram[8] = 97; /* character 'A' */
   videoram[9] = 0x07; /* light grey (7) on black (0). */
   videoram[10] = 32; /* character 'A' */
   videoram[11] = 0x07; /* light grey (7) on black (0). */
   videoram[12] = 38; /* character 'A' */
   videoram[13] = 0x07; /* light grey (7) on black (0). */
   videoram[14] = 32; /* character 'A' */
   videoram[15] = 0x07; /* light grey (7) on black (0). */
   videoram[16] = 77; /* character 'A' */
   videoram[17] = 0x07; /* light grey (7) on black (0). */
   videoram[18] = 105; /* character 'A' */
   videoram[19] = 0x07; /* light grey (7) on black (0). */
   videoram[20] = 107; /* character 'A' */
   videoram[21] = 0x07; /* light grey (7) on black (0). */
   videoram[22] = 101; /* character 'A' */
   videoram[23] = 0x07; /* light grey (7) on black (0). */
   videoram[24] = 0; /* character 'A' */
   videoram[25] = 0x07; /* light grey (7) on black (0). */
   videoram[26] = 0; /* character 'A' */
   videoram[27] = 0x07; /* light grey (7) on black (0). */
   videoram[28] = 0; /* character 'A' */
   videoram[29] = 0x07; /* light grey (7) on black (0). */
   videoram[30] = 0; /* character 'A' */
   videoram[31] = 0x07; /* light grey (7) on black (0). */
   videoram[32] = 0; /* character 'A' */
   videoram[33] = 0x07; /* light grey (7) on black (0). */
   videoram[34] = 0; /* character 'A' */
   videoram[35] = 0x07; /* light grey (7) on black (0). */
}
