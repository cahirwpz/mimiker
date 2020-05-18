#ifndef _SYS_COMMON_BUS_DEFS_H_
#define _SYS_COMMON_BUS_DEFS_H_

#include <sys/types.h>

typedef struct bus_space bus_space_t;

/* Following types roughly correspond to those described in section TYPES of
 * bus_space(9) FreeBSD manual page. */

typedef uintptr_t bus_addr_t;
typedef uintptr_t bus_size_t;
typedef bus_space_t *bus_space_tag_t;
typedef bus_addr_t bus_space_handle_t;

#endif
