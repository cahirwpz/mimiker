#ifndef _SYS_KLOG_H_
#define _SYS_KLOG_H_

#include <stdint.h>

/* Kernel log sources */
#define KL_NONE 0x00000000 /* don't log anything */
#define KL_RUNQ 0x00000001 /* basic kernel data structures */
#define KL_SLEEPQ 0x00000002
#define KL_CALLOUT 0x00000004
#define KL_PMAP 0x00000010 /* memory management */
#define KL_VM 0x00000020
#define KL_MALLOC 0x00000040
#define KL_POOL 0x00000080
#define KL_LOCK 0x00000100    /* lock operations tracing */
#define KL_SCHED 0x00000200   /* scheduler tracing */
#define KL_THREAD 0x00000400  /* kernel threads management */
#define KL_INTR 0x00001000    /* interrupts management and handling */
#define KL_DEV 0x00002000     /* device management */
#define KL_VFS 0x00010000     /* vfs operations tracing */
#define KL_VNODE 0x00020000   /* vnode operations tracing */
#define KL_PROC 0x00040000    /* user process management */
#define KL_SYSCALL 0x00080000 /* syscall processing */
#define KL_DEF 0x00100000     /* default mask for undefined subsystems */
#define KL_ALL 0x7fffffff

// Mask for subsystem using klog. If not specified using default mask.
#ifndef SYS_MASK
#define SYS_MASK KL_DEF
#endif

extern volatile int first_message;
extern volatile int last_message;
extern int kl_size;

void klog_append(unsigned mask, const char *file, unsigned line,
                 const char *format, intptr_t arg1, intptr_t arg2,
                 intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6);

// Print all logs on screen.
void klog_dump_all();

// Delete all logs.
void klog_clear();

#define klog_(format, p1, p2, p3, p4, p5, p6, ...)                             \
  do {                                                                         \
    klog_append(SYS_MASK, __FILE__, __LINE__, format, (intptr_t)(p1),          \
                (intptr_t)(p2), (intptr_t)(p3), (intptr_t)(p4),                \
                (intptr_t)(p5), (intptr_t)(p6));                               \
  } while (0)

#define klog(format, ...) klog_((format), __VA_ARGS__, 0, 0, 0, 0, 0, 0)

#define klog_mask_(m, format, p1, p2, p3, p4, p5, p6, ...)                     \
  do {                                                                         \
    klog_append((m), __FILE__, __LINE__, format, (intptr_t)(p1),               \
                (intptr_t)(p2), (intptr_t)(p3), (intptr_t)(p4),                \
                (intptr_t)(p5), (intptr_t)(p6));                               \
  } while (0)

#define klog_mask(m, format, ...)                                              \
  klog_mask_((m), format, __VA_ARGS__, 0, 0, 0, 0, 0, 0)

#endif
