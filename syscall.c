#include <stdc.h>
#include <sysent.h>
#include <thread.h>
#include <vm_map.h>
#include <vm_pager.h>

int main() {
  /* System call from kernel space! */
  kprintf("syscall(1) = %d\n", syscall(1));

  /* ... let's prepare user space */
  set_active_vm_map(vm_map_new(USER_VM_MAP));

  vm_map_t *umap = get_active_vm_map(PMAP_USER);

  vm_addr_t stext = 0x400000;
  vm_addr_t etext = stext + PAGESIZE;
  vm_map_entry_t *text =
    vm_map_add_entry(umap, stext, etext,
                     VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXEC);
  text->object = default_pager->pgr_alloc();

  vm_addr_t sdata = 0x800000;
  vm_addr_t edata = sdata + PAGESIZE;
  vm_map_entry_t *data =
    vm_map_add_entry(umap, sdata, edata,
                     VM_PROT_READ | VM_PROT_WRITE);
  data->object = default_pager->pgr_alloc();

  vm_map_dump(umap);

  uint32_t *code = (uint32_t *)stext;

  *code++ = 0x24020002; /* li	$v0, 2 */
  *code++ = 0x0000000c; /* syscall   */
  *code++ = 0x1000ffff; /* b .       */

  /* TODO: clear caches ? */

  thread_t *td = thread_self();

  td->td_uctx.pc = stext;
  user_exc_leave();
}
