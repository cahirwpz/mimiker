#ifndef _SYS_BOOT_H_
#define _SYS_BOOT_H_

#ifndef _MACHDEP
#error "Do not use this header file outside kernel machine dependent code!"
#endif

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/mimiker.h>

/* Attribute macros for boot/wired functions/data */
#define __boot_text __long_call __section(".boot.text")
#define __boot_data __section(".boot.data")

__boot_text void boot_sbrk_init(paddr_t ebss_pa);
__boot_text void *boot_sbrk(size_t n);
__boot_text paddr_t boot_sbrk_align(size_t n);

__boot_text paddr_t boot_save_dtb(paddr_t dtb);

__boot_text __noreturn void halt(void);

/* Last physical address used by kernel for boot memory allocation. */
extern __boot_data void *_bootmem_end;

#endif /* !_SYS_BOOT_H_ */
