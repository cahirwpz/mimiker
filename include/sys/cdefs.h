#ifndef _SYS_COMMON_H_
#define _SYS_COMMON_H_

#include <limits.h>    /* UINT_MAX, LONG_MIN, ... */
#include <stdint.h>    /* uint*_t, int*_t */
#include <stddef.h>    /* offsetof, NULL, ptrdiff_t, size_t, etc. */
#include <stdbool.h>   /* bool, true, false */
#include <stdalign.h>  /* alignof, alignas */
#include <stdatomic.h> /* atomic_{load,store,fetch_*,...} */
#include <inttypes.h>  /* PRIdN, PRIxPTR, ... */
#include <sys/types.h>
#include <machine/cdefs.h>

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;

typedef unsigned long vaddr_t; /* virtual address */
typedef unsigned long paddr_t; /* physical address */

typedef int32_t off_t; /* should it be int64_t? */
typedef intptr_t ssize_t;
typedef int32_t pid_t;
typedef int32_t pgid_t;
typedef uint16_t dev_t;
typedef uint32_t systime_t; /* kept in system clock ticks */
typedef uint16_t uid_t;
typedef uint16_t gid_t;
typedef uint32_t mode_t;
typedef uint16_t nlink_t;
typedef uint16_t ino_t;
typedef uint32_t tid_t;
typedef int32_t blkcnt_t;  /* fs block count */
typedef int32_t blksize_t; /* fs optimal block size */

typedef __builtin_va_list __va_list;

/*
 * To be used when an empty body is required like:
 *
 * #ifdef DEBUG
 * # define dprintf(a) printf(a)
 * #else
 * # define dprintf(a) __nothing
 * #endif
 *
 * We use ((void)0) instead of do {} while (0) so that it
 * works on , expressions.
 */
#define __nothing (/*LINTED*/ (void)0)

#ifndef __BEGIN_DECLS
#ifdef __cplusplus
#define __BEGIN_DECLS extern "C" {
#define __END_DECLS }
#define __static_cast(x, y) static_cast<x>(y)
#else
#define __BEGIN_DECLS
#define __END_DECLS
#define __static_cast(x, y) (x) y
#endif
#endif

#define __dso_public __attribute__((__visibility__("default")))
#define __dso_hidden __attribute__((__visibility__("hidden")))

/* Generic preprocessor macros */
#define __STRING(x) #x
#define __CONCAT1(x, y) x##y
#define __CONCAT(x, y) __CONCAT1(x, y)
#define __UNIQUE(x) __CONCAT(x, __LINE__)

/* Check if source code is compiled with giver GCC version. */
#define __GNUC_PREREQ__(x, y)                                                  \
  ((__GNUC__ == (x) && __GNUC_MINOR__ >= (y)) || (__GNUC__ > (x)))

/* Wrapper for various GCC attributes */
#define __nonnull(x) __attribute__((__nonnull__(x)))
#define __section(s) __attribute__((__section__(s)))
#ifndef __unused
#define __unused __attribute__((__unused__))
#endif
#define __used __attribute__((__used__))
#define __returns_twice __attribute__((__returns_twice__))
#define __noreturn __attribute__((__noreturn__))
#define __aligned(x) __attribute__((__aligned__(x)))
#define __packed __attribute__((__packed__))
#define __warn_unused __attribute__((warn_unused_result));
#define __unreachable() __builtin_unreachable()
#define __pure __attribute__((__pure__))
#define __constfunc __attribute__((__const__))
#define __alias(x) __attribute__((alias(#x)))
#define __cleanup(func) __attribute__((__cleanup__(func)))
#define __caller(x) (__builtin_return_address(x) - 8)
#define __likely(x) __builtin_expect((x), 1)
#define __unlikely(x) __builtin_expect((x), 0)
#define __restrict restrict
#define __long_call __attribute__((long_call))
#define __transparent_union __attribute__((__transparent_union__))
#if __GNUC_PREREQ__(8, 2)
#define __fallthrough __attribute__((fallthrough))
#else
#define __fallthrough
#endif

