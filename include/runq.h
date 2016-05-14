#ifndef __RUNQ_H__
#define __RUNQ_H__

#include <common.h>

typedef struct thread thread_t;

#define RQ_NQS 64  /* Number of run queues. */
#define RQ_PPQ 4   /* Priorities per queue. */

TAILQ_HEAD(rq_head, thread);

typedef struct runq {
  struct rq_head rq_queues[RQ_NQS];
} runq_t;

/* Initialize a run structure. */
void runq_init(runq_t *);

/* Add the thread to the queue specified by its priority */
void runq_add(runq_t *, thread_t *);

/* Find the highest priority process on the run queue. */
thread_t *runq_choose(runq_t *);

/* Remove the thread from the queue specified by its priority. */
void runq_remove(runq_t *, thread_t *);

#endif
