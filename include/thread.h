#ifndef __THREAD_H__
#define __THREAD_H__

#include <common.h>
#include <queue.h>
#include <context.h>
#include <exception.h>

typedef uint8_t td_prio_t;
typedef struct vm_page vm_page_t;
typedef struct sleepq sleepq_t;
typedef struct vm_map vm_map_t;

#define TDF_SLICEEND 0x00000001   /* run out of time slice */
#define TDF_NEEDSWITCH 0x00000002 /* must switch on next opportunity */

typedef struct thread {
  TAILQ_ENTRY(thread) td_runq;   /* a link on run queue */
  TAILQ_ENTRY(thread) td_sleepq; /* a link on sleep queue */
  const char *td_name;
  /* thread state */
  enum { TDS_INACTIVE = 0x0, TDS_WAITING, TDS_READY, TDS_RUNNING } td_state;
  uint32_t td_flags;           /* TDF_* flags */
  volatile uint32_t td_csnest; /* critical section nest level */
  /* thread context */
  exc_frame_t td_uctx;    /* user context (always exception) */
  fpu_ctx_t td_uctx_fpu;  /* user FPU context (always exception) */
  exc_frame_t *td_kframe; /* kernel context (last exception frame) */
  ctx_t td_kctx;          /* kernel context (switch) */
  vm_page_t *td_kstack_obj;
  stack_t td_kstack;
  vm_map_t *td_uspace;    /* thread's user space map */
  /* waiting channel */
  sleepq_t *td_sleepqueue;
  void *td_wchan;
  const char *td_wmesg;
  /* scheduler part */
  td_prio_t td_prio;
  int td_slice;
} thread_t;

thread_t *thread_self();
noreturn void thread_init(void (*fn)(), int n, ...);
thread_t *thread_create(const char *name, void (*fn)());
void thread_delete(thread_t *td);

void thread_switch_to(thread_t *td_ready);

#endif // __THREAD_H__
