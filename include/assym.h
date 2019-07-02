#ifndef __SYS_ASSYM_H__
#define __SYS_ASSYM_H__

/* Add an ASSYM_BIAS to all arrays to avoid zero-size arrays,
 * which are turned into one byte arrays by Clang compiler. */
#define ASSYM_BIAS 0x10000000

#define ASSYM(name, value) char name##$[(value) + ASSYM_BIAS]

#endif
