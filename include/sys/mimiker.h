#ifndef _SYS_MIMIKER_H_
#define _SYS_MIMIKER_H_

/* Common definitions that may be used only in kerne source tree. */

#ifndef _KERNEL
#error "<sys/mimiker.h> may be used only inside kernel source tree!"
#endif

#include <sys/cdefs.h>

/* Attribute macros for boot/wired functions/data */
#define __boot_text __long_call __section(".boot.text")
#define __boot_data __section(".boot.data")
#define __wired_text __section(".wired.text")
#define __wired_data __section(".wired.data")

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

int copystr(const void *restrict kfaddr, void *restrict kdaddr, size_t len,
            size_t *restrict lencopied) __nonnull(1) __nonnull(2);
int copyinstr(const void *restrict udaddr, void *restrict kaddr, size_t len,
              size_t *restrict lencopied) __nonnull(1) __nonnull(2);
int copyin(const void *restrict udaddr, void *restrict kaddr, size_t len)
  __nonnull(1) __nonnull(2);
int copyout(const void *restrict kaddr, void *restrict udaddr, size_t len)
  __nonnull(1) __nonnull(2);

#define copyin_s(udaddr, _what) copyin((udaddr), &(_what), sizeof(_what))
#define copyout_s(_what, udaddr) copyout(&(_what), (udaddr), sizeof(_what))

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

#endif /* !_SYS_MIMIKER_H_ */
