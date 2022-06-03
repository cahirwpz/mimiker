#ifndef _SYS_PMAP_H_
#define _SYS_PMAP_H_

#include <stdbool.h>
#include <sys/vm.h>

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

/*
 * For address `va` (must not cross two pages) check if access with `prot`
 * permission would succeed and emulate referenced & modified bits.
 *
 * Returns:
 *  - 0: if the access should be permitted
 *  - EFAULT: if `va` is not mapped by current pmap
 *  - EACCESS: if `va` mapping has been found to have insufficient permissions
 *  - EINVAL: if `va` refers to kernel non-pageable memory
 */
int pmap_emulate_bits(pmap_t *pmap, vaddr_t va, vm_prot_t prot);

void pmap_activate(pmap_t *pmap);

pmap_t *pmap_lookup(vaddr_t va);
pmap_t *pmap_kernel(void);
pmap_t *pmap_user(void);

void pmap_growkernel(vaddr_t maxkvaddr);

/*
 * Direct map.
 */
void *phys_to_dmap(paddr_t addr);

#endif /* !_SYS_PMAP_H_ */
