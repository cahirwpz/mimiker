#include <sys/vmem.h>
#include <sys/queue.h>
#include <sys/pool.h>
#include <sys/mimiker.h>
#include <sys/libkern.h>
#include <sys/klog.h>
#include <sys/errno.h>
#include <sys/hash.h>

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
    LIST_ENTRY(bt) bt_freelist;
    LIST_ENTRY(bt) bt_hashlist;
  };
  vmem_addr_t bt_start;
  vmem_size_t bt_size;
  bt_type_t bt_type;
} bt_t;

static POOL_DEFINE(P_VMEM, "vmem", sizeof(vmem_t));
static POOL_DEFINE(P_BT, "vmem boundary tag", sizeof(bt_t));

static vmem_freelist_t *bt_freehead_tofree(vmem_t *vm, vmem_size_t size) {
  const vmem_size_t qsize = size >> vm->vm_quantum_shift;
  const int idx = SIZE2ORDER(qsize);

  assert(size != 0 && qsize != 0);
  assert(idx >= 0);
  assert(idx < VMEM_MAXORDER);

  return &vm->vm_freelist[idx];
}

static vmem_freelist_t *bt_freehead_toalloc(vmem_t *vm, vmem_size_t size,
                                            vmem_flag_t strat) {
  assert((size & vm->vm_quantum_mask) == 0);
  const vmem_size_t qsize = size >> vm->vm_quantum_shift;
  int idx = SIZE2ORDER(qsize);
  assert(size != 0 && qsize != 0);
  if (strat == VMEM_INSTANTFIT && ORDER2SIZE(idx) != qsize)
    idx++;
  assert(idx >= 0);
  assert(idx < VMEM_MAXORDER);
  return &vm->vm_freelist[idx];
}

static bool bt_isspan(const bt_t *bt) {
  return bt->bt_type == BT_TYPE_SPAN;
}

static vmem_addr_t bt_end(const bt_t *bt) {
  return bt->bt_start + bt->bt_size - 1;
}

static vmem_hashlist_t *bt_hashhead(vmem_t *vm, vmem_addr_t addr) {
  unsigned int hash = hash32_buf(&addr, sizeof(addr), HASH32_BUF_INIT);
  return &vm->vm_hashlist[hash % VMEM_MAXHASH];
}

static void bt_insfree(vmem_t *vm, bt_t *bt) {
  vmem_freelist_t *list = bt_freehead_tofree(vm, bt->bt_size);
  LIST_INSERT_HEAD(list, bt, bt_freelist);
}

static void bt_remfree(vmem_t *vm, bt_t *bt) {
  assert(bt->bt_type == BT_TYPE_SPAN);
  LIST_REMOVE(bt, bt_freelist);
}

static void bt_insseg_tail(vmem_t *vm, bt_t *bt) {
  TAILQ_INSERT_TAIL(&vm->vm_seglist, bt, bt_seglist);
}

static void bt_insseg(vmem_t *vm, bt_t *bt, bt_t *prev) {
  TAILQ_INSERT_AFTER(&vm->vm_seglist, prev, bt, bt_seglist);
}

static void bt_insbusy(vmem_t *vm, bt_t *bt) {
  assert(bt->bt_type == BT_TYPE_BUSY);

  vmem_hashlist_t *list = bt_hashhead(vm, bt->bt_start);
  LIST_INSERT_HEAD(list, bt, bt_hashlist);
  vm->vm_inuse += bt->bt_size;
}

static vmem_size_t vmem_roundup_size(vmem_t *vm, vmem_size_t size) {
  return (size + vm->vm_quantum_mask) & ~vm->vm_quantum_mask;
}

static vmem_addr_t vmem_alignup_addr(vmem_addr_t addr, vmem_size_t align) {
  return (addr + align - 1) & ~(align - 1);
}

static void vmem_check_sanity(vmem_t *vm) {
  const bt_t *bt1, *bt2;

  assert(vm != NULL);

  TAILQ_FOREACH (bt1, &vm->vm_seglist, bt_seglist)
    assert(bt1->bt_start <= bt_end(bt1));

  TAILQ_FOREACH (bt1, &vm->vm_seglist, bt_seglist) {
    TAILQ_FOREACH (bt2, &vm->vm_seglist, bt_seglist) {
      if (bt1 == bt2)
        continue;
      if (bt_isspan(bt1) != bt_isspan(bt2))
        continue;
      assert(bt1->bt_start > bt_end(bt2) || bt2->bt_start > bt_end(bt1));
    }
  }
}

