#include <atomic.h>

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

int atomic_cmp_exchange(__volatile uint32_t *p, uint32_t cmpval, uint32_t newval)
{
    return atomic_cmpset_acq_32(p, cmpval, newval);
}

void atomic_store(__volatile uint32_t *p, uint32_t val)
{
    atomic_set_32(p, val);
}

