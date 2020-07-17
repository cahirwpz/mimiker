#ifndef _ARM64_SIGNAL_H_
#define _ARM64_SIGNAL_H_

#include <machine/cdefs.h> /* for API selection */

#ifndef __ASSEMBLER__

/*
 * Machine-dependent signal definitions
 */

typedef int sig_atomic_t;

/*
 * Information pushed on stack when a signal is delivered.
 * This is used by the kernel to restore state following
 * execution of the signal handler.  It is also made available
 * to the handler to allow it to restore state properly if
 * a non-standard exit is performed.
 */
#if defined(_LIBC) || defined(_KERNEL)
struct sigcontext {
  int sc_onstack;   /* sigstack state to restore */
  sigset_t sc_mask; /* signal mask to restore */
  int sc_pc;        /* pc at time of signal */
  int sc_regs[32];  /* processor regs 0 to 31 */
  int sc_mullo;
  int sc_mulhi;      /* mullo and mulhi registers... */
  int sc_fpused;     /* fp has been used */
  int sc_fpregs[33]; /* fp regs 0 to 31 and csr */
  int sc_fpc_eir;    /* floating point exception instruction reg */
};
#endif /* _LIBC || _KERNEL */

#if defined(_KERNEL)
/* Start and end address of signal trampoline that gets copied onto
 * the user's stack. */
extern char sigcode[];
extern char esigcode[];
#endif /* !_KERNEL */

#endif /* !_ASSEMBLY_ */

#endif /* !_ARM64_SIGNAL_H_ */
