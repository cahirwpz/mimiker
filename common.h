#ifndef __COMMON_H__
#define __COMMON_H__
#include <stdint.h>

/* Useful macros*/

#define STR_NOEXPAND(x) #x     // This does not expand x.
#define STR(x) STR_NOEXPAND(x) // This will expand x.


/* Target platform properties. */

#define BITS_IN_BYTE 8

typedef intptr_t word_t;
#define WORD_SIZE (sizeof(word_t)*BITS_IN_BYTE)


#endif // __COMMON_H__
