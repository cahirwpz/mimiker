#ifndef _AARCH64_SIGNAL_H_
#define _AARCH64_SIGNAL_H_

#include <machine/cdefs.h> /* for API selection */

#ifndef __ASSEMBLER__

/*
 * Machine-dependent signal definitions
 */

typedef int sig_atomic_t;

#if defined(_KERNEL)
/* Start and end address of signal trampoline that gets copied onto
 * the user's stack. */
extern char sigcode[];
extern char esigcode[];
#endif /* !_KERNEL */

#endif /* !_ASSEMBLY_ */

#endif /* !_AARCH64_SIGNAL_H_ */
