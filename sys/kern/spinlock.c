#include <sys/mimiker.h>
#include <sys/spinlock.h>
#include <sys/interrupt.h>
#include <sys/sched.h>
#include <sys/thread.h>

#define spin_recurse_p(s) lk_attr_recursive((s)->s_attrs)

bool spin_owned(spin_t *s) {
  return (s->s_owner == thread_self());
}

void spin_init(spin_t *s, lk_attr_t attrs) {
  /* The caller must not attempt to set the lock's type, only flags. */
  assert(lk_attr_type(attrs) == 0);
  s->s_owner = NULL;
  s->s_count = 0;
  s->s_lockpt = NULL;
  s->s_attrs = attrs | LK_TYPE_SPIN;
}

void _spin_lock(spin_t *s, const void *waitpt) {
  intr_disable();

  if (spin_owned(s)) {
    if (!spin_recurse_p(s))
      panic("Spin lock %p is not recursive!", s);
    s->s_count++;
    return;
  }

  assert(s->s_owner == NULL);
  s->s_owner = thread_self();
  s->s_lockpt = waitpt;
}

void spin_unlock(spin_t *s) {
  assert(spin_owned(s));

  if (s->s_count > 0) {
    assert(spin_recurse_p(s));
    s->s_count--;
  } else {
    s->s_owner = NULL;
    s->s_lockpt = NULL;
  }

  intr_enable();
}
