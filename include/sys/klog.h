#ifndef _SYS_KLOG_H_
#define _SYS_KLOG_H_

#include <sys/types.h>

/* Kernel log message origin. */
typedef enum {
  KL_UNDEF,   /* undefined subsystems */
  KL_RUNQ,    /* scheduler's run queue */
  KL_SLEEPQ,  /* sleep queues */
  KL_CALLOUT, /* callout */
  KL_SIGNAL,  /* signal processing */
  KL_INIT,    /* system initialization */
  KL_PMAP,    /* physical map management */
  KL_PHYSMEM, /* physical memory management */
  KL_VM,      /* virtual memory */
  KL_KMEM,    /* kernel memory allocators (kmem, pool, kmalloc) */
  KL_VMEM,    /* address space allocator */
  KL_LOCK,    /* lock operations tracing */
  KL_SCHED,   /* scheduler tracing */
  KL_TIME,    /* system clock & timers */
  KL_THREAD,  /* kernel threads management */
  KL_INTR,    /* interrupts management and handling */
  KL_DEV,     /* device management */
  KL_USB,     /* usb subsystem */
  KL_VFS,     /* vfs operations tracing */
  KL_VNODE,   /* vnode operations tracing */
  KL_PROC,    /* user process management */
  KL_SYSCALL, /* syscall processing */
  KL_USER,    /* user program */
  KL_TEST,    /* mask for testing purpose */
  KL_FILE,    /* filedesc & file operations */
  KL_FILESYS, /* filesystems */
  KL_TTY,     /* terminal subsystem */
} klog_origin_t;

#define KL_NONE 0x00000000 /* don't log anything */
#define KL_MASK(l) (1 << (l))
#define KL_ALL 0xffffffff /* log everything */

#define KL_DEFAULT_MASK (KL_ALL & (~(KL_MASK(KL_PMAP) | KL_MASK(KL_PHYSMEM))))

/* Mask for subsystem using klog. If not specified using default subsystem. */
#ifndef KL_LOG
#define KL_LOG KL_UNDEF
#endif

#ifdef _KLOG_PRIVATE
#undef _KLOG_PRIVATE

#include <sys/time.h>

#define KL_SIZE 1024

typedef struct klog_entry {
  bintime_t kl_timestamp;
  tid_t kl_tid;
  unsigned kl_line;
  const char *kl_file;
  klog_origin_t kl_origin;
  const char *kl_format;
  uintptr_t kl_params[6];
} klog_entry_t;

typedef struct klog {
  klog_entry_t array[KL_SIZE];
  atomic_uint mask;
  bool verbose;
  volatile unsigned first;
  volatile unsigned last;
  bool repeated;
  int prev;
} klog_t;

extern klog_t klog;
#endif

/*! \brief Called during kernel initialization. */
void init_klog(void);

void klog_append(klog_origin_t origin, const char *file, unsigned line,
                 const char *format, uintptr_t arg1, uintptr_t arg2,
                 uintptr_t arg3, uintptr_t arg4, uintptr_t arg5,
                 uintptr_t arg6);

unsigned klog_setmask(unsigned newmask);

/* Print all logs on screen. */
void klog_dump(void);

/* Delete all logs. */
void klog_clear(void);

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
#define klog_(o, ...) _klog_((o), __VA_ARGS__, 0, 0, 0, 0, 0, 0)

#endif /* !_SYS_KLOG_H_ */
