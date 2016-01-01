#ifndef GLOBAL_CONFIG_H
#define GLOBAL_CONFIG_H

#define MHZ     200             /* CPU clock. */
#define TICKS_PER_MS (1000 * MHZ / 2)

#define BITS_IN_BYTE 8
#define WORD_TYPE uint32_t
#define WORD_SIZE (sizeof(WORD_TYPE)*BITS_IN_BYTE)

#endif // GLOBAL_CONFIG_H
