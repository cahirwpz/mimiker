#include <sys/vmem.h>
#include <sys/queue.h>
#include <sys/pool.h>
#include <sys/mimiker.h>
#include <sys/libkern.h>
#include <sys/klog.h>

#define VMEM_MAXORDER ((int)(sizeof(vmem_size_t) * CHAR_BIT))
#define VMEM_MAXHASH 512
#define VMEM_NAME_MAX 16

#define VMEM_ADDR_MIN 0
#define VMEM_ADDR_MAX (~(vmem_addr_t)0)

#define ORDER2SIZE(order) ((vmem_size_t)1 << (order))
#define SIZE2ORDER(size) ((int)log2(size))

typedef TAILQ_HEAD(vmem_seglist, bt) vmem_seglist_t;
typedef LIST_HEAD(vmem_freelist, bt) vmem_freelist_t;
typedef LIST_HEAD(vmem_hashlist, bt) vmem_hashlist_t;

static LIST_HEAD(, vmem) vmem_list = LIST_HEAD_INITIALIZER(vmem_list);

typedef struct vmem {
  vmem_flag_t vm_flags;
  size_t vm_size;
  size_t vm_inuse;
  size_t vm_quantum_mask;
  int vm_quantum_shift;
  char vm_name[VMEM_NAME_MAX];
  vmem_seglist_t vm_seglist;
  vmem_freelist_t vm_freelist[VMEM_MAXORDER];
  vmem_hashlist_t vm_hashlist[VMEM_MAXHASH];
  LIST_ENTRY(vmem) vm_alllist;
} vmem_t;

typedef enum { BT_TYPE_FREE, BT_TYPE_BUSY, BT_TYPE_SPAN } bt_type_t;

typedef struct bt {
  TAILQ_ENTRY(bt) bt_seglist;
  union {
    LIST_ENTRY(bt) u_freelist;
    LIST_ENTRY(bt) u_hashlist;
  } bt_u;
#define bt_hashlist bt_u.u_hashlist
#define bt_freelist bt_u.u_freelist
  vmem_addr_t bt_start;
  vmem_size_t bt_size;
  bt_type_t bt_type;
} bt_t;

static POOL_DEFINE(P_VMEM, "vmem", sizeof(vmem_t));
static POOL_DEFINE(P_BT, "vmem boundary tag", sizeof(bt_t));

vmem_t *vmem_create(const char *name, vmem_addr_t base, vmem_size_t size,
                    vmem_size_t quantum, vmem_flag_t flags) {
  /* only NOSLEEP is currently supported */
  assert(flags == VMEM_NOSLEEP);
  assert(quantum > 0);

  vmem_t *vm = pool_alloc(P_VMEM, PF_ZERO);
  vm->vm_flags = flags;

  vm->vm_quantum_mask = quantum - 1;
  vm->vm_quantum_shift = SIZE2ORDER(quantum);
  assert(ORDER2SIZE(vm->vm_quantum_shift) == quantum);

  strlcpy(vm->vm_name, name, sizeof(vm->vm_name));

  TAILQ_INIT(&vm->vm_seglist);
  for (int i = 0; i < VMEM_MAXORDER; i++)
    LIST_INIT(&vm->vm_freelist[i]);
  for (int i = 0; i < VMEM_MAXHASH; i++)
    LIST_INIT(&vm->vm_hashlist[i]);

  LIST_INSERT_HEAD(&vmem_list, vm, vm_alllist);

  return 0;
}

static vmem_freelist_t *bt_freehead_tofree(vmem_t *vm, vmem_size_t size) {
  const vmem_size_t qsize = size >> vm->vm_quantum_shift;
  const int idx = SIZE2ORDER(qsize);

  assert(size != 0 && qsize != 0);
  assert(idx >= 0);
  assert(idx < VMEM_MAXORDER);

  return &vm->vm_freelist[idx];
}

