#ifndef _SYS_RUNQ_H_
#define _SYS_RUNQ_H_

#ifdef _KERNEL

#include <bitstring.h>
#include <sys/cdefs.h>
#include <sys/queue.h>

typedef struct thread thread_t;
typedef TAILQ_HEAD(, thread) thread_list_t;

#define RQ_NQS 64 /* Number of run queues. */
#define RQ_PPQ 4  /* Priorities per queue. */

typedef struct runq {
  bitstr_t bit_decl(rq_status, RQ_NQS);
  thread_list_t rq_queues[RQ_NQS];
} runq_t;

/* Initialize a run structure. */
void runq_init(runq_t *);

/* Add the thread to the queue specified by its priority */
void runq_add(runq_t *, thread_t *);

/* Find the highest priority process on the run queue. */
thread_t *runq_choose(runq_t *);

/* Remove the thread from the queue specified by its priority. */
void runq_remove(runq_t *, thread_t *);

#endif /* !_KERNEL */

#endif /* !_SYS_RUNQ_H_ */
