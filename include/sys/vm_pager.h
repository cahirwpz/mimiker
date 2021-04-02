#ifndef _SYS_VM_PAGER_H_
#define _SYS_VM_PAGER_H_

#include <sys/vm.h>

typedef enum {
  VM_DUMMY,
  VM_ANONYMOUS,
} vm_pgr_type_t;

typedef vm_page_t *vm_pgr_fault_t(uvm_object_t *obj, off_t offset);

typedef struct vm_pager {
  vm_pgr_type_t pgr_type;
  vm_pgr_fault_t *pgr_fault;
} vm_pager_t;

extern vm_pager_t pagers[];

#endif /* !_SYS_VM_PAGER_H_ */
