#define KL_LOG KL_VMEM
#include <sys/klog.h>
#include <sys/vmem.h>
#include <sys/queue.h>
#include <sys/pool.h>
#include <sys/malloc.h>
#include <sys/mimiker.h>
#include <sys/libkern.h>
#include <sys/errno.h>
#include <sys/hash.h>
#include <sys/mutex.h>
#include <machine/vm_param.h>

#define VMEM_DEBUG 0

#define VMEM_MAXORDER ((int)(sizeof(vmem_size_t) * CHAR_BIT))
#define VMEM_MAXHASH 512
#define VMEM_NAME_MAX 16

#define ORDER2SIZE(order) ((vmem_size_t)1 << (order))
#define SIZE2ORDER(size) ((int)log2(size))

typedef TAILQ_HEAD(vmem_seglist, bt) vmem_seglist_t;
typedef LIST_HEAD(vmem_freelist, bt) vmem_freelist_t;
typedef LIST_HEAD(vmem_hashlist, bt) vmem_hashlist_t;

/* List of all vmem instances and a guarding mutex */
static MTX_DEFINE(vmem_list_lock, 0);
static LIST_HEAD(, vmem) vmem_list = LIST_HEAD_INITIALIZER(vmem_list);

/*! \brief vmem structure
 *
 * Field markings and the corresponding locks:
 *  (a) vm_lock
 *  (@) vmem_list_lock
 *  (!) read-only access, do not modify!
 */
typedef struct vmem {
  LIST_ENTRY(vmem) vm_link; /* (@) link for vmem_list */
  mtx_t vm_lock;            /* vmem lock */
  size_t vm_size;           /* (a) total size of all added spans */
  size_t vm_inuse;          /* (a) total size of all allocated segments */
  size_t vm_quantum;    /* (!) alignment & the smallest unit of allocation */
  int vm_quantum_shift; /* (!) log2 of vm_quantum */
  char vm_name[VMEM_NAME_MAX]; /* (!) name of vmem instance */
  vmem_seglist_t vm_seglist;   /* (a) list of all segments */
  /* (a) table of lists of free segments */
  vmem_freelist_t vm_freelist[VMEM_MAXORDER];
  /* (a) hashtable of lists of allocated segments */
  vmem_hashlist_t vm_hashlist[VMEM_MAXHASH];
} vmem_t;

typedef enum {
  BT_TYPE_FREE, /* free segment */
  BT_TYPE_BUSY, /* allocated segment */
  BT_TYPE_SPAN  /* segment representing a whole span added by vmem_add() */
} bt_type_t;

/*! \brief boundary tag structure
 *
 * Field markings and the corresponding locks:
 *  (a) vm_lock
 */
typedef struct bt {
  TAILQ_ENTRY(bt) bt_seglink; /* (a) link for vm_seglist */
  union {
    /* (a) link for vm_freelist[] (array index based on bt_size) */
    LIST_ENTRY(bt) bt_freelink;
    /* (a) link for vm_hashlist[] (array index based on bt_start) */
    LIST_ENTRY(bt) bt_hashlink;
  };
  vmem_addr_t bt_start; /* (a) start address of segment represented by bt */
  vmem_size_t bt_size;  /* (a) size of segment represented by bt */
  bt_type_t bt_type;    /* (a) type of segment represented by bt */
} bt_t;

static KMALLOC_DEFINE(M_VMEM, "vmem");
static POOL_DEFINE(P_BT, "vmem boundary tag", sizeof(bt_t));
/* Note: in the future, the amount of static memory for boundary tags should
 * be reduced by more clever tag allocation technique that always keeps some
 * number of free tags. For more information, please see bt_alloc and bt_refill
 * methods in NetBSD's vmem and M_NOGROW flag in Mimiker. */
#define PT_BT_BOOTPAGE_SIZE ((sizeof(void *) / sizeof(int)) * PAGESIZE)
static alignas(PT_BT_BOOTPAGE_SIZE) uint8_t P_BT_BOOTPAGE[PT_BT_BOOTPAGE_SIZE];

void init_vmem(void) {
  pool_add_page(P_BT, P_BT_BOOTPAGE, sizeof(P_BT_BOOTPAGE));
}

static vmem_freelist_t *bt_freehead(vmem_t *vm, vmem_size_t size) {
  vmem_size_t qsize = size >> vm->vm_quantum_shift;
  assert(size != 0 && qsize != 0);
  int idx = SIZE2ORDER(qsize);
  assert(idx >= 0 && idx < VMEM_MAXORDER);
  return &vm->vm_freelist[idx];
}

static vmem_addr_t bt_end(const bt_t *bt) {
  return bt->bt_start + bt->bt_size - 1;
}

