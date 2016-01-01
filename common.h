#ifndef __COMMON_H__
#define __COMMON_H__


/* Useful macros*/

#define STR_NOEXPAND(x) #x     // This does not expand x.
#define STR(x) STR_NOEXPAND(x) // This will expand x.


/* Target platform properties. */

#define BITS_IN_BYTE 8
#define WORD_TYPE uint32_t
#define WORD_SIZE (sizeof(WORD_TYPE)*BITS_IN_BYTE)

#define MHZ     200             /* CPU clock. */
#define TICKS_PER_MS (1000 * MHZ / 2)


#endif // __COMMON_H__
