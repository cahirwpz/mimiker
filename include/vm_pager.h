#ifndef _PAGER_H_
#define _PAGER_H_

#include <vm.h>

typedef vm_object_t *pgr_alloc_t(void);
typedef vm_page_t *pgr_fault_t(vm_object_t *, vm_addr_t fault_addr,
                               vm_addr_t offset, vm_prot_t prot);
typedef void pgr_free_t(vm_object_t *);

typedef struct pager {
  pgr_alloc_t *pgr_alloc;
  pgr_free_t *pgr_free;
  pgr_fault_t *pgr_fault;
} pager_t;

extern pager_t default_pager[1];

#endif /* _PAGER_H_ */
