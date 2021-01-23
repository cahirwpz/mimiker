#ifndef _SYS_VM_MAP_H_
#define _SYS_VM_MAP_H_

#include <sys/queue.h>
#include <sys/pmap.h>
#include <sys/vm.h>
#include <sys/mutex.h>

typedef struct vm_map vm_map_t;
typedef struct vm_segment vm_segment_t;

typedef enum {
  VM_SEG_SHARED = 1,    /* shared memory */
  VM_SEG_PRIVATE = 2,   /* private memory (default) */
  VM_SEG_NEED_COPY = 4, /* copy on write segment, needs copying of pages */
} vm_seg_flags_t;

/*! \brief Called during kernel initialization. */
void init_vm_map(void);

/*! \brief Acquire vm_map non-recursive mutex. */
void vm_map_lock(vm_map_t *map);

/*! \brief Release vm_map non-recursive mutex. */
void vm_map_unlock(vm_map_t *map);

DEFINE_CLEANUP_FUNCTION(vm_map_t *, vm_map_unlock);

#define WITH_VM_MAP_LOCK(map)                                                  \
  WITH_STMT(vm_map_t, vm_map_lock, CLEANUP_FUNCTION(vm_map_unlock), map)

#define SCOPED_VM_MAP_LOCK(map)                                                \
  SCOPED_STMT(vm_map_t, vm_map_lock, CLEANUP_FUNCTION(vm_map_unlock), map)

void vm_map_activate(vm_map_t *map);
void vm_map_switch(thread_t *td);

vm_map_t *vm_map_user(void);
vm_map_t *vm_map_kernel(void);
vm_map_t *vm_map_lookup(vaddr_t addr);

vm_map_t *vm_map_new(void);
void vm_map_delete(vm_map_t *vm_map);

vm_segment_t *vm_segment_alloc(vm_object_t *obj, vaddr_t start, vaddr_t end,
                               vm_prot_t prot, vm_seg_flags_t flags);
void vm_segment_destroy(vm_map_t *map, vm_segment_t *seg);
void vm_segment_destroy_range(vm_map_t *map, vm_segment_t *seg, vaddr_t start,
                              vaddr_t end);

vm_segment_t *vm_map_find_segment(vm_map_t *vm_map, vaddr_t vaddr);

void vm_map_protect(vm_map_t *map, vaddr_t start, vaddr_t end, vm_prot_t prot);

/*! \brief Insert given \a segment into the \a map. */
int vm_map_insert(vm_map_t *map, vm_segment_t *segment, vm_flags_t flags);

/*! \brief Can address \a addr be mapped by this \a map? */
bool vm_map_address_p(vm_map_t *map, vaddr_t addr);
bool vm_map_contains_p(vm_map_t *map, vaddr_t start, vaddr_t end);

/*! \brief Reports start & end addresses that are handled by the map. */
vaddr_t vm_map_start(vm_map_t *map);
vaddr_t vm_map_end(vm_map_t *map);

/*! \brief Reports start & end addresses that are handled by the segment. */
vaddr_t vm_segment_start(vm_segment_t *seg);
vaddr_t vm_segment_end(vm_segment_t *seg);

/*! \brief Looks up a gap of \a length size in \a map.
 *
 * The search starts at an address read from \a start_p location.
 * Both start address and \a length arguments must be page-aligned.
 *
 * \returns 0 on success, sets *start_p as well
 */
int vm_map_findspace(vm_map_t *map, vaddr_t /*inout*/ *start_p, size_t length);

/*! \brief Allocates segment and associate anonymous memory object with it. */
int vm_map_alloc_segment(vm_map_t *map, vaddr_t addr, size_t length,
                         vm_prot_t prot, vm_flags_t flags,
                         vm_segment_t **seg_p);

/* Tries to resize an segment, by moving its end if there
   are no other mappings in the way. On success, returns 0. */
int vm_segment_resize(vm_map_t *map, vm_segment_t *seg, vaddr_t new_end);

void vm_map_dump(vm_map_t *vm_map);

vm_map_t *vm_map_clone(vm_map_t *map);

int vm_page_fault(vm_map_t *map, vaddr_t fault_addr, vm_prot_t fault_type);

#endif /* !_SYS_VM_MAP_H_ */
