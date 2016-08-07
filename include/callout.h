#ifndef __CALLOUT_H__
#define __CALLOUT_H__

#include <common.h>
#include "queue.h"
#include "bitset.h"
#include "_bitset.h"

typedef void (*timeout_t)(void *);
typedef int64_t sbintime_t;

typedef struct callout {
  TAILQ_ENTRY(callout) c_link;
  sbintime_t c_time;  /* absolute time of the event */
  timeout_t c_func;  /* function to call */
  void    *c_arg;     /* function argument */
  uint16_t c_flags;
  unsigned int index; /* index of bucket this callout is assigned to */
} callout_t;

#define CALLOUT_ACTIVE      0x0001 /* callout is currently being executed */
#define CALLOUT_PENDING     0x0002 /* callout is waiting for timeout */

#define callout_active(c) (c)->c_flags |= CALLOUT_ACTIVE
#define callout_not_active(c) (c)->c_flags &= ~CALLOUT_ACTIVE

#define callout_pending(c) (c)->c_flags |= CALLOUT_PENDING
#define callout_not_pending(c) (c)->c_flags &= ~CALLOUT_ACTIVE

void callout_init();

/*
  Adds a callout to the queue. After "time" ticks, function "fn" is executed
  with an argument "arg".
*/
void callout_setup(callout_t *handle, sbintime_t time, timeout_t fn, void *arg);

/*
  Delete an event from the queue without executing it.
  The element must not have been executed yet.
*/
void callout_stop(callout_t *handle);

/*
  This function makes a tick and processes all callouts that are supposed to happen.
  If an event is executed, it is deleted from the queue.
*/
void callout_process();

/*
  A demo presenting example usage of this interface.
*/
void callout_demo();


#endif /* __CALLOUT_H__ */
