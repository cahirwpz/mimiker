#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <common.h>

/* Defines a variable of bitmap<size> type, named "name". This can be
 * used both for creating a bitmap variable, as well as for defining a
 * bitmap field within a struct. */
#define BITMAP_DEFINE(name, size)                       \
    struct name ## _bitmap ## size ##_t {               \
        word_t bf[_BITMAP_WORDS_NO(size)];              \
    } name;

/* Macros for initialising bitmaps. */
#define BITMAP_ZERO(bitmap,size)                        \
    do{                                                 \
        size_t i;                                       \
        for( i = 0; i < _BITMAP_WORDS_NO(size); i++)    \
            bitmap.bf[i] = 0;                           \
    } while(0)
#define BITMAP_FILL(bitmap,size)                        \
    do{                                                 \
        size_t i;                                       \
        for( i = 0; i < _BITMAP_WORDS_NO(size); i++)    \
            bitmap.bf[i] = ~((word_t)0);                \
    } while(0)

/* Bit operations on a bitmap. */
#define BITMAP_SET(bitmap, bitno)                       \
    ( bitmap.bf[_BITMAP_BIT_WORD(bitno)] |=             \
      _BITMAP_BIT_MASK(bitno) )

#define BITMAP_CLR(bitmap, bitno)                       \
    ( bitmap.bf[_BITMAP_BIT_WORD(bitno)] &=             \
      ~_BITMAP_BIT_MASK(bitno) )

#define BITMAP_INV(bitmap, bitno)                       \
    ( bitmap.bf[_BITMAP_BIT_WORD(bitno)] ^=             \
      _BITMAP_BIT_MASK(bitno) )

/* Returns the value of the queried bit in a bitmap. */
#define BITMAP_GET(bitmap, bitno)                       \
    (bitmap.bf[_BITMAP_BIT_WORD(bitno)] & _BITMAP_BIT_MASK(bitno) != 0)

/* Common operations on bitmaps. */

/* dest = ~src */
#define BITMAP_NOT(dest,src,size)                       \
    do{                                                 \
        size_t i;                                       \
        for( i = 0; i < _BITMAP_WORDS_NO(size); i++)    \
            dest.bf[i] = ~src.bf[i]                     \
    } while(0)
/* dest = src1 | src2 */
#define BITMAP_OR(dest,src1,src2,size)                  \
    do{                                                 \
        size_t i;                                       \
        for( i = 0; i < _BITMAP_WORDS_NO(size); i++)    \
            dest.bf[i] = src1.bf[i] | src2.bf[i];       \
    } while(0)
/* dest = src1 & src2 */
#define BITMAP_AND(dest,src1,src2,size)                 \
    do{                                                 \
        size_t i;                                       \
        for( i = 0; i < _BITMAP_WORDS_NO(size); i++)    \
            dest.bf[i] = src1.bf[i] & src2.bf[i];       \
    } while(0)
/* dest = src1 ^ src2 */
#define BITMAP_XOR(dest,src1,src2,size)                 \
    do{                                                 \
        size_t i;                                       \
        for( i = 0; i < _BITMAP_WORDS_NO(size); i++)    \
            dest.bf[i] = src1.bf[i] ^ src2.bf[i];       \
    } while(0)

/* dest |= src */
#define BITMAP_INPLACE_OR(dest,src,size)                \
    do{                                                 \
        size_t i;                                       \
        for( i = 0; i < _BITMAP_WORDS_NO(size); i++)    \
            dest.bf[i] |= src.bf[i];                    \
    } while(0)
/* dest &= src */
#define BITMAP_INPLACE_AND(dest,src,size)               \
    do{                                                 \
        size_t i;                                       \
        for( i = 0; i < _BITMAP_WORDS_NO(size); i++)    \
            dest.bf[i] &= src.bf[i];                    \
    } while(0)
/* dest ^= src */
#define BITMAP_INPLACE_XOR(dest,src,size)               \
    do{                                                 \
        size_t i;                                       \
        for( i = 0; i < _BITMAP_WORDS_NO(size); i++)    \
            dest.bf[i] ^= src.bf[i];                    \
    } while(0)


/* Comparison between two bitmaps. Returns 0 iff both bitmaps are
 * eqeual. */
/* Please note that statement statement expressions are a GNU
 * extension. */
#define BITMAP_CMP(bitmap1, bitmap2, size)              \
    ({                                                  \
        size_t i;                                       \
        int nonequal = 0;                               \
        for( i = 0; i < _BITMAP_WORDS_NO(size); i++)    \
            if( bitmap1.bf[i] != bitmap2.bf[i] ){       \
                nonequal = 1;                           \
                break;                                  \
            }                                           \
        nonequal;                                       \
    })

/* Debug macros for printing out the bitfield state via kprintf. The
 * bits are printed in decreasing number, so bit no. 0 is displayed
 * the last. */
#define BITMAP_PRINT_HEX(bitmap,size) \
    __bitmap_printer_hex((bitmap).bf, _BITMAP_WORDS_NO(size))
#define BITMAP_PRINT_BIN(bitmap,size) \
    __bitmap_printer_bin((bitmap).bf, _BITMAP_WORDS_NO(size))

/* ================================= */
/* Bitmap internal helper functions. */

/* The nunber of words used to store a bitmap of given size. */
#define _BITMAP_WORDS_NO(size)                          \
    ((size + WORD_SIZE - 1)/WORD_SIZE)

/* The number of the word to which a given bit belongs. */
#define _BITMAP_BIT_WORD(bitno)                         \
    ((bitno)/(WORD_SIZE))
/* The mask for a given bit to be applied with the macro above. */
#define _BITMAP_BIT_MASK(bitno)                         \
    (1 << ((bitno)%(WORD_SIZE)))

/* Helper functions for displaying bitfields. Please use
 * BITMAP_PRINT_* instead. */
void __bitmap_printer_hex(word_t* bf, size_t size);
void __bitmap_printer_bin(word_t* bf, size_t size);

#endif // __BITMAP_H__
