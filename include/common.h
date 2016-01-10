#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdint.h>
#include <stddef.h>

/* Extra types. */

typedef intptr_t word_t;

/* Useful macros*/
#define STR_NOEXPAND(x) #x     // This does not expand x.
#define STR(x) STR_NOEXPAND(x) // This will expand x.

/* Target platform properties. */
#define BITS_IN_BYTE 8
#define WORD_SIZE (sizeof(word_t) * BITS_IN_BYTE)

/* Aligns the address to given size (must be power of 2). */
#define ALIGN(addr, size) (((uintptr_t)(addr) + (size) - 1) & ~((size) - 1))

#endif // __COMMON_H__