static int vmem_fit(const bt_t *bt, vmem_size_t size, vmem_size_t align,
                    vmem_addr_t *addrp) {
  assert(size > 0);
  assert(bt->bt_size >= size); /* caller's responsibility */

  vmem_addr_t start = bt->bt_start;
  vmem_addr_t end = bt_end(bt);

  start = vmem_alignup_addr(start, align);
  if (start < bt->bt_start)
    start += align;

  if (start <= end && end >= start + size - 1) {
    assert((start & (align - 1)) == 0);
    assert(start + size - 1 <= VMEM_ADDR_MAX);
    assert(bt->bt_start <= start);
    assert(bt_end(bt) >= start + size - 1);
    *addrp = start;
    return 0;
  }

  return ENOMEM;
}

vmem_t *vmem_create(const char *name, vmem_addr_t base, vmem_size_t size,
                    vmem_size_t quantum, vmem_flag_t flags) {
  klog("vmem_create(name=%s, base=%p, size=%lu, quantum=%lu, flags=%u)", name,
       base, size, quantum, flags);
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

  /* TODO call vmem_add etc. */

  return vm;
}

int vmem_add(vmem_t *vm, vmem_addr_t addr, vmem_size_t size,
             vmem_flag_t flags) {
  klog("vmem_add(vm=%p, addr=%lu, size=%lu, flags=%u)", vm, addr, size, flags);
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
  klog("vmem_alloc(vm=%p, size=%lu, flags=%u, addrp=%p)", vm, size, flags,
       addrp);
  const vmem_flag_t strat = flags & (VMEM_BESTFIT | VMEM_INSTANTFIT);
  /* only VMEM_INSTANTFIT is currently supported */
  assert(strat == VMEM_INSTANTFIT);
  assert(size > 0);

  size = vmem_roundup_size(vm, size);
  assert(size > 0);

  const vmem_size_t align = vm->vm_quantum_mask + 1;

  bt_t *btnew = pool_alloc(P_BT, PF_ZERO);
  bt_t *btnew2 = pool_alloc(P_BT, PF_ZERO);

  vmem_freelist_t *first = bt_freehead_toalloc(vm, size, strat);
  vmem_freelist_t *end = &vm->vm_freelist[VMEM_MAXORDER];
  vmem_addr_t start;

  vmem_check_sanity(vm);

  bt_t *bt = NULL;
  for (vmem_freelist_t *list = first; list < end; list++) {
    bt = LIST_FIRST(list);
    if (bt != NULL) {
      int rc = vmem_fit(bt, size, align, &start);
      if (rc == 0)
        goto gotit;
    }
  }

  pool_free(P_BT, btnew);
  pool_free(P_BT, btnew2);

gotit:
  assert(bt->bt_size >= size);
  vmem_check_sanity(vm);

  bt_remfree(vm, bt);

  if (bt->bt_start != start) {
    btnew2->bt_type = BT_TYPE_FREE;
    btnew2->bt_start = bt->bt_start;
    btnew2->bt_size = start - bt->bt_start;
    bt->bt_start = start;
    bt->bt_size -= btnew2->bt_size;
    bt_insfree(vm, btnew2);
    bt_insseg(vm, btnew2, TAILQ_PREV(bt, vmem_seglist, bt_seglist));
    btnew2 = NULL;
    vmem_check_sanity(vm);
  }
  assert(bt->bt_start == start);
  if (bt->bt_size != size && bt->bt_size - size > vm->vm_quantum_mask) {
    /* split */
    btnew->bt_type = BT_TYPE_BUSY;
    btnew->bt_start = bt->bt_start;
    btnew->bt_size = size;
    bt->bt_start = bt->bt_start + size;
    bt->bt_size -= size;
    bt_insfree(vm, bt);
    bt_insseg(vm, btnew, TAILQ_PREV(bt, vmem_seglist, bt_seglist));
    bt_insbusy(vm, btnew);
    vmem_check_sanity(vm);
  } else {
    bt->bt_type = BT_TYPE_BUSY;
    bt_insbusy(vm, bt);
    vmem_check_sanity(vm);
    pool_free(P_BT, btnew);
    btnew = bt;
  }
  if (btnew2 != NULL) {
    pool_free(P_BT, btnew2);
  }

  assert(btnew->bt_size >= size);
  btnew->bt_type = BT_TYPE_BUSY;

  if (addrp != NULL)
    *addrp = btnew->bt_start;

  return 0;
}

void vmem_free(vmem_t *vm, vmem_addr_t addr, vmem_size_t size) {
  klog("vmem_free(vm=%p, addr=%ld, size=%ld) had no effect", vm, addr, size);
}

void vmem_destroy(vmem_t *vm) {
  klog("vmem_destroy(vm=%p)", vm);

  LIST_REMOVE(vm, vm_alllist);

  /* in NetBSD they only free vm_hashlist entries, asserting that all of them
     are of span type */

  for (int i = 0; i < VMEM_MAXHASH; i++) {
    bt_t *bt;
    while ((bt = LIST_FIRST(&vm->vm_hashlist[i])) != NULL) {
      assert(bt->bt_type == BT_TYPE_SPAN);
      pool_free(P_BT, bt);
    }
  }

  pool_free(P_VMEM, vm);
}
