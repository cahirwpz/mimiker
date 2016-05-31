#include <tlb.h>
#include <mips/cpu.h>
#include <mips/m32c0.h>
#include <libkern.h>
#include <vm_phys.h>
#include <vm_map.h>

#define DPT_WIRED_TLB_INDEX 0
#define NULL_CATCH_TLB_INDEX 1

static void insert_null_catch_tlb_entry()
{
    tlblo_t entrylo0 = 0;
    tlblo_t entrylo1 = 0;
    uint32_t pagemask = 0;
    tlbhi_t entryhi = 0;

    SET_V(entrylo0, 0);
    SET_D(entrylo0, 0);
    SET_G(entrylo0, 1);

    mips_tlbwi2(entryhi, entrylo0, entrylo1, pagemask, NULL_CATCH_TLB_INDEX);

}

void tlb_init()
{
    mips_tlbinvalall();
    mips32_set_c0(C0_WIRED,2); /* DPT will be kept in index 0, while null catcher will be at 1 */
    insert_null_catch_tlb_entry();
}

void tlb_set_page_table(pmap_t *pmap)
{
    mips32_set_c0(C0_CONTEXT, (uint32_t)(pmap->pt) << 1);

    tlblo_t entrylo0 = 0;
    tlblo_t entrylo1 = 0;
    uint32_t pagemask = 0;
    tlbhi_t entryhi = ((uint32_t)pmap->dpt) | pmap->asid;

    SET_PADDR(entrylo0, pmap->dpt_page->phys_addr);
    SET_V(entrylo0, 1);
    SET_D(entrylo0, 1);

    mips_tlbwi2(entryhi, entrylo0, entrylo1, pagemask, DPT_WIRED_TLB_INDEX);
}

void tlb_print()
{
    size_t n = mips_tlb_size();
    for(size_t i = 0; i < n; i++)
    {

        tlbhi_t entryhi;
        tlblo_t entrylo0;
        tlblo_t entrylo1;
        unsigned pmask;
        mips_tlbri2(&entryhi, &entrylo0, &entrylo1, &pmask, i);

        kprintf("[tlb] index:    %d\n", i);
        kprintf("[tlb] entryhi:  %p\n", entryhi);
        kprintf("[tlb] entrylo0: %p\n", entrylo0);
        kprintf("[tlb] entrylo1: %p\n", entrylo1);
        kprintf("[tlb] pmask:    %p\n", pmask);
    }
}
