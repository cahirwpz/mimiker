#ifndef _ARM64_SYSCALL_H_
#define _ARM64_SYSCALL_H_

#ifdef __ASSEMBLER__

/* Do a syscall that is missing an implementation. */
#define SYSCALL_MISSING(name)

/* Do a syscall that cannot fail. */
#define SYSCALL_NOERROR(name, num)

/* Do a normal syscall. */
#define SYSCALL(name, num)

#endif /* !__ASSEMBLER__ */

#endif /* !_ARM64_SYSCALL_H_ */
