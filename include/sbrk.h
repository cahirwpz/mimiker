#ifndef _SYS_SBRK_H_
#define _SYS_SBRK_H_

typedef struct proc proc_t;

void sbrk_attach(proc_t *p);
vm_addr_t sbrk_resize(proc_t *p, intptr_t increment);

#endif /* !_SYS_SBRK_H_ */
