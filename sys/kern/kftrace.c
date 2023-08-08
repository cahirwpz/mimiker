#include <sys/kftrace.h>
#include <sys/thread.h>
#include <sys/types.h>
#include <sys/kmem.h>

/* Kernel Function Trace
 *
 * In this file there are defined structures and functions that are used to
 * implement function tracking in kernel.
 *
 * When entering or exiting a function an event is added to `kft_event_list`.
 * The size of the list is specified by `KFT_EVENT_MAX`.
 *
 * When the list is full the function `kft_flush` is invoked. Currently it only
 * resets the event count. The main purpose of it is to set a breakpoint on it
 * and dump the contents of the list of events to a file by the debugger.
 *
 * Information about the event is compressed into a single 64 bit value.
 * Below is a picture of layout of information encoded in that value.
 *
 * +---------+-----------+-----------+------------+
 * | 24 bits | 31 bits   |   8 bits  |   1 bit    |
 * |    PC   | timestamp | thread id | event type |
 * +---------+-----------+-----------+------------+
 */

typedef uint64_t kft_event_t;

static kft_event_t *kft_event_list;
static unsigned kft_used = 0;

#define PC_MASK 0xFFFFFF          /* 24 bits */
#define TIMESTAMP_MASK 0x7FFFFFFF /* 31 bits */
#define THREAD_MASK 0xF           /* 8 bits */
#define TYPE_MASK 0x1             /* 1 bit */

#define PC_SHIFT 40
#define TIMESTAMP_SHIFT 9
#define THREAD_SHIFT 1
#define TYPE_SHIFT 0

typedef enum kft_event_type {
  KFT_EVENT_FUN_IN,
  KFT_EVENT_FUN_OUT,
} kft_event_type_t;

void init_kftrace(void) {
  kft_event_list =
    kmem_alloc(sizeof(kft_event_t) * (KFT_EVENT_MAX + 5), M_WAITOK);
}

static __no_profile inline kft_event_t make_event(kft_event_type_t type,
                                                  tid_t thread,
                                                  uint64_t timestamp,
                                                  uint64_t pc) {
  return ((type & TYPE_MASK) << TYPE_SHIFT) |
         ((thread & THREAD_MASK) << THREAD_SHIFT) |
         ((timestamp & TIMESTAMP_MASK) << TIMESTAMP_SHIFT) |
         ((pc & PC_MASK) << PC_SHIFT);
}

/* XXX: function for debugger breakpoint. */
static __no_profile void kft_flush(void) {
  /* Free the kft event list because all events are recorded by debugger. */
  kft_used = 0;
}

static __no_profile void add_event(void *pc, kft_event_type_t type) {
  if (!kft_event_list)
    return;

  uint64_t time = kft_get_time();
  tid_t thread = thread_self()->td_tid;

  uintptr_t rel_pc = (uintptr_t)pc - (uintptr_t)__kernel_start;
  kft_event_list[kft_used++] = make_event(type, thread, time, rel_pc);

  /* If buffer is full flush it. */
  if (kft_used >= KFT_EVENT_MAX) {
    kft_flush();
  }
}

__no_profile void __cyg_profile_func_enter(void *this_fn, void *call_site) {
  add_event(this_fn, KFT_EVENT_FUN_IN);
}

__no_profile void __cyg_profile_func_exit(void *this_fn, void *call_site) {
  add_event(this_fn, KFT_EVENT_FUN_OUT);
}
