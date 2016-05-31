#include <pmap.h>
#include <malloc.h>
#include <libkern.h>
#include <mips/cpu.h>
#include <tlb.h>
#include <vm_map.h>

void pmap_init(pmap_t *pmap, vm_object_t *pt_object)
{
    pt_object->refcount++;
    pmap->pt = (pte_t*)PT_BASE;
    pmap->pt_object = pt_object;

    pmap->dpt_page = vm_phys_alloc(0);
    pmap->dpt      = (pte_t*) DPT_BASE;
    pte_t *kseg0_dpt = (pte_t*)pmap->dpt_page->virt_addr;

    uint32_t dpt_index = DPT_INDEX(DPT_BASE);
    /* It is better to use kseg0 address here,
     * so user doesn't need to modify tlb in order to initialize vm_map */
    SET_PADDR(kseg0_dpt[dpt_index], pmap->dpt_page->phys_addr);
    SET_V(kseg0_dpt[dpt_index], 1);
    SET_D(kseg0_dpt[dpt_index], 1);
}

static void dpt_map(pmap_t *pmap, vaddr_t vaddr)
{
    uint32_t dpt_index = DPT_INDEX(vaddr);
    if( (pmap->dpt[dpt_index] & V_MASK) == 0) /* part of page table isn't located in memory */
    {
        vm_page_t *pg = vm_phys_alloc(0);
        pg->vm_offset = dpt_index*PAGESIZE;

        vm_object_insert_page(pmap->pt_object, pg);
        SET_PADDR(pmap->dpt[dpt_index],pg->phys_addr);
        SET_V(pmap->dpt[dpt_index],1);
        SET_D(pmap->dpt[dpt_index],1);
    }
}

static void pt_map(pmap_t *pmap, vm_addr_t vaddr, vm_paddr_t paddr, uint32_t flags)
{
    dpt_map(pmap, vaddr);
    pte_t entry = flags;
    uint32_t pt_index = PT_INDEX(vaddr);
    SET_PADDR(entry ,paddr);
    pmap->pt[pt_index] = entry;
}

void pmap_map_range(pmap_t *pmap, vm_addr_t vaddr, vm_paddr_t paddr, size_t npages, uint32_t flags)
{
    for(size_t i = 0; i < npages; i++)
        pt_map(pmap, vaddr + i*PAGESIZE, paddr + i*PAGESIZE, flags);
}

void pmap_map_vm_object(pmap_t *pmap, vm_addr_t vaddr, uint32_t flags, vm_object_t *obj)
{
    vm_page_t *pg;
    TAILQ_FOREACH(pg, &obj->list, obj_list)
    {
        pmap_map_range(pmap, vaddr+pg->vm_offset, pg->phys_addr, 1 << pg->order, flags);
    }
}
