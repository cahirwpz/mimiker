#include <stdc.h>
#include <mutex.h>
#include <interrupt.h>
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
    assert(m->m_type & MTX_RECURSE);
    m->m_count++;
    return;
  }

  WITH_INTR_DISABLED {
    while (m->m_owner != NULL)
      sleepq_wait(&m->m_owner, waitpt);
    m->m_owner = thread_self();
  }
}

void mtx_unlock(mtx_t *m) {
  assert(mtx_owned(m));

  if (m->m_count > 0) {
    m->m_count--;
    return;
  }

  WITH_INTR_DISABLED {
    m->m_owner = NULL;
    sleepq_signal(&m->m_owner);
  }
}
