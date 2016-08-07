#ifndef _PAGER_H_
#define _PAGER_H_

#include <libkern.h>
#include <vm_map.h>

typedef struct vm_map vm_map_t;
typedef struct vm_map_entry vm_map_entry_t;
typedef struct vm_object vm_object_t;

typedef enum { READ_ACCESS, WRITE_ACCESS } access_t;

typedef enum {
  DEFAULT_PAGER /* Default pager implements paging on demand */
} pager_handler_t;

typedef struct pager {
  pager_handler_t type;
  union {
    /* Currently empty, because default pager has no data */
  } pager_data;
} pager_t;

void page_fault(vm_map_t *map, vm_addr_t fault_addr, access_t access);

void pager_init();
pager_t *pager_allocate(pager_handler_t type);
void pager_delete(pager_t *pgr);

#endif /* _PAGER_H_ */
