#include <sys/libkern.h>
#include <sys/mimiker.h>
#include <sys/thread.h>
#include <sys/runq.h>

void runq_init(runq_t *rq) {
  memset(rq, 0, sizeof(*rq));

  for (unsigned i = 0; i < RQ_NQS; i++)
    TAILQ_INIT(&rq->rq_queues[i]);
}

void runq_add(runq_t *rq, thread_t *td) {
  unsigned prio = td->td_prio / RQ_PPQ;
  TAILQ_INSERT_TAIL(&rq->rq_queues[prio], td, td_runq);
}

thread_t *runq_choose(runq_t *rq) {
  for (int i = 0; i < RQ_NQS; i++) {
    struct rq_head *head = &rq->rq_queues[i];
    thread_t *td = TAILQ_FIRST(head);

    if (td)
      return td;
  }

  return NULL;
}

void runq_remove(runq_t *rq, thread_t *td) {
  unsigned prio = td->td_prio / RQ_PPQ;
  TAILQ_REMOVE(&rq->rq_queues[prio], td, td_runq);
}