static vmem_hashlist_t *bt_hashhead(vmem_t *vm, vmem_addr_t addr) {
  unsigned int hash = hash32_buf(&addr, sizeof(addr), HASH32_BUF_INIT);
  return &vm->vm_hashlist[hash % VMEM_MAXHASH];
}

static void bt_insfree(vmem_t *vm, bt_t *bt) {
  assert(mtx_owned(&vm->vm_lock));
  assert(bt->bt_type == BT_TYPE_FREE);
  vmem_freelist_t *list = bt_freehead(vm, bt->bt_size);
  LIST_INSERT_HEAD(list, bt, bt_freelink);
}

static void bt_remfree(vmem_t *vm, bt_t *bt) {
  assert(mtx_owned(&vm->vm_lock));
  assert(bt->bt_type == BT_TYPE_FREE);
  LIST_REMOVE(bt, bt_freelink);
}

static void bt_insseg_tail(vmem_t *vm, bt_t *bt) {
  assert(mtx_owned(&vm->vm_lock));
  TAILQ_INSERT_TAIL(&vm->vm_seglist, bt, bt_seglink);
}

static void bt_insseg_after(vmem_t *vm, bt_t *bt, bt_t *prev) {
  assert(mtx_owned(&vm->vm_lock));
  TAILQ_INSERT_AFTER(&vm->vm_seglist, prev, bt, bt_seglink);
}

static void bt_remseg(vmem_t *vm, bt_t *bt) {
  assert(mtx_owned(&vm->vm_lock));
  TAILQ_REMOVE(&vm->vm_seglist, bt, bt_seglink);
}

static void bt_insbusy(vmem_t *vm, bt_t *bt) {
  assert(mtx_owned(&vm->vm_lock));
  assert(bt->bt_type == BT_TYPE_BUSY);
  vmem_hashlist_t *list = bt_hashhead(vm, bt->bt_start);
  LIST_INSERT_HEAD(list, bt, bt_hashlink);
  vm->vm_inuse += bt->bt_size;
}

static void bt_rembusy(vmem_t *vm, bt_t *bt) {
  assert(mtx_owned(&vm->vm_lock));
  assert(bt->bt_type == BT_TYPE_BUSY);
  vm->vm_inuse -= bt->bt_size;
  LIST_REMOVE(bt, bt_hashlink);
}

static bt_t *bt_lookupbusy(vmem_t *vm, vmem_addr_t addr) {
  assert(mtx_owned(&vm->vm_lock));
  vmem_hashlist_t *list = bt_hashhead(vm, addr);
  bt_t *bt;
  LIST_FOREACH (bt, list, bt_hashlink) {
    assert(bt->bt_type == BT_TYPE_BUSY);
    if (bt->bt_start == addr)
      return bt;
  }
  return NULL;
}

static bt_t *bt_find_freeseg(vmem_t *vm, vmem_size_t size) {
  assert(mtx_owned(&vm->vm_lock));

  vmem_freelist_t *first = bt_freehead(vm, size);
  vmem_freelist_t *end = &vm->vm_freelist[VMEM_MAXORDER];

  for (vmem_freelist_t *list = first; list < end; list++) {
    bt_t *bt;
    LIST_FOREACH (bt, list, bt_freelink) {
      if (bt->bt_size >= size)
        return bt;
    }
  }
  return NULL;
}

#if VMEM_DEBUG
static bool bt_isspan(const bt_t *bt) {
  return bt->bt_type == BT_TYPE_SPAN;
}

