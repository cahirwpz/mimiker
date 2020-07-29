#ifndef _AARCH64_AARCH64_H_
#define _AARCH64_AARCH64_H_

#ifndef _MACHDEP
#error "Do not use this header file outside kernel machine dependent code!"
#endif

#ifndef __ASSEMBLER__

#define AARCH64_UPPERADDR 0xffffffff00000000UL
#define AARCH64_PHYSADDR(x) ((x) - (AARCH64_UPPERADDR))

extern char _ebase[];
extern char __boot[];
extern char __text[];
extern char __data[];
extern char __bss[];
extern char __ebss[];

#endif /* __ASSEMBLER__ */

#endif /* !_AARCH64_AARCH64_H */
