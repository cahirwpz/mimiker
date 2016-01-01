#ifndef __BITSET_H__
#define __BITSET_H__
#include <stdint.h>
#include <stddef.h>
#include "global_config.h"

/* Bit operations on simple values. */
#define BIT_SET(n, b) ((n) |=  (1 << (b)) )
#define BIT_CLR(n, b) ((n) &= ~(1 << (b)) )
#define BIT_INV(n, b) ((n) ^=  (1 << (b)) )
#define BIT_GET(n, b) (((n) & (1<<(b))) != 0)

/* TODO: This should be in some other file. */
#define STR_NOEXPAND(x) #x     // This does not expand x.
#define STR(x) STR_NOEXPAND(x) // This will expand x.

/* ============================== */
/*   Bitsets                      */

/* Defines a variable of bitset<size> type, named "name". This can be
 * used both for creating a bitset variable, as well as for defining a
 * bitset field within a struct. */
#define BITSET_DEFINE(name, size)                       \
    struct name ## _bitset ## size ##_t {               \
        WORD_TYPE bf[_BITSET_WORDS_NO(size)];           \
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
    bitset.bf[_BITSET_BIT_WORD(bitno)] |=               \
        _BITSET_BIT_MASK(bitno)

#define BITSET_BIT_CLR(bitset, bitno)                   \
    bitset.bf[_BITSET_BIT_WORD(bitno)] &=               \
       ~_BITSET_BIT_MASK(bitno)

#define BITSET_BIT_INV(bitset, bitno)                   \
    bitset.bf[_BITSET_BIT_WORD(bitno)] ^=               \
        _BITSET_BIT_MASK(bitno)

/* Returns the value of the queried bit in a bitset. */
#define BITSET_GET(bitset, bitno)                       \
    (bitset.bf[_BITSET_BIT_WORD(bitno)] & _BITSET_BIT_MASK(bitno) != 0)

/* Debug macros for printing out the bitfield state via kprintf. The
 * bits are printed in decreasing number, so bit no. 0 is displayed
 * the last. */
#define BITSET_PRINT_HEX(bitset,size) \
    __bitset_printer_hex((bitset).bf, _BITSET_WORDS_NO(size));
#define BITSET_PRINT_BIN(bitset,size) \
    __bitset_printer_bin((bitset).bf, _BITSET_WORDS_NO(size));

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
void __bitset_printer_hex(WORD_TYPE* bf, size_t size);
void __bitset_printer_bin(WORD_TYPE* bf, size_t size);



#endif // __BITSET_H__
