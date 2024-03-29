#ifndef _SYS_PMAP_H_
#define _SYS_PMAP_H_

#include <stdbool.h>
#include <sys/vm.h>

typedef struct ctx ctx_t;
typedef struct pmap pmap_t;

/*
 * Flags passed to `pmap_kenter` and `pmap_enter`.
 *
 * PMAP flags layout:
 *
 *    31  ...  24 23  ...  0
 *      MD flags    MI flags
 *
 * NOTE: permissions are passed using `vm_prot_t`.
 */

/* Machine-independent flags */
#define PMAP_NOCACHE 1
#define PMAP_WRITE_THROUGH 2
#define PMAP_WRITE_BACK 4

/* Machine-dependent flags */
#define _PMAP_KERNEL (1 << 24)

__long_call void pmap_bootstrap(vaddr_t vma_end, paddr_t pd_pa, void *pd);
void init_pmap(void);

vaddr_t pmap_start(pmap_t *pmap);
vaddr_t pmap_end(pmap_t *pmap);

pmap_t *pmap_new(void);
void pmap_delete(pmap_t *pmap);

void pmap_enter(pmap_t *pmap, vaddr_t va, vm_page_t *pg, vm_prot_t prot,
                unsigned flags);
bool pmap_extract(pmap_t *pmap, vaddr_t va, paddr_t *pap);
void pmap_remove(pmap_t *pmap, vaddr_t start, vaddr_t end);

void pmap_kenter(vaddr_t va, paddr_t pa, vm_prot_t prot, unsigned flags);
bool pmap_kextract(vaddr_t va, paddr_t *pap);
void pmap_kremove(vaddr_t va, size_t size);

void pmap_protect(pmap_t *pmap, vaddr_t start, vaddr_t end, vm_prot_t prot);
void pmap_page_remove(vm_page_t *pg);

void pmap_zero_page(vm_page_t *pg);
void pmap_copy_page(vm_page_t *src, vm_page_t *dst);

bool pmap_clear_modified(vm_page_t *pg);
bool pmap_clear_referenced(vm_page_t *pg);
bool pmap_is_modified(vm_page_t *pg);
bool pmap_is_referenced(vm_page_t *pg);
void pmap_set_referenced(vm_page_t *pg);
void pmap_set_modified(vm_page_t *pg);

void pmap_activate(pmap_t *pmap);

pmap_t *pmap_kernel(void);
pmap_t *pmap_user(void);

/*
 * Handle page fault caused by access to `vaddr` with permissions `access`.
 *
 * Returns:
 *  - 0: on success, indicates that the faulting instruction should be run again
 *      (for user-space) or the on-fault has been triggered (for kernel-space)
 *  - `EINVAL`: if page fault has been caused by access to kernel memory
 *  - `EACCES`: there were insufficient permissions to access the page
 *  - `EFAULT`: indicates that the page was not mapped to any object
 *
 * TODO(cahir): what about mmap(2) and SIGBUS?
 *
 * When a page fault was triggered by kernel space and non-zero is returned,
 * kernel will panic!
 */
int pmap_fault_handler(ctx_t *ctx, vaddr_t vaddr, vm_prot_t access);

/*
 * Increase maximum kernel virtual addresses by at least `size` bytes.
 * Allocate page tables if needed.
 *
 * Returns:
 *  - current maximum KVA if `size` == 0
 *  - new maximum KVA otherwise
 */
vaddr_t pmap_growkernel(size_t size);

/*
 * Direct map.
 */
void *phys_to_dmap(paddr_t addr);

#endif /* !_SYS_PMAP_H_ */