static void vmem_check_sanity(vmem_t *vm) {
  assert(mtx_owned(&vm->vm_lock));

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
#else
#define vmem_check_sanity(vm) (void)vm
#endif

vmem_t *vmem_create(const char *name, vmem_size_t quantum) {
  vmem_t *vm = kmalloc(M_VMEM, sizeof(vmem_t), M_NOWAIT | M_ZERO);
  assert(vm != NULL);

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

  WITH_MTX_LOCK (&vmem_list_lock)
    LIST_INSERT_HEAD(&vmem_list, vm, vm_link);

  klog("new vmem '%s' created", name);
  return vm;
}

int vmem_add(vmem_t *vm, vmem_addr_t addr, vmem_size_t size) {
  bt_t *btspan = pool_alloc(P_BT, M_ZERO);
  bt_t *btfree = pool_alloc(P_BT, M_ZERO);

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

  klog("%s: added [%p-%p] span to '%s'", __func__, addr, addr + size - 1,
       vm->vm_name);

  return 0;
}

int vmem_alloc(vmem_t *vm, vmem_size_t size, vmem_addr_t *addrp,
               kmem_flags_t flags) {
  size = align(size, vm->vm_quantum);
  assert(size > 0);

  /* Allocate new boundary tag before acquiring the vmem lock */
  bt_t *bt, *btnew;

  if (!(btnew = pool_alloc(P_BT, flags | M_ZERO)))
    return ENOMEM;

  WITH_MTX_LOCK (&vm->vm_lock) {
    vmem_check_sanity(vm);

    bt = bt_find_freeseg(vm, size);

    if (bt == NULL) {
      pool_free(P_BT, btnew);
      klog("%s: block of %lu bytes not found in '%s'", __func__, size,
           vm->vm_name);
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
  }

  if (btnew != NULL)
    pool_free(P_BT, btnew);

  assert(bt->bt_size >= size);
  assert(bt->bt_type == BT_TYPE_BUSY);

  if (addrp != NULL)
    *addrp = bt->bt_start;

  klog("%s: found block of %lu bytes in '%s'", __func__, size, vm->vm_name);
  return 0;
}

void vmem_free(vmem_t *vm, vmem_addr_t addr, vmem_size_t size) {
  bt_t *prev = NULL;
  bt_t *next = NULL;

  WITH_MTX_LOCK (&vm->vm_lock) {
    vmem_check_sanity(vm);

    bt_t *bt = bt_lookupbusy(vm, addr);
    assert(bt != NULL);
    assert(bt->bt_size == align(size, vm->vm_quantum));

    bt_rembusy(vm, bt);
    bt->bt_type = BT_TYPE_FREE;

    /* coalesce previous segment */
    prev = TAILQ_PREV(bt, vmem_seglist, bt_seglink);
    if (prev != NULL && prev->bt_type == BT_TYPE_FREE) {
      assert(bt_end(prev) < bt->bt_start);
      bt_remfree(vm, prev);
      bt_remseg(vm, prev);
      bt->bt_size += prev->bt_size;
      bt->bt_start = prev->bt_start;
    } else {
      /* Set prev to NULL so that it won't be deallocated after exiting
       * from the vmem lock */
      prev = NULL;
    }

    /* coalesce next segment */
    next = TAILQ_NEXT(bt, bt_seglink);
    if (next != NULL && next->bt_type == BT_TYPE_FREE) {
      assert(bt_end(bt) < next->bt_start);
      bt_remfree(vm, next);
      bt_remseg(vm, next);
      bt->bt_size += next->bt_size;
    } else {
      /* Set next to NULL so that it won't be deallocated after exiting
       * from the vmem lock */
      next = NULL;
    }

    bt_insfree(vm, bt);

    vmem_check_sanity(vm);
  }

  if (prev != NULL)
    pool_free(P_BT, prev);
  if (next != NULL)
    pool_free(P_BT, next);

  klog("%s: block of %lu bytes deallocated from '%s'", __func__, size,
       vm->vm_name);
}

void vmem_destroy(vmem_t *vm) {
  WITH_MTX_LOCK (&vmem_list_lock)
    LIST_REMOVE(vm, vm_link);

  /* perform last sanity checks */

  /* check #1
   * - segment list is not empty
   */
  assert(!TAILQ_EMPTY(&vm->vm_seglist));

  /* check #2
   * - each segment is either free, or is a span,
   * - total size of all spans is equal to vm_size
   * - vm_inuse is equal to 0
   */
  bt_t *bt;
  vmem_size_t span_size_sum = 0;
  TAILQ_FOREACH (bt, &vm->vm_seglist, bt_seglink) {
    assert(bt->bt_type == BT_TYPE_SPAN || bt->bt_type == BT_TYPE_FREE);
    if (bt->bt_type == BT_TYPE_SPAN)
      span_size_sum += bt->bt_size;
  }
  assert(vm->vm_size == span_size_sum);
  assert(vm->vm_inuse == 0);

  /* check #3
   * - first segment is a span,
   * - each free segment is preceded by a corresponding span segment,
   *   with equal start address and size,
   * - each span segment is preceded by a free segment
   */
  TAILQ_FOREACH (bt, &vm->vm_seglist, bt_seglink) {
    bt_t *prev = TAILQ_PREV(bt, vmem_seglist, bt_seglink);
    if (prev == NULL) {
      assert(bt->bt_type == BT_TYPE_SPAN);
      continue;
    }
    if (bt->bt_type == BT_TYPE_FREE) {
      assert(prev->bt_type == BT_TYPE_SPAN);
      assert(prev->bt_size == bt->bt_size);
      assert(prev->bt_start == bt->bt_start);
    } else if (bt->bt_type == BT_TYPE_SPAN)
      assert(prev->bt_type == BT_TYPE_FREE);
  }

  klog("vmem '%s' destroyed", vm->vm_name);

  /* free the memory */
  bt_t *next;
  TAILQ_FOREACH_SAFE (bt, &vm->vm_seglist, bt_seglink, next)
    pool_free(P_BT, bt);
  kfree(M_VMEM, vm);
}
