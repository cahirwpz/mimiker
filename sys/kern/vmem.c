#include <sys/vmem.h>
#include <sys/queue.h>
#include <sys/pool.h>
#include <sys/mimiker.h>

#define VMEM_MAXORDER (sizeof(vmem_size_t) * CHAR_BIT)
#define VMEM_MAXHASH 512
#define VMEM_NAME_MAX 16

#define VMEM_ADDR_MIN 0
#define VMEM_ADDR_MAX (~(vmem_addr_t)0)

typedef TAILQ_HEAD(vmem_seglist, vmem_bt) vmem_seglist_t;
typedef LIST_HEAD(vmem_freelist, vmem_bt) vmem_freelist_t;
typedef LIST_HEAD(vmem_hashlist, vmem_bt) vmem_hashlist_t;

// static LIST_HEAD(, vmem) vmem_list = LIST_HEAD_INITIALIZER(vmem_list);

typedef struct vmem {
  vm_flag_t vm_flags;
  size_t vm_size;
  size_t vm_inuse;
  char vm_name[VMEM_NAME_MAX + 1];
  vmem_seglist_t vm_seglist;
  vmem_freelist_t vm_freelist[VMEM_MAXORDER];
  vmem_hashlist_t vm_hashlist[VMEM_MAXHASH];
  LIST_ENTRY(vmem) vm_alllist;
} vmem_t;

typedef enum { BT_TYPE_FREE, BT_TYPE_BUSY, BT_TYPE_SPAN } bt_type_t;

typedef struct vmem_bt {
  TAILQ_ENTRY(vmem_bt) bt_seglist;
  union {
    LIST_ENTRY(vmem_bt) u_freelist;
    LIST_ENTRY(vmem_bt) u_hashlist;
  } bt_u;
  vmem_addr_t bt_start;
  vmem_size_t bt_size;
  bt_type_t bt_type;
} vmem_bt_t;

static POOL_DEFINE(P_VMEM, "vmem", sizeof(vmem_t));
static POOL_DEFINE(P_BT, "vmem boundary tag", sizeof(vmem_bt_t));

vmem_t *vmem_create(const char *name, vmem_addr_t base, vmem_size_t size,
                    vm_flag_t flags) {
  /* only NOSLEEP is currently supported */
  assert(flags == VMEM_NOSLEEP);

  return 0;
}

int vmem_add(vmem_t *vm, vmem_addr_t addr, vmem_size_t size, vm_flag_t flags) {
  /* only NOSLEEP is currently supported */
  assert(flags == VMEM_NOSLEEP);

  return 0;
}

int vmem_alloc(vmem_t *vm, vmem_size_t size, vm_flag_t flags,
               vmem_addr_t *addrp) {
  /* only NOSLEEP is currently supported */
  assert(flags == VMEM_INSTANTFIT);

  return 0;
}

void vmem_free(vmem_t *vm, vmem_addr_t addr, vmem_size_t size) {
}

void vmem_destroy(vmem_t *vm) {
}
