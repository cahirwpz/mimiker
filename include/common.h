#ifndef _SYS_COMMON_H_
#define _SYS_COMMON_H_

#include <stdint.h>      /* uint*_t, int*_t */
#include <stddef.h>      /* offsetof, NULL, ptrdiff_t, size_t, etc. */
#include <stdbool.h>     /* bool, true, false */
#include <stdalign.h>    /* alignof, alignas */
#include <stdnoreturn.h> /* noreturn */
#include <sys/types.h>

typedef unsigned long vm_addr_t;
typedef unsigned long pm_addr_t;

/* Wrapper for various GCC attributes */
#define __nonnull(x) __attribute__((__nonnull__(x)))

/* Macros for counting and rounding. */
#ifndef howmany
#define howmany(x, y) (((x) + ((y)-1)) / (y))
#endif
#define nitems(x) (sizeof((x)) / sizeof((x)[0]))
#define rounddown(x, y) (((x) / (y)) * (y))
#define roundup(x, y) ((((x) + ((y)-1)) / (y)) * (y))
#define powerof2(x) ((((x)-1) & (x)) == 0)
#define log2(x) (__builtin_ffs(x) - 1)
#define ffs(x) (__builtin_ffs(x))
#define clz(x) (__builtin_clz(x))
#define ctz(x) (__builtin_ctz(x))

#ifndef __STRING
#define __STRING(x) #x
#endif

#define abs(x)                                                                 \
  ({                                                                           \
    typeof(x) _x = (x);                                                        \
    (_x < 0) ? -_x : _x;                                                       \
  })

#define min(a, b)                                                              \
  ({                                                                           \
    typeof(a) _a = (a);                                                        \
    typeof(b) _b = (b);                                                        \
    _a < _b ? _a : _b;                                                         \
  })

#define max(a, b)                                                              \
  ({                                                                           \
    typeof(a) _a = (a);                                                        \
    typeof(b) _b = (b);                                                        \
    _a > _b ? _a : _b;                                                         \
  })

#define swap(a, b)                                                             \
  ({                                                                           \
    typeof(a) _a = (a);                                                        \
    typeof(a) _b = (b);                                                        \
    (a) = _b;                                                                  \
    (b) = _a;                                                                  \
  })

/* Aligns the address to given size (must be power of 2) */
#define align(addr, size)                                                      \
  ({                                                                           \
    intptr_t _addr = (intptr_t)(addr);                                         \
    intptr_t _size = (intptr_t)(size);                                         \
    _addr = (_addr + (_size - 1)) & -_size;                                    \
    (typeof(addr)) _addr;                                                      \
  })

#define is_aligned(addr, size)                                                 \
  ({                                                                           \
    intptr_t _addr = (intptr_t)(addr);                                         \
    intptr_t _size = (intptr_t)(size);                                         \
    !(_addr & (_size - 1));                                                    \
  })

#ifndef _USERSPACE

/* Terminate thread. */
noreturn void thread_exit();

#define panic(FMT, ...)                                                        \
  __extension__({                                                              \
    kprintf("[%s:%d] " FMT "\n", __FILE__, __LINE__, ##__VA_ARGS__);           \
    thread_exit();                                                             \
  })

#define log(FMT, ...)                                                          \
  __extension__(                                                               \
    { kprintf("[%s:%d] " FMT "\n", __FILE__, __LINE__, ##__VA_ARGS__); })

#ifdef DEBUG
#define assert(EXPR)                                                           \
  __extension__({                                                              \
    if (!(EXPR))                                                               \
      panic("Assertion '" __STRING(EXPR) "' failed!");                         \
  })
#else
#define assert(expr)
#endif

#else
#include <assert.h>
#endif // _USERSPACE

#endif // _SYS_COMMON_H_
