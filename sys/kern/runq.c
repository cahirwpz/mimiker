#include <sys/libkern.h>
#include <sys/mimiker.h>
#include <sys/thread.h>
#include <sys/runq.h>

void runq_init(runq_t *rq) {
  memset(rq, 0, sizeof(*rq));

  mtx_init(&rq->rq_lock, MTX_SPIN);
  for (unsigned i = 0; i < RQ_NQS; i++)
    TAILQ_INIT(&rq->rq_queues[i]);
}

void runq_add(runq_t *rq, thread_t *td) {
  unsigned prio = td->td_prio / RQ_PPQ;
  TAILQ_INSERT_TAIL(&rq->rq_queues[prio], td, td_runq);
  bit_set(rq->rq_status, prio);
}

thread_t *runq_choose(runq_t *rq) {
  int prio;
  bit_ffs(rq->rq_status, RQ_NQS, &index);
  if (prio == -1)
    return NULL;
  thread_list_t *head = &rq->rq_queues[i];
  return TAILQ_FIRST(head);
}

void runq_remove(runq_t *rq, thread_t *td) {
  unsigned prio = td->td_prio / RQ_PPQ;
  bit_clear(rq->rq_status, prio);
  TAILQ_REMOVE(&rq->rq_queues[prio], td, td_runq);
}