static void bt_insfree(vmem_t *vm, bt_t *bt) {
  vmem_freelist_t *list = bt_freehead_tofree(vm, bt->bt_size);
  LIST_INSERT_HEAD(list, bt, bt_freelist);
}

static void bt_insseg_tail(vmem_t *vm, bt_t *bt) {
  TAILQ_INSERT_TAIL(&vm->vm_seglist, bt, bt_seglist);
}

static void bt_insseg(vmem_t *vm, bt_t *bt, bt_t *prev) {
  TAILQ_INSERT_AFTER(&vm->vm_seglist, prev, bt, bt_seglist);
}

static vmem_size_t vmem_roundup_size(vmem_t *vm, vmem_size_t size) {
  return (size + vm->vm_quantum_mask) & ~vm->vm_quantum_mask;
}

// static bool vmem_check_sanity(vmem_t *vm) {
//   const bt_t *bt, *bt2;

//   KASSERT(vm != NULL);

//   TAILQ_FOREACH (bt, &vm->vm_seglist, bt_seglist) {
//     if (bt->bt_start > BT_END(bt)) {
//       printf("corrupted tag\n");
//       bt_dump(bt, vmem_printf);
//       return false;
//     }
//   }
//   TAILQ_FOREACH (bt, &vm->vm_seglist, bt_seglist) {
//     TAILQ_FOREACH (bt2, &vm->vm_seglist, bt_seglist) {
//       if (bt == bt2) {
//         continue;
//       }
//       if (BT_ISSPAN_P(bt) != BT_ISSPAN_P(bt2)) {
//         continue;
//       }
//       if (bt->bt_start <= BT_END(bt2) && bt2->bt_start <= BT_END(bt)) {
//         printf("overwrapped tags\n");
//         bt_dump(bt, vmem_printf);
//         bt_dump(bt2, vmem_printf);
//         return false;
//       }
//     }
//   }

//   return true;
// }

int vmem_add(vmem_t *vm, vmem_addr_t addr, vmem_size_t size,
             vmem_flag_t flags) {
  /* only NOSLEEP is currently supported */
  assert(flags == VMEM_NOSLEEP);

  bt_t *btspan = pool_alloc(P_BT, PF_ZERO);
  bt_t *btfree = pool_alloc(P_BT, PF_ZERO);
  /* TODO should I check btspan or btfree for NULL? */

  btspan->bt_type = BT_TYPE_SPAN;
  btspan->bt_start = addr;
  btspan->bt_size = size;

  btfree->bt_type = BT_TYPE_FREE;
  btfree->bt_start = addr;
  btspan->bt_size = size;

  bt_insseg_tail(vm, btspan);
  bt_insseg(vm, btfree, btspan);
  bt_insfree(vm, btfree);

  vm->vm_size += size;

  return 0;
}

int vmem_alloc(vmem_t *vm, vmem_size_t size, vmem_flag_t flags,
               vmem_addr_t *addrp) {
  const vmem_flag_t strat = flags & (VMEM_BESTFIT | VMEM_INSTANTFIT);
  assert(strat == VMEM_BESTFIT || strat == VMEM_INSTANTFIT);
  assert(size > 0);

  size = vmem_roundup_size(vm, size);
  assert(size > 0);

  const vmem_size_t align = vm->vm_quantum_mask + 1;

  bt_t *btnew = pool_alloc(P_BT, PF_ZERO);
  bt_t *btnew2 = pool_alloc(P_BT, PF_ZERO);

  return 0;
}

void vmem_free(vmem_t *vm, vmem_addr_t addr, vmem_size_t size) {
  klog("vmem_free(vm=%p, addr=%ld, size=%ld) had no effect", vm, addr, size);
}

void vmem_destroy(vmem_t *vm) {
  klog("vmem_destroy(vm=%p) had no effect", vm);

  // LIST_REMOVE(vm, vm_alllist);

  /* in NetBSD they only free vm_hashlist entries, asserting that all of them
     are of type BT_TYPE_SPAN */

  /* TODO */

  // pool_free(P_VMEM, vm);
}
