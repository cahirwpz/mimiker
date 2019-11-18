#define KL_LOG KL_VMEM
#include <sys/klog.h>
#include <sys/vmem.h>
#include <sys/queue.h>
#include <sys/pool.h>
#include <sys/mimiker.h>
#include <sys/libkern.h>
#include <sys/errno.h>
#include <sys/hash.h>
#include <sys/mutex.h>

#define VMEM_MAXORDER ((int)(sizeof(vmem_size_t) * CHAR_BIT))
#define VMEM_MAXHASH 512
#define VMEM_NAME_MAX 16

#define ORDER2SIZE(order) ((vmem_size_t)1 << (order))
#define SIZE2ORDER(size) ((int)log2(size))

#define VMEM_ASSERT_LOCKED(vm) assert(mtx_owned(&(vm)->vm_lock))

typedef TAILQ_HEAD(vmem_seglist, bt) vmem_seglist_t;
typedef LIST_HEAD(vmem_freelist, bt) vmem_freelist_t;
typedef LIST_HEAD(vmem_hashlist, bt) vmem_hashlist_t;

static mtx_t vmem_alllist_lock = MTX_INITIALIZER(0);
static LIST_HEAD(, vmem) vmem_alllist = LIST_HEAD_INITIALIZER(vmem_alllist);

typedef struct vmem {
  mtx_t vm_lock;
  size_t vm_size;
  size_t vm_inuse;
  size_t vm_quantum;
  int vm_quantum_shift;
  char vm_name[VMEM_NAME_MAX];
  vmem_seglist_t vm_seglist;
  vmem_freelist_t vm_freelist[VMEM_MAXORDER];
  vmem_hashlist_t vm_hashlist[VMEM_MAXHASH];
  LIST_ENTRY(vmem) vm_alllist_link;
} vmem_t;

typedef enum { BT_TYPE_FREE, BT_TYPE_BUSY, BT_TYPE_SPAN } bt_type_t;

typedef struct bt {
  TAILQ_ENTRY(bt) bt_seglink;
  union {
    LIST_ENTRY(bt) bt_freelink;
    LIST_ENTRY(bt) bt_hashlink;
  };
  vmem_addr_t bt_start;
  vmem_size_t bt_size;
  bt_type_t bt_type;
} bt_t;

static POOL_DEFINE(P_VMEM, "vmem", sizeof(vmem_t));
static POOL_DEFINE(P_BT, "vmem boundary tag", sizeof(bt_t));

static vmem_freelist_t *bt_freehead_tofree(vmem_t *vm, vmem_size_t size) {
  vmem_size_t qsize = size >> vm->vm_quantum_shift;
  assert(size != 0 && qsize != 0);
  int idx = SIZE2ORDER(qsize);
  assert(idx >= 0 && idx < VMEM_MAXORDER);
  return &vm->vm_freelist[idx];
}

/* Returns the freelist for the given size and allocation strategy.
 *
 * For VMEM_INSTANTFIT, returns the list in which any blocks are large enough.
 * For VMEM_BESTFIT, returns the list which can have blocks large enough. */
static vmem_freelist_t *bt_freehead_toalloc(vmem_t *vm, vmem_size_t size) {
  assert(is_aligned(size, vm->vm_quantum));
  vmem_size_t qsize = size >> vm->vm_quantum_shift;
  assert(size != 0 && qsize != 0);
  int idx = SIZE2ORDER(qsize);
  if (ORDER2SIZE(idx) != qsize)
    idx++;
  assert(idx >= 0 && idx < VMEM_MAXORDER);
  return &vm->vm_freelist[idx];
}

static void vmem_lock(vmem_t *vm) {
  mtx_lock(&vm->vm_lock);
}

static void vmem_unlock(vmem_t *vm) {
  mtx_unlock(&vm->vm_lock);
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
  VMEM_ASSERT_LOCKED(vm);
  assert(bt->bt_type == BT_TYPE_FREE);
  vmem_freelist_t *list = bt_freehead_tofree(vm, bt->bt_size);
  LIST_INSERT_HEAD(list, bt, bt_freelink);
}

static void bt_remfree(vmem_t *vm, bt_t *bt) {
  VMEM_ASSERT_LOCKED(vm);
  assert(bt->bt_type == BT_TYPE_FREE);
  LIST_REMOVE(bt, bt_freelink);
}

static void bt_insseg_tail(vmem_t *vm, bt_t *bt) {
  VMEM_ASSERT_LOCKED(vm);
  TAILQ_INSERT_TAIL(&vm->vm_seglist, bt, bt_seglink);
}

static void bt_insseg_after(vmem_t *vm, bt_t *bt, bt_t *prev) {
  VMEM_ASSERT_LOCKED(vm);
  TAILQ_INSERT_AFTER(&vm->vm_seglist, prev, bt, bt_seglink);
}

static void bt_insbusy(vmem_t *vm, bt_t *bt) {
  VMEM_ASSERT_LOCKED(vm);
  assert(bt->bt_type == BT_TYPE_BUSY);
  vmem_hashlist_t *list = bt_hashhead(vm, bt->bt_start);
  LIST_INSERT_HEAD(list, bt, bt_hashlink);
  vm->vm_inuse += bt->bt_size;
}

