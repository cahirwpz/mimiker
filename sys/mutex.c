#include <mutex.h>
#include <interrupt.h>
#include <sched.h>
#include <thread.h>

bool mtx_owned(mtx_t *m) {
  return (m->m_owner == thread_self());
}

void mtx_init(mtx_t *m, unsigned type) {
  m->m_owner = NULL;
  m->m_count = 0;
  m->m_type = type;
}

void mtx_lock(mtx_t *m) {
  _mtx_lock(m, __caller(0));
}

void _mtx_lock(mtx_t *m, const void *waitpt) {
  if (mtx_owned(m)) {
    /* MTX_RECURSE and MTX_SPIN are recursive locks! */
    assert(m->m_type != MTX_DEF);
    m->m_count++;
    return;
  }

  if (m->m_type == MTX_SPIN) {
    intr_disable();
    assert(m->m_owner == NULL);
    m->m_owner = thread_self();
    m->m_lockpt = waitpt;
  } else {
    WITH_NO_PREEMPTION {
      while (m->m_owner != NULL)
        sleepq_wait(&m->m_owner, waitpt);
      m->m_owner = thread_self();
      m->m_lockpt = waitpt;
    }
  }
}

void mtx_unlock(mtx_t *m) {
  assert(mtx_owned(m));

  if (m->m_count > 0) {
    m->m_count--;
    return;
  }

  if (m->m_type == MTX_SPIN) {
    m->m_owner = NULL;
    m->m_lockpt = NULL;
    intr_enable();
  } else {
    WITH_NO_PREEMPTION {
      m->m_owner = NULL;
      m->m_lockpt = NULL;
      sleepq_signal(&m->m_owner);
    }
  }
}
