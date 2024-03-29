#define KL_LOG KL_VMEM
#include <sys/condvar.h>
#include <sys/klog.h>
#include <sys/kmem.h>
#include <sys/vmem.h>
#include <sys/malloc.h>
#include <sys/mimiker.h>
#include <sys/libkern.h>
#include <sys/errno.h>
#include <sys/hash.h>
#include <sys/mutex.h>
#include <sys/thread.h>
#include <machine/vm_param.h>

#define VMEM_DEBUG 0

#define ORDER2SIZE(order) ((vmem_size_t)1 << (order))
#define SIZE2ORDER(size) ((int)log2(size))

/* List of all vmem instances and a guarding mutex */
static MTX_DEFINE(vmem_list_lock, 0);
static LIST_HEAD(, vmem) vmem_list = LIST_HEAD_INITIALIZER(vmem_list);

typedef enum {
  BT_TYPE_FREE, /* free segment */
  BT_TYPE_BUSY, /* allocated segment */
  BT_TYPE_SPAN  /* segment representing a whole span added by vmem_add() */
} bt_type_t;

/*! \brief boundary tag structure
 *
 * Field markings and the corresponding locks:
 *  (a) vm_lock
 *  (@) bt_lock
 */
typedef struct bt {
  union {
    LIST_ENTRY(bt) bt_alloclink; /* (@) link on bt_alloclist */
    TAILQ_ENTRY(bt) bt_seglink;  /* (a) link for vm_seglist */
  };
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

#define BT_PAGESIZE ((sizeof(void *) / sizeof(int)) * PAGESIZE)
#define BT_PAGECAPACITY (BT_PAGESIZE / sizeof(bt_t))
#define BT_RESERVE_THRESHOLD 3

static MTX_DEFINE(bt_lock, 0);
static LIST_HEAD(, bt) bt_alloclist = LIST_HEAD_INITIALIZER(bt_alloclist);
static size_t bt_freecnt;
static thread_t *bt_refiller;
static condvar_t bt_cv;

static alignas(BT_PAGESIZE) uint8_t bt_bootpage[BT_PAGESIZE];

static void bt_add_page(void *page) {
  bt_t *bt = page;
  for (size_t i = 0; i < BT_PAGECAPACITY; i++, bt++)
    LIST_INSERT_HEAD(&bt_alloclist, bt, bt_alloclink);
  bt_freecnt += BT_PAGECAPACITY;
}

void init_vmem(void) {
  cv_init(&bt_cv, 0);
  bt_add_page(bt_bootpage);
}

static bt_t *bt_alloc(kmem_flags_t flags) {
  SCOPED_MTX_LOCK(&bt_lock);

  thread_t *td = thread_self();

  while ((td != bt_refiller) && (bt_freecnt <= BT_RESERVE_THRESHOLD)) {
    if (!bt_refiller) {
      bt_refiller = td;
      mtx_unlock(&bt_lock);

      void *page = kmem_alloc(BT_PAGESIZE, M_ZERO | M_NOWAIT);
      assert(page);

      mtx_lock(&bt_lock);
      bt_add_page(page);
      bt_refiller = NULL;
      cv_broadcast(&bt_cv);
      break;
    }

    /* If the calling thread cannot sleep, we return `NULL` and the calling
     * function can choose to return an error or loop waiting for the refiller
     * to provide more boundary tags. */
    if (flags & M_NOWAIT)
      return NULL;

    cv_wait(&bt_cv, &bt_lock);
  }

  bt_t *bt = LIST_FIRST(&bt_alloclist);
  assert(bt);
  LIST_REMOVE(bt, bt_alloclink);
  bt_freecnt--;
  return bt;
}

static void bt_free(bt_t *bt) {
  if (!bt)
    return;

  WITH_MTX_LOCK (&bt_lock) {
    LIST_INSERT_HEAD(&bt_alloclist, bt, bt_alloclink);
    bt_freecnt++;
  }
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

void vmem_init(vmem_t *vm, const char *name, vmem_size_t quantum) {
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
}

vmem_t *vmem_create(const char *name, vmem_size_t quantum) {
  vmem_t *vm = kmalloc(M_VMEM, sizeof(vmem_t), M_NOWAIT | M_ZERO);
  assert(vm != NULL);
  vmem_init(vm, name, quantum);
  return vm;
}

vmem_size_t vmem_size(vmem_t *vm, vmem_addr_t addr) {
  SCOPED_MTX_LOCK(&vm->vm_lock);
  bt_t *bt = bt_lookupbusy(vm, addr);
  return bt->bt_size;
}

int vmem_add(vmem_t *vm, vmem_addr_t addr, vmem_size_t size,
             kmem_flags_t flags) {
  bt_t *btspan, *btfree;
  int error = 0;

  if (!(btspan = bt_alloc(flags))) {
    error = EAGAIN;
    goto end;
  }

  if (!(btfree = bt_alloc(flags))) {
    error = EAGAIN;
    goto end;
  }

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

  btspan = NULL;

  klog("%s: added [%p-%p] span to '%s'", __func__, addr, addr + size - 1,
       vm->vm_name);

end:
  bt_free(btspan);
  return error;
}

int vmem_alloc(vmem_t *vm, vmem_size_t size, vmem_addr_t *addrp,
               kmem_flags_t flags) {
  size = align(size, vm->vm_quantum);
  assert(size > 0);

  /* Allocate new boundary tag before acquiring the vmem lock */
  bt_t *bt, *btnew;

  if (!(btnew = bt_alloc(flags)))
    return EAGAIN;

  WITH_MTX_LOCK (&vm->vm_lock) {
    vmem_check_sanity(vm);

    bt = bt_find_freeseg(vm, size);

    if (bt == NULL) {
      bt_free(btnew);
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
    bt_free(btnew);

  assert(bt->bt_size >= size);
  assert(bt->bt_type == BT_TYPE_BUSY);

  if (addrp != NULL)
    *addrp = bt->bt_start;

  klog("%s: found block of %lu bytes in '%s'", __func__, size, vm->vm_name);
  return 0;
}

void vmem_free(vmem_t *vm, vmem_addr_t addr) {
  bt_t *prev = NULL;
  bt_t *next = NULL;
  vmem_size_t size;

  WITH_MTX_LOCK (&vm->vm_lock) {
    vmem_check_sanity(vm);

    bt_t *bt = bt_lookupbusy(vm, addr);
    assert(bt != NULL);

    size = bt->bt_size;

    bt_rembusy(vm, bt);
    bt->bt_type = BT_TYPE_FREE;

    /* coalesce previous segment */
    prev = TAILQ_PREV(bt, vmem_seglist, bt_seglink);
    if (prev != NULL && prev->bt_type == BT_TYPE_FREE) {
      assert(bt_end(prev) + 1 == bt->bt_start);
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
      assert(bt_end(bt) + 1 == next->bt_start);
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
    bt_free(prev);
  if (next != NULL)
    bt_free(next);

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
    bt_free(bt);
  kfree(M_VMEM, vm);
}
