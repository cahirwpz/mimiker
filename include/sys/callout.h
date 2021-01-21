#ifndef _SYS_CALLOUT_H_
#define _SYS_CALLOUT_H_

#include <stdbool.h>
#include <sys/types.h>
#include <sys/queue.h>

typedef void (*timeout_t)(void *);

typedef struct callout {
  TAILQ_ENTRY(callout) c_link;
  systime_t c_time; /* absolute time of the event */
  timeout_t c_func; /* function to call */
  void *c_arg;      /* function argument */
  uint32_t c_flags;
  unsigned c_index; /* index of bucket this callout is assigned to */
} callout_t;

/* callout has been delegated to callout thread and will be executed soon */
#define CALLOUT_ACTIVE 0x0001
#define CALLOUT_PENDING 0x0002 /* callout is waiting for timeout */

/*! \brief Called during kernel initialization. */
void init_callout(void);

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
 * Cancel a callout if it is currently pending.
 *
 * \return True if the callout was pending and has been stopped, false if the
 * callout has already been delegated to callout thread or executed.
 *
 * \warning It's not safe to deallocate callout memory after it has been
 * stopped. You should use \a callout_drain if you need that.
 */
bool callout_stop(callout_t *handle);

/*
 * Process all callouts that happened since last time and delegate them to
 * callout thread.
 */
void callout_process(systime_t now);

/*
 * Wait until a callout ends its execution or return immediately if the
 * callout has already been executed or stopped.
 *
 * \return True if the function call blocked and waited for callout execution,
 * or false if the function returned immediately.
 */
bool callout_drain(callout_t *handle);

#endif /* !_SYS_CALLOUT_H_ */
