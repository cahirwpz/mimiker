#include <sched.h>
#include <turnstile.h>
#include <sync.h>

void turnstile_wait(turnstile_t* ts) {
  cs_enter();
  struct thread *td = thread_self();
  TAILQ_INSERT_TAIL(&ts->td_queue, td, td_lock);
  td->td_state = TDS_WAITING;
  cs_leave();
  //RACE CONDITION HERE!
  sched_yield(NULL);
}

void turnstile_signal(turnstile_t* ts) {
  cs_enter();
  struct thread *newtd = TAILQ_FIRST(&ts->td_queue);
  if(newtd)
  {
    TAILQ_REMOVE(&ts->td_queue, newtd, td_lock);
    cs_leave();
    sched_switch(newtd);
  }
  else
    cs_leave();
}

static __inline uint32_t
atomic_cmpset_32(__volatile uint32_t* p, uint32_t cmpval, uint32_t newval)
{
    uint32_t ret;

    __asm __volatile (
        "1:\tll %0, %4\n\t"     /* load old value */
        "bne %0, %2, 2f\n\t"        /* compare */
        "move %0, %3\n\t"       /* value to store */
        "sc %0, %1\n\t"         /* attempt to store */
        "beqz %0, 1b\n\t"       /* if it failed, spin */
        "j 3f\n\t"
        "2:\n\t"
        "li %0, 0\n\t"
        "3:\n"
        : "=&r" (ret), "=m" (*p)
        : "r" (cmpval), "r" (newval), "m" (*p)
        : "memory");
    return ret;
}
