#ifndef _SYS_KMEM_H_
#define _SYS_KMEM_H_

#include <sys/kmem_flags.h>

void kmem_bootstrap(void);
void *kmem_alloc(size_t size, unsigned flags);
void kmem_free(void *ptr);

#endif /* !_SYS_KMEM_H_ */
