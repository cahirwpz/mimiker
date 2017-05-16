#ifndef _SYS_KLOG_H_
#define _SYS_KLOG_H_

#include <common.h>

/* Kernel log message origin. */
typedef enum {
  KL_UNDEF,   /* undefined subsystems */
  KL_RUNQ,    /* scheduler's run queue */
  KL_SLEEPQ,  /* sleep queues */
  KL_CALLOUT, /* callout */
  KL_SIGNAL,  /* signal processing */
  KL_INIT,    /* system initialization */
  KL_PMAP,    /* physical map management */
  KL_VM,      /* virtual memory */
  KL_KMEM,    /* generick kernel memory allocator */
  KL_POOL,    /* pooled allocator */
  KL_LOCK,    /* lock operations tracing */
  KL_SCHED,   /* scheduler tracing */
  KL_THREAD,  /* kernel threads management */
  KL_INTR,    /* interrupts management and handling */
  KL_DEV,     /* device management */
  KL_VFS,     /* vfs operations tracing */
  KL_VNODE,   /* vnode operations tracing */
  KL_PROC,    /* user process management */
  KL_SYSCALL, /* syscall processing */
  KL_USER,    /* user program */
  KL_TEST,    /* mask for testing purpose */
} klog_origin_t;

#define KL_NONE 0x00000000 /* don't log anything */
#define KL_MASK(l) (1 << (l))
#define KL_ALL 0xffffffff /* log everything */

/* Mask for subsystem using klog. If not specified using default subsystem. */
#ifndef KL_LOG
#define KL_LOG KL_UNDEF
#endif

#ifdef _KLOG_PRIVATE
#undef _KLOG_PRIVATE

#include <clock.h>

#define KL_SIZE 1024

typedef struct {
  realtime_t kl_timestamp;
  unsigned kl_line;
  const char *kl_file;
  klog_origin_t kl_origin;
  const char *kl_format;
  intptr_t kl_params[6];
} klog_entry_t;

typedef struct {
  klog_entry_t array[KL_SIZE];
  unsigned mask;
  bool verbose;
  volatile unsigned first;
  volatile unsigned last;
} klog_t;

extern klog_t klog;
#endif

void klog_init();

void klog_append(klog_origin_t origin, const char *file, unsigned line,
                 const char *format, intptr_t arg1, intptr_t arg2,
                 intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6);

/* Print all logs on screen. */
void klog_dump();

/* Delete all logs. */
void klog_clear();

#define _klog(format, p1, p2, p3, p4, p5, p6, ...)                             \
  do {                                                                         \
    klog_append(KL_LOG, __FILE__, __LINE__, format, (intptr_t)(p1),            \
                (intptr_t)(p2), (intptr_t)(p3), (intptr_t)(p4),                \
                (intptr_t)(p5), (intptr_t)(p6));                               \
  } while (0)
#define klog(...) _klog(__VA_ARGS__, 0, 0, 0, 0, 0, 0)

#define _klog_(o, format, p1, p2, p3, p4, p5, p6, ...)                         \
  do {                                                                         \
    klog_append((o), __FILE__, __LINE__, format, (intptr_t)(p1),               \
                (intptr_t)(p2), (intptr_t)(p3), (intptr_t)(p4),                \
                (intptr_t)(p5), (intptr_t)(p6));                               \
  } while (0)
#define klog_(o, ...) klog_((o), __VA_ARGS__, 0, 0, 0, 0, 0, 0)

#endif
