#include <stdc.h>
#include <sched.h>
#include <turnstile.h>
#include <sync.h>

//TODO: MOVE THIS TO OTHER FILE
#define __MIPS_PLATFORM_SYNC_NOPS ""
static __inline  void
mips_sync(void)
{
    __asm __volatile (".set noreorder\n"
            "\tsync\n"
            __MIPS_PLATFORM_SYNC_NOPS
            ".set reorder\n"
            : : : "memory");
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

/*
 * Atomically compare the value stored at *p with cmpval and if the
 * two values are equal, update the value of *p with newval. Returns
 * zero if the compare failed, nonzero otherwise.
 */
static __inline uint32_t
atomic_cmpset_acq_32(__volatile uint32_t *p, uint32_t cmpval, uint32_t newval)
{
    int retval;
    retval = atomic_cmpset_32(p, cmpval, newval);
    mips_sync();
    return (retval);
}

static __inline void
atomic_set_32(__volatile uint32_t *p, uint32_t v)
{
    uint32_t temp;

    __asm __volatile (
        "1:\tll %0, %3\n\t"     /* load old value */
        "or %0, %2, %0\n\t"     /* calculate new value */
        "sc %0, %1\n\t"     /* attempt to store */
        "beqz   %0, 1b\n\t"     /* spin if failed */
        : "=&r" (temp), "=m" (*p)
        : "r" (v), "m" (*p)
        : "memory");

}

static __inline  void                   \
atomic_store_rel_32(__volatile uint32_t *p, uint32_t v)
{                           
    mips_sync();                    
    *p = v;                     
}
//TODO: end here


void turnstile_wait(turnstile_t* ts) {
  cs_enter();
  struct thread *td = thread_self();
  TAILQ_INSERT_TAIL(&ts->td_queue, td, td_lock);
  td->td_state = TDS_WAITING;
  cs_leave();
  sched_yield();
}


void turnstile_dump(turnstile_t* ts)
{
    thread_t* it;
    kprintf("Blocked on turnstile: \n");
    TAILQ_FOREACH(it, &ts->td_queue, td_lock)
    {
        kprintf("%s\n", it->td_name);
        if(TAILQ_NEXT(it, td_lock) == it)
            panic("Broken list!");
    }
    kprintf("\n");
}

void mutex_dump(mtx_sleep_t *mtx)
{
    turnstile_dump(&mtx->turnstile);
}

void turnstile_signal(turnstile_t* ts) {
  cs_enter();
  struct thread *newtd = TAILQ_FIRST(&ts->td_queue);
  if(newtd)
  {
    TAILQ_REMOVE(&ts->td_queue, newtd, td_lock);
    sched_add(newtd);
  }
  cs_leave();
}

int mtx_owned(mtx_sleep_t *mtx)
{
    return mtx->mtx_state == (uint32_t)thread_self();
}

int mtx_sleep_try_to_lock(mtx_sleep_t *mtx)
{
  return atomic_cmpset_acq_32(&mtx->mtx_state, MTX_UNOWNED, (uint32_t)thread_self());
}

void mtx_sleep_init(mtx_sleep_t *mtx)
{
    mtx->mtx_state = MTX_UNOWNED;
    turnstile_init(&mtx->turnstile);
}

void turnstile_init(turnstile_t* turnstile)
{
  TAILQ_INIT(&turnstile->td_queue);
}

void mtx_sleep_lock(mtx_sleep_t *mtx)
{
  assert(!mtx_owned(mtx)); //No recursive mutexes for now
  while(!mtx_sleep_try_to_lock(mtx))
  {
    turnstile_wait(&mtx->turnstile);
  }
}

void mtx_sleep_unlock(mtx_sleep_t *mtx)
{
  atomic_store_rel_32(&mtx->mtx_state, MTX_UNOWNED);
  turnstile_signal(&mtx->turnstile);
}

