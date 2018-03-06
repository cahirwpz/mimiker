#include <spinlock.h>
#include <interrupt.h>
#include <sched.h>
#include <thread.h>

bool spin_owned(spinlock_t *s) {
  return (s->s_owner == thread_self());
}

void spin_init(spinlock_t *s) {
  s->s_owner = NULL;
  s->s_lockpt = NULL;
}

void _spin_acquire(spinlock_t *s, const void *waitpt) {
  intr_disable();

  assert(s->s_owner == NULL);
  s->s_owner = thread_self();
  s->s_lockpt = waitpt;
}

void spin_release(spinlock_t *s) {
  assert(spin_owned(s));
  s->s_owner = NULL;
  s->s_lockpt = NULL;

  intr_enable();
}
