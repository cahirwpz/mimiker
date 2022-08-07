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

__long_call void pmap_bootstrap(paddr_t pd_pa, void *pd);
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
 * Handle page fault exception caused by access to `vaddr`
 * with permissions `access`.
 *
 * Returns:
 *  - 0: on success, indicates that the faulting instruction
 *    should be run again or the on fault mechanism is on
 *  - `EINVAL`: if page fault has been caused by access to kernel memory
 *  - `EACCESS`: indicates the the attempted access dosen't have
 *    sufficient permissions
 *  - `EFAULT`: indicates a pager fault
 */
int pmap_fault_handler(ctx_t *ctx, vaddr_t vaddr, vm_prot_t access);

void pmap_growkernel(vaddr_t maxkvaddr);

/*
 * Direct map.
 */
void *phys_to_dmap(paddr_t addr);
paddr_t dmap_to_phys(void *ptr);

#endif /* !_SYS_PMAP_H_ */
