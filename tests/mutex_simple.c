#include <stdc.h>
#include <sched.h>
#include <mutex.h>
#include <thread.h>
#include <ktest.h>

static mtx_t mtx1 = MTX_INITIALIZER(MTX_DEF);
// static mtx_t mtx2 = MTX_INITIALIZER(MTX_DEF);
// static mtx_t mtx3 = MTX_INITIALIZER(MTX_DEF);

static thread_t *t1;
// static thread_t* t2;

static int s1 = 0;
// static int s2 = 0;

/* how about:

WITH_NO_PREEMPTION {
  s1 = 1;
  mtx_lock(&mtx1);
}
WITH_NO_PREEMPTION {
  s2 = 2;
  mtx_lock(&mtx2);
}
...

 */

static void mtxt_routine1(void *arg) {
  WITH_NO_PREEMPTION {
    s1 = 1;
    mtx_lock(&mtx1);
  }
  WITH_NO_PREEMPTION {
    s1 = 2;
    mtx_unlock(&mtx1);
  }
  s1 = 3;
}

/*
static void mtxt_routine2(void *arg) {
  WITH_NO_PREEMPTION {
    s2 = 1;
    mtx_lock(&mtx2);
  }
  WITH_NO_PREEMPTION {
    s2 = 2;
    mtx_lock(&mtx3);
  }
  mtx_unlock(&mtx3);

  mtx_unlock(&mtx2);
  s2 = 3;
}
*/

static int mtx_test_simple(void) {
  t1 = thread_create("td1", mtxt_routine1, NULL);
  // t2 = thread_create("td2", mtxt_routine2, NULL);

  mtx_lock(&mtx1);

  sched_add(t1);
  while (s1 != 1) {
    thread_yield();
  }

  assert(s1 == 1);
  mtx_unlock(&mtx1);

  thread_join(t1);
  assert(s1 == 3);
  // NOTE this assert is pretty implementation-specific
  // some changes might make it invalid
  assert(mtx1.m_owner == NULL);

  return KTEST_SUCCESS;
}

KTEST_ADD(mutex_simple, mtx_test_simple, 0);
