#include <sys/thread.h>
#include <sys/types.h>
#include <machine/kft.h>

typedef uint64_t kft_event_t;

#define KFT_MAX 1000000
kft_event_t kft_event_list[KFT_MAX];
static unsigned kft_used = 0;

static void kft_flush(void);

#define PC_MASK 0xFFFFFF          /* 24 bits */
#define TIMESTAMP_MASK 0x7FFFFFFF /* 31 bits */
#define THREAD_MASK 0xF           /* 8 bits */
#define TYPE_MASK 0x1             /* 1 bit */

#define PC_SHIFT 40
#define TIMESTAMP_SHIFT 9
#define THREAD_SHIFT 1
#define TYPE_SHIFT 0

#define KFT_EVENT_FUN_IN 0x0
#define KFT_EVENT_FUN_OUT 0x1

static __no_profile kft_event_t make_event(uint8_t type, tid_t thread,
                                           uint64_t timestamp, uint64_t pc) {
  return ((type & TYPE_MASK) << TYPE_SHIFT) |
         ((thread & THREAD_MASK) << THREAD_SHIFT) |
         ((timestamp & TIMESTAMP_MASK) << TIMESTAMP_SHIFT) |
         ((pc & PC_MASK) << PC_SHIFT);
}

static __no_profile void add_event(uint8_t type, uintptr_t pc) {
  uint64_t time = kft_get_time();
  tid_t thread = thread_self()->td_tid;

  uint64_t rel_pc = (uint64_t)((char *)pc - __kernel_start);
  kft_event_list[kft_used++] = make_event(type, thread, time, rel_pc);

  /* If buffer is full flush it. */
  if (kft_used >= KFT_MAX) {
    kft_flush();
  }
}

__no_profile void __cyg_profile_func_enter(void *this_fn, void *call_site) {
  add_event(KFT_EVENT_FUN_IN, (uintptr_t)this_fn);
}

__no_profile void __cyg_profile_func_exit(void *this_fn, void *call_site) {
  add_event(KFT_EVENT_FUN_OUT, (uintptr_t)this_fn);
}

static __no_profile void kft_flush(void) {
  /* GDB will break on this funciton and dump contents of the thread kft event
   * list. */

  /* Free the kft event list because all events are recorded. */
  kft_used = 0;
}
