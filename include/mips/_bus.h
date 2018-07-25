#ifndef _SYS_MIPS_BUS_H_
#define _SYS_MIPS_BUS_H_

#include <common.h>

typedef struct bus_space bus_space_t;

/* Following types roughly correspond to those described in section TYPES of
 * bus_space(9) FreeBSD manual page. */

typedef uintptr_t bus_addr_t;
typedef size_t bus_size_t;
typedef bus_space_t *bus_space_tag_t;
typedef bus_addr_t bus_space_handle_t;

#endif /* !_SYS_MIPS_BUS_H */
