#include <atomic.h>

static inline void mips_sync(void) {
  asm volatile(".set noreorder\n"
               "\tsync\n"
               ".set reorder\n" ::
                 : "memory");
}

static inline uint32_t atomic_cmpset_32(volatile uint32_t *p, uint32_t cmpval,
                                        uint32_t newval) {
  uint32_t ret;

  asm volatile("1:\tll %0, %4\n\t"  /* load old value */
               "bne %0, %2, 2f\n\t" /* compare */
               "move %0, %3\n\t"    /* value to store */
               "sc %0, %1\n\t"      /* attempt to store */
               "beqz %0, 1b\n\t"    /* if it failed, spin */
               "j 3f\n\t"
               "2:\n\t"
               "li %0, 0\n\t"
               "3:\n"
               : "=&r"(ret), "=m"(*p)
               : "r"(cmpval), "r"(newval), "m"(*p)
               : "memory");
  return ret;
}

/*
 * Atomically compare the value stored at *p with cmpval and if the
 * two values are equal, update the value of *p with newval. Returns
 * zero if the compare failed, nonzero otherwise.
 */
static inline uint32_t atomic_cmpset_acq_32(volatile uint32_t *p,
                                            uint32_t cmpval, uint32_t newval) {
  int retval;
  retval = atomic_cmpset_32(p, cmpval, newval);
  mips_sync();
  return (retval);
}

static inline void atomic_store_rel_32(volatile uint32_t *p, uint32_t v) {
  mips_sync();
  *p = v;
}

int atomic_cmp_exchange(volatile uint32_t *p, uint32_t cmpval,
                        uint32_t newval) {
  return atomic_cmpset_acq_32(p, cmpval, newval);
}

void atomic_store(volatile uint32_t *p, uint32_t val) {
  atomic_store_rel_32(p, val);
}
