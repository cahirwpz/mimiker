#include <stdc.h>
#include <common.h>
#include <thread.h>
#include <runq.h>

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
  for (int i = RQ_NQS - 1; i >= 0; i--) {
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

#ifdef _USERSPACE
int main(void) {
  thread_t t1;
  t1.td_priority = 3 * RQ_PPQ;
  thread_t t2;
  t2.td_priority = 4 * RQ_PPQ;
  thread_t t3;
  t3.td_priority = 1 * RQ_PPQ;
  thread_t t4;
  t4.td_priority = 5 * RQ_PPQ;

  runq_t runq;

  runq_init(&runq);

  runq_add(&runq, &t1);
  assert(runq_choose(&runq) == &t1);

  runq_add(&runq, &t2);
  assert(runq_choose(&runq) == &t2);

  runq_add(&runq, &t3);
  assert(runq_choose(&runq) == &t2);

  runq_add(&runq, &t4);
  assert(runq_choose(&runq) == &t4);

  runq_remove(&runq, &t4);
  assert(runq_choose(&runq) == &t2);

  runq_remove(&runq, &t3);
  assert(runq_choose(&runq) == &t2);

  runq_remove(&runq, &t2);
  assert(runq_choose(&runq) == &t1);

  runq_remove(&runq, &t1);
  assert(runq_choose(&runq) == NULL);

  return 0;
}
#endif // _USERSPACE
