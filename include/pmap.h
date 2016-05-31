#ifndef _PMAP_H_
#define	_PMAP_H_

#include <vm_types.h>
#include <tlb.h>
#include <vm_phys.h>
#include <queue.h>

#define PMAP_GLOBAL G_MASK /* Ignore for now */
#define PMAP_VALID V_MASK /* Access to page with PMAP_VALID won't cause exception */
#define PMAP_DIRTY D_MASK /* Writing to page with PMAP_DIRTY won't cause exception */

#define PT_INDEX(x)  (((x) & 0xfffff000) >> 12)
#define DPT_INDEX(x) (((x) & 0xffc00000) >> 22)


typedef struct pmap
{
    pte_t* pt;  /* page table */
    pte_t* dpt; /* directory page table */
    vm_page_t *dpt_page; /* pointer to page allocated from vm_phys containing dpt */
    vm_object_t *pt_object; /* vm_object containing page table, kept here for optimisation */
    asid_t asid;
} pmap_t;

void pmap_init(pmap_t *pmap, vm_object_t *pt_object);

/* Before using these functions make sure proper vm_map is set as cur_vm_map */
void pmap_map_range(pmap_t *pmap, vm_addr_t vaddr, vm_paddr_t paddr, size_t npages, uint32_t flags);
void pmap_map_vm_object(pmap_t *pmap, vm_addr_t vaddr, uint32_t flags, vm_object_t *obj);

#endif /* !_PMAP_H_ */

