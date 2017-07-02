#include <stdc.h>
#include <sync.h>
#include <mutex.h>
#include <sync.h>
#include <thread.h>

#define MTX_UNOWNED 0

bool mtx_owned(mtx_t *m) {
  return (m->m_owner == thread_self());
}

void mtx_init(mtx_t *m, unsigned type) {
  m->m_owner = NULL;
  m->m_count = 0;
  m->m_type = type;
}

void mtx_lock(mtx_t *m) {
  if (mtx_owned(m)) {
    assert(m->m_type & MTX_RECURSE);
    m->m_count++;
    return;
  }

  SCOPED_CRITICAL_SECTION();

  while (m->m_owner != NULL)
    sleepq_wait(&m->m_owner, "mutex");
  m->m_owner = thread_self();
}

void mtx_unlock(mtx_t *m) {
  assert(mtx_owned(m));

  if (m->m_count > 0) {
    m->m_count--;
    return;
  }

  SCOPED_CRITICAL_SECTION();

  m->m_owner = NULL;
  sleepq_signal(&m->m_owner);
}
