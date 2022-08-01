#ifndef _RISCV_CPUFUNC_H_
#define _RISCV_CPUFUNC_H_

#include <riscv/riscvreg.h>

#define __wfi() __asm__ volatile("wfi")

#define __fence_i() __asm __volatile("fence.i" ::: "memory")

#define __sfence_vma() __asm__ volatile("sfence.vma" ::: "memory")

#define __sp()                                                                 \
  ({                                                                           \
    register_t __rv;                                                           \
    __asm __volatile("mv %0, sp" : "=r"(__rv));                                \
    __rv;                                                                      \
  })

#define __gp()                                                                 \
  ({                                                                           \
    register_t __rv;                                                           \
    __asm __volatile("mv %0, gp" : "=r"(__rv));                                \
    __rv;                                                                      \
  })

#define __set_tp() __asm__ volatile("mv tp, %0" ::"r"(_pcpu_data))

#define __set_satp(val) csr_write(satp, val)

#define rdcycle() csr_read64(cycle)
#define rdtime() csr_read64(time)
#define rdinstret() csr_read64(instret)
#define rdhpmcounter(n) csr_read64(hpmcounter##n)

#if TRAP_USER_ACCESS
#define enter_user_access() csr_set(sstatus, SSTATUS_SUM)
#define exit_user_access() csr_clear(sstatus, SSTATUS_SUM)
#else
#define enter_user_access()
#define exit_user_access()
#endif /* TRAP_USER_ACCESS */

#endif /* !_RISCV_CPUFUNC_H_ */