/*
 * Compiler-dependent macros to declare that functions take printf-like
 * or scanf-like arguments.  They are null except for versions of gcc
 * that are known to support the features properly (old versions of gcc-2
 * didn't permit keeping the keywords out of the application namespace).
 */
#define __printflike(fmtarg, firstvararg)                                      \
  __attribute__((__format__(__printf__, fmtarg, firstvararg)))
#define __scanflike(fmtarg, firstvararg)                                       \
  __attribute__((__format__(__scanf__, fmtarg, firstvararg)))
#define __format_arg(fmtarg) __attribute__((__format_arg__(fmtarg)))

#define __strong_alias(alias, sym)                                             \
  extern __typeof(alias) alias __attribute__((__alias__(#sym)))
#define __weak_alias(alias, sym)                                               \
  __strong_alias(alias, sym) __attribute__((__weak__))

/*
 * The following macro is used to remove const cast-away warnings
 * from gcc -Wcast-qual; it should be used with caution because it
 * can hide valid errors; in particular most valid uses are in
 * situations where the API requires it, not to cast away string
 * constants. We don't use *intptr_t on purpose here and we are
 * explicit about unsigned long so that we don't have additional
 * dependencies.
 */
#define __UNCONST(a) ((void *)(unsigned long)(const void *)(a))

/*
 * The following macro is used to remove the volatile cast-away warnings
 * from gcc -Wcast-qual; as above it should be used with caution
 * because it can hide valid errors or warnings.  Valid uses include
 * making it possible to pass a volatile pointer to memset().
 * For the same reasons as above, we use unsigned long and not intptr_t.
 */
#define __UNVOLATILE(a) ((void *)(unsigned long)(volatile void *)(a))

/*
 * GNU C version 2.96 adds explicit branch prediction so that
 * the CPU back-end can hint the processor and also so that
 * code blocks can be reordered such that the predicted path
 * sees a more linear flow, thus improving cache behavior, etc.
 *
 * The following two macros provide us with a way to use this
 * compiler feature.  Use __predict_true() if you expect the expression
 * to evaluate to true, and __predict_false() if you expect the
 * expression to evaluate to false.
 *
 * A few notes about usage:
 *
 *       * Generally, __predict_false() error condition checks(unless
 *         you have some _strong_ reason to do otherwise, in which case
 *         document it), and/or __predict_true() `no-error' condition
 *         checks, assuming you want to optimize for the no-error case.
 *
 *       * Other than that, if you don't know the likelihood of atest
 *         succeeding from empirical or other `hard' evidence, don't
 *         make predictions.
 *
 *       * These are meant to be used in places that are run `a lot'.
 *         It is wasteful to make predictions in code that is run
 *         seldomly (e.g. at subsystem initialization time) as the
 *         basic block reordering that this affects can often generate
 *         larger code.
 */
#define __predict_true(exp) __builtin_expect((exp) != 0, 1)
#define __predict_false(exp) __builtin_expect((exp) != 0, 0)

/* Attribute macros for boot/wired functions/data */
#define __boot_text __long_call __section(".boot.text")
#define __boot_data __section(".boot.data")
#define __wired_text __section(".wired.text")
#define __wired_data __section(".wired.data")

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
  __unused TYP *__UNIQUE(__scoped) __cleanup(RELEASE) = ({                     \
    ACQUIRE(VAL, ##__VA_ARGS__);                                               \
    VAL;                                                                       \
  })

#define WITH_STMT(TYP, ACQUIRE, RELEASE, VAL, ...)                             \
  for (SCOPED_STMT(TYP, ACQUIRE, RELEASE, VAL, ##__VA_ARGS__),                 \
       *__UNIQUE(__loop) = (TYP *)1;                                           \
       __UNIQUE(__loop); __UNIQUE(__loop) = NULL)

#ifdef _KERNEL

/* Write a formatted string to default console. */
int kprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/* Terminate thread. */
__noreturn void panic_fail(void);

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
#endif /* !_KERNEL */

#endif /* !_SYS_CDEFS_H */
