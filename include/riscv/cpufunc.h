#ifndef _RISCV_CPUFUNC_H_
#define _RISCV_CPUFUNC_H_

#include <riscv/riscvreg.h>

#define __wfi() __asm__ volatile("wfi")

#define __sfence_vma() __asm__ volatile("sfence.vma" ::: "memory")

#define __sp()                                                                 \
  ({                                                                           \
    uint64_t __rv;                                                             \
    __asm __volatile("mv %0, sp" : "=r"(__rv));                                \
    __rv;                                                                      \
  })

#define __gp()                                                                 \
  ({                                                                           \
    register_t __rv;                                                           \
    __asm __volatile("mv %0, gp" : "=r"(__rv));                                \
    __rv;                                                                      \
  })

#define __set_tp() __asm__ volatile("mv tp, %0" ::"r"(_pcpu_data) : "tp")

#define __set_satp(val) csr_write(satp, val)

#define rdcycle() csr_read64(cycle)
#define rdtime() csr_read64(time)
#define rdinstret() csr_read64(instret)
#define rdhpmcounter(n) csr_read64(hpmcounter##n)

#define enter_user_access() csr_set(sstatus, SSTATUS_SUM)
#define exit_user_access() csr_clear(sstatus, SSTATUS_SUM)

#endif /* !_RISCV_CPUFUNC_H_ */
