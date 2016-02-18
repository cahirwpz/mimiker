#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdint.h>       /* uint*_t, int*_t */
#include <stddef.h>       /* offsetof, NULL, ptrdiff_t, size_t, etc. */
#include <stdbool.h>      /* bool, true, false */
#include <stdalign.h>     /* alignof, alignas */
#include <stdnoreturn.h>  /* noreturn */

#include <mips/cpu.h>
#include <mips/mips32.h>
#include <mips/m32c0.h>

/* Macros for counting and rounding. */
#ifndef howmany
#define howmany(x, y)   (((x) + ((y) - 1)) / (y))
#endif
#define nitems(x)       (sizeof((x)) / sizeof((x)[0]))
#define rounddown(x, y) (((x) / (y)) * (y))
#define roundup(x, y)   ((((x) + ((y) - 1)) / (y)) * (y))
#define powerof2(x)     ((((x) - 1) & (x)) == 0)

/* Aligns the address to given size (must be power of 2) */
#define ALIGN(addr, size) (void *)(((uintptr_t)(addr) + (size) - 1) & -(size))

/* Terminate kernel. */
void kernel_exit();

#define panic(FMT, ...) __extension__ ({                                 \
  kprintf("[panic] %s:%d " FMT "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
  kernel_exit();                                                         \
})

#define log(FMT, ...) __extension__ ({                                 \
  kprintf("[%s:%d] " FMT "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
})

#endif // __COMMON_H__
