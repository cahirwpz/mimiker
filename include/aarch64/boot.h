#ifndef _AARCH64_BOOT_H_
#define _AARCH64_BOOT_H_

#ifndef _MACHDEP
#error "Do not use this header file outside kernel machine dependent code!"
#endif

#include <sys/cdefs.h>

extern char __boot[];
extern char __text[];
extern char __data[];
extern char __bss[];
extern char __ebss[];

extern __boot_data void *_bootmem_end;

#endif /* !_AARCH64_BOOT_H_ */
