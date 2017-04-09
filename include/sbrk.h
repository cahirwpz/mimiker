#ifndef _SYS_SBRK_H_
#define _SYS_SBRK_H_

#include <vm_map.h>

#define SBRK_INITIAL_SIZE 2048

vm_addr_t sbrk_create(vm_map_t *map);
vm_addr_t sbrk_resize(vm_map_t *map, intptr_t increment);

#endif // _SYS_SBRK_H_
