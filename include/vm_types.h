#ifndef _TYPES_H_
#define _TYPES_H_

/* This file is used to solve problem of cyclic dependencies between includes in files
 * vm_phys.h, vm_map.h pmap.h, tlb.h */

#include <common.h> 

typedef struct vm_page vm_page_t;
typedef struct vm_object vm_object_t;
typedef struct vm_map_entry vm_map_entry_t;
typedef struct vm_map vm_map_t;
typedef struct pmap pmap_t;
typedef uint8_t asid_t;

#endif /* _TYPES_H */