static void vmem_check_sanity(vmem_t *vm) {
  VMEM_ASSERT_LOCKED(vm);

  const bt_t *bt1;
  TAILQ_FOREACH (bt1, &vm->vm_seglist, bt_seglink) {
    assert(bt1->bt_start <= bt_end(bt1));
    assert(bt1->bt_size >= vm->vm_quantum);
    assert(is_aligned(bt1->bt_start, vm->vm_quantum));
  }

  const bt_t *bt2;
  TAILQ_FOREACH (bt1, &vm->vm_seglist, bt_seglink) {
    TAILQ_FOREACH (bt2, &vm->vm_seglist, bt_seglink) {
      if (bt1 == bt2)
        continue;
      if (bt_isspan(bt1) != bt_isspan(bt2))
        continue;
      assert(bt1->bt_start > bt_end(bt2) || bt2->bt_start > bt_end(bt1));
    }
  }
}

vmem_t *vmem_create(const char *name, vmem_addr_t base, vmem_size_t size,
                    vmem_size_t quantum) {
  vmem_t *vm = pool_alloc(P_VMEM, PF_ZERO);

  vm->vm_quantum = quantum;
  assert(quantum > 0);
  vm->vm_quantum_shift = SIZE2ORDER(quantum);
  /* Check that quantum is a power of 2 */
  assert(ORDER2SIZE(vm->vm_quantum_shift) == quantum);

  mtx_init(&vm->vm_lock, 0);
  strlcpy(vm->vm_name, name, sizeof(vm->vm_name));

  TAILQ_INIT(&vm->vm_seglist);
  for (int i = 0; i < VMEM_MAXORDER; i++)
    LIST_INIT(&vm->vm_freelist[i]);
  for (int i = 0; i < VMEM_MAXHASH; i++)
    LIST_INIT(&vm->vm_hashlist[i]);

  if (size != 0)
    vmem_add(vm, base, size);

  WITH_MTX_LOCK (&vmem_alllist_lock)
    LIST_INSERT_HEAD(&vmem_alllist, vm, vm_alllist_link);

  klog("new vmem '%s' created", name);
  return vm;
}

int vmem_add(vmem_t *vm, vmem_addr_t addr, vmem_size_t size) {
  bt_t *btspan = pool_alloc(P_BT, PF_ZERO);
  bt_t *btfree = pool_alloc(P_BT, PF_ZERO);

  btspan->bt_type = BT_TYPE_SPAN;
  btspan->bt_start = addr;
  btspan->bt_size = size;

  btfree->bt_type = BT_TYPE_FREE;
  btfree->bt_start = addr;
  btfree->bt_size = size;

  WITH_MTX_LOCK (&vm->vm_lock) {
    bt_insseg_tail(vm, btspan);
    bt_insseg_after(vm, btfree, btspan);
    bt_insfree(vm, btfree);
    vm->vm_size += size;
    vmem_check_sanity(vm);
  }

  klog("span [%lu, %lu] added", addr, addr + size - 1);

  return 0;
}

int vmem_alloc(vmem_t *vm, vmem_size_t size, vmem_addr_t *addrp) {
  size = align(size, vm->vm_quantum);
  assert(size > 0);

  /* Allocate new boundary tag before acquiring the vmem lock */
  bt_t *btnew = pool_alloc(P_BT, PF_ZERO);

  vmem_freelist_t *first = bt_freehead_toalloc(vm, size);
  vmem_freelist_t *end = &vm->vm_freelist[VMEM_MAXORDER];

  vmem_lock(vm);
  vmem_check_sanity(vm);

  bt_t *bt;
  bool found = false;
  for (vmem_freelist_t *list = first; list < end; list++) {
    bt = LIST_FIRST(list);
    if (bt != NULL && bt->bt_size >= size) {
      found = true;
      break;
    }
  }

  if (!found) {
    vmem_unlock(vm);
    pool_free(P_BT, btnew);
    klog("alloc of size %lu for vmem '%s' failed (ENOMEM)", size, vm->vm_name);
    return ENOMEM;
  }

  bt_remfree(vm, bt);
  vmem_check_sanity(vm);

  if (bt->bt_size > size && bt->bt_size - size >= vm->vm_quantum) {
    /* Split [bt] into [bt | btnew] */
    btnew->bt_type = BT_TYPE_FREE;
    btnew->bt_start = bt->bt_start + size;
    btnew->bt_size = bt->bt_size - size;
    bt->bt_type = BT_TYPE_BUSY;
    bt->bt_size = size;
    bt_insfree(vm, btnew);
    bt_insseg_after(vm, btnew, bt);
    bt_insbusy(vm, bt);
    /* Set btnew to NULL so that it won't be deallocated after exiting
     * from the vmem lock */
    btnew = NULL;
  } else {
    bt->bt_type = BT_TYPE_BUSY;
    bt_insbusy(vm, bt);
  }

  vmem_check_sanity(vm);
  vmem_unlock(vm);

  if (btnew != NULL)
    pool_free(P_BT, btnew);

  assert(bt->bt_size >= size);
  assert(bt->bt_type == BT_TYPE_BUSY);

  if (addrp != NULL)
    *addrp = bt->bt_start;

  klog("alloc of size %lu for vmem '%s' succeeded", size, vm->vm_name);
  return 0;
}

void vmem_free(vmem_t *vm, vmem_addr_t addr, vmem_size_t size) {
  klog("vmem_free(%p, %ld, %ld) had no effect", vm, addr, size);
}

void vmem_destroy(vmem_t *vm) {
  klog("vmem_destroy(%p) had no effect", vm);
}
