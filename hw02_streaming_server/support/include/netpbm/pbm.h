/* pbm.h - header file for libpbm portable bitmap library
*/

#ifndef _PBM_H_
#define _PBM_H_

#include "pm.h"

typedef unsigned char bit;
#define PBM_WHITE 0
#define PBM_BLACK 1


/* Magic constants. */

#define PBM_MAGIC1 'P'
#define PBM_MAGIC2 '1'
#define RPBM_MAGIC2 '4'
#define PBM_FORMAT (PBM_MAGIC1 * 256 + PBM_MAGIC2)
#define RPBM_FORMAT (PBM_MAGIC1 * 256 + RPBM_MAGIC2)
#define PBM_TYPE PBM_FORMAT


/* Macro for turning a format number into a type number. */

#define PBM_FORMAT_TYPE(f) \
  ((f) == PBM_FORMAT || (f) == RPBM_FORMAT ? PBM_TYPE : -1)


/* Declarations of routines. */

void pbm_init ARGS(( int* argcP, char* argv[] ));
void
pbm_nextimage(FILE *file, int * const eofP);

#define pbm_allocarray(cols, rows) \
  ((bit**) pm_allocarray(cols, rows, sizeof(bit)))
#define pbm_allocrow(cols) ((bit*) pm_allocrow( cols, sizeof(bit)))
#define pbm_freearray(bits, rows) pm_freearray((char**) bits, rows)
#define pbm_freerow(bitrow) pm_freerow((char*) bitrow)
#define pbm_packed_bytes(cols) (((cols)+7)/8)

bit** pbm_readpbm(FILE* file, int* colsP, int* rowsP);
void pbm_readpbminit(FILE* file, int* colsP, int* rowsP, int* formatP);
void pbm_readpbmrow(FILE* file, bit* bitrow, int cols, int format);
void pbm_readpbmrow_packed(
    FILE* const file, unsigned char * const packed_bits, 
    const int cols, const int format);

void pbm_writepbm (FILE* file, bit** bits, int cols, int rows, int forceplain);
void pbm_writepbminit(FILE* file, int cols, int rows, int forceplain);
void pbm_writepbmrow (FILE* file, bit* bitrow, int cols, int forceplain);
void pbm_writepbmrow_packed(
    FILE* const file, const unsigned char * const packed_bits, 
    const int cols, const int format);

void
pbm_check(FILE * file, const enum pm_check_type check_type, 
          const int format, const int cols, const int rows,
          enum pm_check_code * const retval_p);

#endif /*_PBM_H_*/

