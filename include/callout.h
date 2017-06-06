#ifndef _SYS_CALLOUT_H_
#define _SYS_CALLOUT_H_

#include <common.h>
#include <queue.h>
#include <time.h>

typedef void (*timeout_t)(void *);

typedef struct callout {
  TAILQ_ENTRY(callout) c_link;
  systime_t c_time; /* absolute time of the event */
  timeout_t c_func; /* function to call */
  void *c_arg;      /* function argument */
  uint32_t c_flags;
  unsigned c_index; /* index of bucket this callout is assigned to */
} callout_t;

#define CALLOUT_ACTIVE 0x0001  /* callout is currently being executed */
#define CALLOUT_PENDING 0x0002 /* callout is waiting for timeout */

/*
 * Add a callout to the queue.
 * At tick @time function @fn is called with argument @arg.
 */
void callout_setup(callout_t *handle, systime_t time, timeout_t fn, void *arg);

/*
 * Add a callout to the queue, using timing relative to current time.
 * After ticks @time passed the function @fn is called with argument @arg.
 */
void callout_setup_relative(callout_t *handle, systime_t time, timeout_t fn,
                            void *arg);

/*
 * Delete a callout from the queue without handling it.
 */
void callout_stop(callout_t *handle);

/*
 * Process all callouts that happened since last time.
 * A callout is removed from the queue when handled.
 */
void callout_process(systime_t now);

#endif /* !_SYS_CALLOUT_H_ */
