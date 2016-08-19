#include <interrupts.h>
#include <libkern.h>
#include <mips.h>
#include <pmap.h>
#include <mips/cpu.h>
#include <vm_map.h>
#include <pager.h>

extern const char _ebase[];

void intr_init() {
  /*
   * Enable Vectored Interrupt Mode as described in „MIPS32® 24KETM Processor
   * Core Family Software User’s Manual”, chapter 6.3.1.2.
   */

  /* The location of exception vectors is set to EBase. */
  mips32_set_c0(C0_EBASE, _ebase);
  mips32_bc_c0(C0_STATUS, SR_BEV);
  /* Use the special interrupt vector at EBase + 0x200. */
  mips32_bs_c0(C0_CAUSE, CR_IV);
  /* Set vector spacing for 0x20. */
  mips32_set_c0(C0_INTCTL, INTCTL_VS_32);

  /*
   * Mask out software and hardware interrupts. 
   * You should enable them one by one in driver initialization code.
   */
  mips32_set_c0(C0_STATUS, mips32_get_c0(C0_STATUS) & ~SR_IPL_MASK);

  intr_enable();
}

static intr_event_t *events[8];

void intr_event_add_handler(intr_event_t *ie, intr_handler_t *ih) {
  intr_handler_t *it;
  /* Add new handler according to it's priority */
  TAILQ_FOREACH(it, &ie->ie_handlers, ih_next) {
    if(ih->ih_prio > it->ih_prio)
      break;
  }
  if (it)
    TAILQ_INSERT_BEFORE(it, ih, ih_next);
  else
    /* List is empty */
    TAILQ_INSERT_TAIL(&ie->ie_handlers, ih, ih_next);
}

void run_event_handlers(unsigned irq) {
  intr_event_execute_handlers(events[irq]);
}

void intr_event_init(intr_event_t *ie, uint8_t irq, char *name) {
  ie->ie_rq = irq;
  ie->ie_name = name;
  TAILQ_INIT(&ie->ie_handlers);
  events[irq] = ie;
}

void intr_event_remove_handler(intr_handler_t *ih) {
  TAILQ_REMOVE(&ih->ih_event->ie_handlers, ih, ih_next);
}

/* TODO when we have threads implement deferring work to thread.
 * With current implementation all filters have to either handle filter or
 * report that it is stray interrupt.
 * */

void intr_event_execute_handlers(intr_event_t *ie) {
  intr_handler_t *it;
  int flag = 0;
  TAILQ_FOREACH(it, &ie->ie_handlers, ih_next) {
    flag |= it->ih_filter(it->ih_argument);
    /* Filter captured interrupt */
    if (flag & FILTER_HANDLED) return;
  }
#if 0
  /* clear flag */
  KASSERT(flag & FILTER_STRAY);
  int ifs_n = ie->ie_rq / 32;
  int intr_offset = (ie->ie_rq % 32);
  IFSCLR(ifs_n) = 1 << intr_offset;
#endif
}

static const char *exceptions[32] = {
  [EXC_INTR] = "Interrupt",
  [EXC_MOD]  = "TLB modification exception",
  [EXC_TLBL] = "TLB exception (load or instruction fetch)",
  [EXC_TLBS] = "TLB exception (store)",
  [EXC_ADEL] = "Address error exception (load or instruction fetch)",
  [EXC_ADES] = "Address error exception (store)",
  [EXC_IBE]  = "Bus error exception (instruction fetch)",
  [EXC_DBE]  = "Bus error exception (data reference: load or store)",
  [EXC_BP]   = "Breakpoint exception",
  [EXC_RI]   = "Reserved instruction exception",
  [EXC_CPU]  = "Coprocessor Unusable exception",
  [EXC_OVF]  = "Arithmetic Overflow exception",
  [EXC_TRAP] = "Trap exception",
  [EXC_FPE]  = "Floating point exception",
  [EXC_WATCH] = "Reference to watchpoint address",
  [EXC_MCHECK] = "Machine checkcore",
};

#define PDE_ID_FROM_PTE_ADDR(x) (((x) & 0x003ff000) >> 12)

static vm_map_entry_t *locate_vm_map_entry(vm_addr_t addr)
{
    return vm_map_find_entry(get_active_vm_map(), addr);
}

__attribute__((interrupt)) 
void tlb_exception_handler() {
  int code = (mips32_get_c0(C0_CAUSE) & CR_X_MASK) >> CR_X_SHIFT;
  unsigned vaddr = mips32_get_c0(C0_BADVADDR);

  kprintf("[tlb] %s at 0x%08x!\n",
          exceptions[code], (unsigned)mips32_get_c0(C0_EPC));
  kprintf("[tlb] Caused by reference to 0x%08x!\n", vaddr);

  if (PTE_BASE <= vaddr && vaddr < PTE_BASE + PTE_SIZE) 
  {
    /* If the fault was in virtual pt range it means it's time to refill */
    kprintf("[tlb] pde_refill\n");
    uint32_t id = PDE_ID_FROM_PTE_ADDR(vaddr);
    tlbhi_t entryhi = mips32_get_c0(C0_ENTRYHI);

    pmap_t *active_pmap = get_active_pmap();
    if (!(active_pmap->pde[id] & V_MASK))
      panic("Trying to access unmapped memory region. "
            "Check for null pointers and stack overflows.");

    id &= ~1;
    pte_t entrylo0 = active_pmap->pde[id];
    pte_t entrylo1 = active_pmap->pde[id + 1];
    tlb_overwrite_random(entryhi, entrylo0, entrylo1);
  }
  else if (code == (EXC_TLBL | EXC_TLBS)) 
  {
    vm_map_entry_t *entry = locate_vm_map_entry(vaddr);
    if (entry) {
      /* If access to address was ok, but didn't match entry in page table,
       * it means it's time to call pager */
      if (entry->flags & (VM_READ | VM_WRITE)) {
        assert(entry->object != NULL);
        vm_addr_t offset = vaddr - entry->start;
        entry->object->handler(entry, offset);
      } else {
        /* TODO: What if processor gets here? */
        panic("???");
      }
    } else {
      panic("Address not mapped.");
    }
  }
}

void kernel_oops() {
  unsigned code = (mips32_get_c0(C0_CAUSE) & CR_X_MASK) >> CR_X_SHIFT;
  unsigned errpc = mips32_get_c0(C0_ERRPC);
  unsigned badvaddr = mips32_get_c0(C0_BADVADDR);

  kprintf("[oops] %s at $%08x!\n", exceptions[code], errpc);
  if (code == EXC_ADEL || code == EXC_ADES)
    kprintf("[oops] Caused by reference to $%08x!\n", badvaddr);

  panic("Unhandled exception");
}

/* 
 * Following is general exception table. General exception handler has very
 * little space to use. So it loads address of handler from here. All functions
 * being jumped to, should have ((interrupt)) attribute, unless some exception
 * is unhandled, then these functions should panic the kernel.  For exact
 * meanings of exception handlers numbers please check 5.23 Table of MIPS32
 * 4KEc User's Manual. 
 */

void *general_exception_table[32] = {
  [EXC_MOD]  = tlb_exception_handler,
  [EXC_TLBL] = tlb_exception_handler,
  [EXC_TLBS] = tlb_exception_handler,
};

bool during_intr_handler() {
  return mips32_get_c0(C0_STATUS) & SR_EXL;
}
