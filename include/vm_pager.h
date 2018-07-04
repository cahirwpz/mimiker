#ifndef _SYS_VM_PAGER_H_
#define _SYS_VM_PAGER_H_

#include <vm.h>

typedef enum {
  VM_DUMMY,
  VM_ANONYMOUS,
} vm_pgr_type_t;

typedef vm_page_t *vm_pgr_fault_t(vm_object_t *obj, vaddr_t fault_addr,
                                  off_t offset, vm_prot_t prot);

typedef struct vm_pager {
  vm_pgr_type_t pgr_type;
  vm_pgr_fault_t *pgr_fault;
} vm_pager_t;

extern vm_pager_t pagers[];

#endif /* !_SYS_VM_PAGER_H_ */
