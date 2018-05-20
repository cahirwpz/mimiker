#ifndef _SYS_COMMON_H_
#define _SYS_COMMON_H_

#include <limits.h>      /* UINT_MAX, LONG_MIN, ... */
#include <stdint.h>      /* uint*_t, int*_t */
#include <stddef.h>      /* offsetof, NULL, ptrdiff_t, size_t, etc. */
#include <stdbool.h>     /* bool, true, false */
#include <stdalign.h>    /* alignof, alignas */
#include <stdnoreturn.h> /* noreturn */

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;

typedef unsigned long vm_addr_t;
typedef unsigned long pm_addr_t;

typedef long off_t;
typedef long ssize_t;
typedef uint8_t prio_t;
typedef int32_t pid_t;
typedef uint16_t dev_t;
typedef uint32_t time_t;
typedef int32_t suseconds_t; /* microseconds (signed) */
typedef uint16_t uid_t;
typedef uint16_t gid_t;
typedef uint32_t mode_t;
typedef uint16_t nlink_t;
typedef uint16_t ino_t;
typedef uint32_t tid_t;
typedef int32_t blkcnt_t;  /* fs block count */
typedef int32_t blksize_t; /* fs optimal block size */

/* Generic preprocessor macros */
#define __STRING(x) #x
#define __CONCAT1(x, y) x##y
#define __CONCAT(x, y) __CONCAT1(x, y)
#define __UNIQUE(x) __CONCAT(x, __LINE__)

/* Wrapper for various GCC attributes */
#define __nonnull(x) __attribute__((__nonnull__(x)))
#define __section(s) __attribute__((__section__(s)))
#define __unused __attribute__((unused))
#define __used __attribute__((used))
#define __aligned(x) __attribute__((__aligned__(x)))
#define __warn_unused __attribute__((warn_unused_result));
#define __unreachable() __builtin_unreachable()
#define __alias(x) __attribute__((alias(#x)))
#define __cleanup(func) __attribute__((__cleanup__(func)))
#define __caller(x) (__builtin_return_address(x) - 8)
#define __likely(x) __builtin_expect((x), 1)
#define __unlikely(x) __builtin_expect((x), 0)

/* Macros for counting and rounding. */
#ifndef howmany
#define howmany(x, y) (((x) + ((y)-1)) / (y))
#endif
#define nitems(x) (sizeof((x)) / sizeof((x)[0]))
#define rounddown(x, y) (((x) / (y)) * (y))
#define roundup(x, y) ((((x) + ((y)-1)) / (y)) * (y))
#define powerof2(x) ((((x)-1) & (x)) == 0)
#define log2(x) (__builtin_ffs(x) - 1)
#define ffs(x) (size_t)(__builtin_ffs(x))
#define clz(x) (size_t)(__builtin_clz(x))
#define ctz(x) (size_t)(__builtin_ctz(x))

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

#define container_of(p, type, field)                                           \
  ((type *)((char *)(p)-offsetof(type, field)))

#define CLEANUP_FUNCTION(func) __CONCAT(__cleanup_, func)
#define DEFINE_CLEANUP_FUNCTION(type, func)                                    \
  static inline void __cleanup_##func(type *ptr) {                             \
    if (*ptr)                                                                  \
      func(*ptr);                                                              \
  }                                                                            \
  struct __force_semicolon__

#define SCOPED_STMT(TYP, ACQUIRE, RELEASE, VAL, ...)                           \
  TYP *__UNIQUE(__scoped) __cleanup(RELEASE) = ({                              \
    ACQUIRE(VAL, ##__VA_ARGS__);                                               \
    VAL;                                                                       \
  })

#define WITH_STMT(TYP, ACQUIRE, RELEASE, VAL, ...)                             \
  for (SCOPED_STMT(TYP, ACQUIRE, RELEASE, VAL, ##__VA_ARGS__),                 \
       *__UNIQUE(__loop) = (TYP *)1;                                           \
       __UNIQUE(__loop); __UNIQUE(__loop) = NULL)

#ifndef _USERSPACE

/* Write a formatted string to default console. */
int kprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/* Terminate thread. */
noreturn void panic_fail(void);

#define panic(FMT, ...)                                                        \
  __extension__({                                                              \
    kprintf("[%s:%d] PANIC: " FMT "\n", __FILE__, __LINE__, ##__VA_ARGS__);    \
    panic_fail();                                                              \
  })

#ifdef DEBUG
void assert_fail(const char *expr, const char *file, unsigned int line);

#define assert(EXPR)                                                           \
  __extension__({                                                              \
    if (!(EXPR))                                                               \
      assert_fail(__STRING(EXPR), __FILE__, __LINE__);                         \
  })
#else
#define assert(expr)
#endif

#else
#include <assert.h>
#endif /* !_USERSPACE */

#endif /* !_SYS_COMMON_H_ */
