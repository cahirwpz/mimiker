#ifndef _SYS_COMMON_H_
#define _SYS_COMMON_H_

#include <stdint.h>      /* uint*_t, int*_t */
#include <stddef.h>      /* offsetof, NULL, ptrdiff_t, size_t, etc. */
#include <stdbool.h>     /* bool, true, false */
#include <stdalign.h>    /* alignof, alignas */
#include <stdnoreturn.h> /* noreturn */

typedef unsigned long vm_addr_t;
typedef unsigned long pm_addr_t;

typedef long off_t;
typedef long ssize_t;
typedef int32_t pid_t;
typedef uint16_t dev_t;
typedef uint32_t time_t;
typedef uint16_t uid_t;
typedef uint16_t gid_t;
typedef uint32_t mode_t;
typedef uint16_t nlink_t;
typedef uint16_t ino_t;

/* Generic preprocessor macros */
#define __STRING(x) #x
#define __CONCAT1(x, y) x##y
#define __CONCAT(x, y) __CONCAT1(x, y)

/* Wrapper for various GCC attributes */
#define __nonnull(x) __attribute__((__nonnull__(x)))
#define __section(s) __attribute__((__section__(#s)))
#define __unused __attribute__((unused))
#define __used __attribute__((used))

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

#define cleanup(func) __attribute__((__cleanup__(cleanup_##func)))
#define DEFINE_CLEANUP_FUNCTION(type, func)                                    \
  static inline void cleanup_##func(type *ptr) {                               \
    if (*ptr)                                                                  \
      func(*ptr);                                                              \
  }                                                                            \
  struct __force_semicolon__

#ifndef _USERSPACE

/* Terminate thread. */
noreturn void thread_exit();

#define panic(FMT, ...)                                                        \
  __extension__({                                                              \
    kprintf("[%s:%d] " FMT "\n", __FILE__, __LINE__, ##__VA_ARGS__);           \
    thread_exit(-1);                                                           \
  })

#ifdef DEBUG
#define log(FMT, ...)                                                          \
  __extension__(                                                               \
    { kprintf("[%s:%d] " FMT "\n", __FILE__, __LINE__, ##__VA_ARGS__); })

void assert_fail(const char *expr, const char *file, unsigned int line);

#define assert(EXPR)                                                           \
  __extension__({                                                              \
    if (!(EXPR))                                                               \
      assert_fail(__STRING(EXPR), __FILE__, __LINE__);                         \
  })
#else
#define log(...)
#define assert(expr)
#endif

#else
#include <assert.h>
#endif /* !_USERSPACE */

#endif /* !_SYS_COMMON_H_ */
