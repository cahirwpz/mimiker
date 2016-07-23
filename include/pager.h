#ifndef _PAGER_H_
#define _PAGER_H_

#include <libkern.h>
#include <vm_map.h>

void anon_pager_handler(vm_map_entry_t *entry, vm_addr_t fault_addr);

#endif /* _VM_MAP_H */
