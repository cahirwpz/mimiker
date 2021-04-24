#ifndef _SYS_DTB_
#define _SYS_DTB_

#include <sys/types.h>

void *dtb_root(void);
paddr_t dtb_early_root(void);
void dtb_early_init(paddr_t dtb, size_t size);
void dtb_init(void);

#endif /* !_SYS_DTB_ */
