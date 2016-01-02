#ifndef __BITSET_H__
#define __BITSET_H__
#include <stdint.h>
#include <stddef.h>
#include "common.h"

/* Bit operations on simple values. */
#define BIT_SET(n, b) ((n) |=  (1 << (b)) )
#define BIT_CLR(n, b) ((n) &= ~(1 << (b)) )
#define BIT_INV(n, b) ((n) ^=  (1 << (b)) )
#define BIT_GET(n, b) (((n) & (1<<(b))) != 0)

/* TODO: This should be in some other file. */

/* ============================== */
/*   Bitsets                      */

/* Defines a variable of bitset<size> type, named "name". This can be
 * used both for creating a bitset variable, as well as for defining a
 * bitset field within a struct. */
#define BITSET_DEFINE(name, size)                       \
    struct name ## _bitset ## size ##_t {               \
        word_t bf[_BITSET_WORDS_NO(size)];           \
    } name;

/* Macros for initialising bitsets. */
#define BITSET_CLEAR(bitset,size)                       \
    do{                                                 \
        size_t i;                                       \
        for( i = 0; i < _BITSET_WORDS_NO(size); i++)    \
            bitset.bf[i] = 0;                           \
    } while(0)
#define BITSET_CLEAR(bitset,size)                       \
    do{                                                 \
        size_t i;                                       \
        for( i = 0; i < _BITSET_WORDS_NO(size); i++)    \
            bitset.bf[i] = 0;                           \
    } while(0)

/* Bit operations on a bitset. */
#define BITSET_BIT_SET(bitset, bitno)                   \
    ( bitset.bf[_BITSET_BIT_WORD(bitno)] |=             \
      _BITSET_BIT_MASK(bitno) )

#define BITSET_BIT_CLR(bitset, bitno)                   \
    ( bitset.bf[_BITSET_BIT_WORD(bitno)] &=             \
      ~_BITSET_BIT_MASK(bitno) )

#define BITSET_BIT_INV(bitset, bitno)                   \
    ( bitset.bf[_BITSET_BIT_WORD(bitno)] ^=             \
      _BITSET_BIT_MASK(bitno) )

/* Returns the value of the queried bit in a bitset. */
#define BITSET_GET(bitset, bitno)                       \
    (bitset.bf[_BITSET_BIT_WORD(bitno)] & _BITSET_BIT_MASK(bitno) != 0)

/* Common operations on bitsets. */

/* dest = ~src */
#define BITSET_NOT(dest,src,size)                       \
    do{                                                 \
        size_t i;                                       \
        for( i = 0; i < _BITSET_WORDS_NO(size); i++)    \
            dest.bf[i] = ~src.bf[i]                     \
    } while(0)
/* dest = src1 | src2 */
#define BITSET_OR(dest,src1,src2,size)                  \
    do{                                                 \
        size_t i;                                       \
        for( i = 0; i < _BITSET_WORDS_NO(size); i++)    \
            dest.bf[i] = src1.bf[i] | src2.bf[i];       \
    } while(0)
/* dest = src1 & src2 */
#define BITSET_AND(dest,src1,src2,size)                 \
    do{                                                 \
        size_t i;                                       \
        for( i = 0; i < _BITSET_WORDS_NO(size); i++)    \
            dest.bf[i] = src1.bf[i] & src2.bf[i];       \
    } while(0)
/* dest = src1 ^ src2 */
#define BITSET_XOR(dest,src1,src2,size)                 \
    do{                                                 \
        size_t i;                                       \
        for( i = 0; i < _BITSET_WORDS_NO(size); i++)    \
            dest.bf[i] = src1.bf[i] ^ src2.bf[i];       \
    } while(0)

/* dest |= src */
#define BITSET_INPLACE_OR(dest,src,size)                \
    do{                                                 \
        size_t i;                                       \
        for( i = 0; i < _BITSET_WORDS_NO(size); i++)    \
            dest.bf[i] |= src.bf[i];                    \
    } while(0)
/* dest &= src */
#define BITSET_INPLACE_AND(dest,src,size)               \
    do{                                                 \
        size_t i;                                       \
        for( i = 0; i < _BITSET_WORDS_NO(size); i++)    \
            dest.bf[i] &= src.bf[i];                    \
    } while(0)
/* dest ^= src */
#define BITSET_INPLACE_XOR(dest,src,size)               \
    do{                                                 \
        size_t i;                                       \
        for( i = 0; i < _BITSET_WORDS_NO(size); i++)    \
            dest.bf[i] ^= src.bf[i];                    \
    } while(0)


/* Comparison between two bitsets. Returns 0 iff both bitsets are
 * eqeual. */
/* Please note that statement statement expressions are a GNU
 * extension. */
#define BITSET_CMP(bitset1, bitset2, size)              \
    ({                                                  \
        size_t i;                                       \
        int nonequal = 0;                               \
        for( i = 0; i < _BITSET_WORDS_NO(size); i++)    \
            if( bitset1.bf[i] != bitset2.bf[i] ){       \
                nonequal = 1;                           \
                break;                                  \
            }                                           \
        nonequal;                                       \
    })

/* Debug macros for printing out the bitfield state via kprintf. The
 * bits are printed in decreasing number, so bit no. 0 is displayed
 * the last. */
#define BITSET_PRINT_HEX(bitset,size) \
    __bitset_printer_hex((bitset).bf, _BITSET_WORDS_NO(size))
#define BITSET_PRINT_BIN(bitset,size) \
    __bitset_printer_bin((bitset).bf, _BITSET_WORDS_NO(size))

/* ================================= */
/* Bitset internal helper functions. */

/* The nunber of words used to store a bitset of given size. */
#define _BITSET_WORDS_NO(size)                          \
    ((size + WORD_SIZE - 1)/WORD_SIZE)

/* The number of the word to which a given bit belongs. */
#define _BITSET_BIT_WORD(bitno)                         \
    ((bitno)/(WORD_SIZE))
/* The mask for a given bit to be applied with the macro above. */
#define _BITSET_BIT_MASK(bitno)                         \
    (1 << ((bitno)%(WORD_SIZE)))

/* Helper functions for displaying bitfields. Please use
 * BITSET_PRINT_* instead. */
void __bitset_printer_hex(word_t* bf, size_t size);
void __bitset_printer_bin(word_t* bf, size_t size);



#endif // __BITSET_H__
