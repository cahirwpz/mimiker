#define KL_LOG KL_KMEM
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/libkern.h>
#include <sys/mutex.h>
#include <sys/malloc.h>
#include <sys/kmem.h>
#include <sys/pool.h>
#include <sys/errno.h>
#include <sys/kasan.h>
#include <sys/queue.h>

#define BLOCK_MAXSIZE (PAGESIZE * 16) /* Maximum block size. */
#define ARENA_SIZE (PAGESIZE * 64)    /* Size of each allocated arena. */

static TAILQ_HEAD(, arena) arena_list; /* Queue of managed arenas. */
static mtx_t arena_lock;               /* Mutex protecting arena list. */

#if KASAN
static quar_t block_quarantine;
#endif /* !KASAN */

typedef uintptr_t word_t;

#define ALIGNMENT (4 * sizeof(word_t))
#define CANARY 0xDEADC0DE

/* Free block consists of header BT, pointer to previous and next free block,
 * payload and footer BT. */
#define FREEBLK_SZ (4 * sizeof(word_t))
/* Used block consists of header BT, user memory and canary. */
#define USEDBLK_SZ (2 * sizeof(word_t))

/* Boundary tag flags. */
typedef enum {
  FREE = 0,     /* this block is free */
  USED = 1,     /* this block is used */
  PREVFREE = 2, /* previous block is free */
  ISLAST = 4,   /* last block in an arena */
} bt_flags;

/* Stored in payload of free blocks. */
typedef struct fbnode {
  struct fbnode *prev;
  struct fbnode *next;
} fbnode_t;

/* Structure kept in the header of each managed memory region. */
typedef struct arena {
  TAILQ_ENTRY(arena) link; /* link on list of all arenas */
  fbnode_t freelst;        /* guard of free block list */
  word_t *end;             /* first address after the arena */
  word_t pad[2];           /* make user address aligned to ALIGNMENT */
  word_t start[];          /* first block in the arena */
} arena_t;

static inline word_t bt_size(word_t *bt) {
  return *bt & ~(USED | PREVFREE | ISLAST);
}

static inline int bt_used(word_t *bt) {
  return *bt & USED;
}

static inline int bt_free(word_t *bt) {
  return !(*bt & USED);
}

static inline word_t *bt_footer(word_t *bt) {
  return (void *)bt + bt_size(bt) - sizeof(word_t);
}

static inline word_t *bt_fromptr(void *ptr) {
  return (word_t *)ptr - 1;
}

static inline __always_inline void bt_make(word_t *bt, size_t size,
                                           bt_flags flags) {
  word_t val = size | flags;
  *bt = val;
  word_t *ft = (void *)bt + size - sizeof(word_t);
  *ft = (flags & USED) ? CANARY : val;
}

static inline int bt_has_canary(word_t *bt) {
  return *bt_footer(bt) == CANARY;
}

static inline bt_flags bt_get_flags(word_t *bt) {
  return *bt & (PREVFREE | ISLAST);
}

static inline bt_flags bt_get_prevfree(word_t *bt) {
  return *bt & PREVFREE;
}

static inline bt_flags bt_get_islast(word_t *bt) {
  return *bt & ISLAST;
}

static inline void bt_clr_islast(word_t *bt) {
  *bt &= ~ISLAST;
}

static inline void bt_clr_prevfree(word_t *bt) {
  *bt &= ~PREVFREE;
}

static inline void bt_set_prevfree(word_t *bt) {
  *bt |= PREVFREE;
}

static inline void *bt_payload(word_t *bt) {
  return bt + 1;
}

static inline word_t *bt_next(word_t *bt) {
  return (void *)bt + bt_size(bt);
}

/* Must never be called on first block in an arena. */
static inline word_t *bt_prev(word_t *bt) {
  word_t *ft = bt - 1;
  return (void *)bt - bt_size(ft);
}

static inline void ar_free_insert(arena_t *ar, word_t *bt) {
  fbnode_t *head = &ar->freelst;
  fbnode_t *node = bt_payload(bt);
  fbnode_t *prev = head->prev;

  /* Insert at the end of free block list. */
  node->next = head;
  node->prev = prev;
  prev->next = node;
  head->prev = node;
}

static inline void free_remove(word_t *bt) {
  fbnode_t *node = bt_payload(bt);
  fbnode_t *prev = node->prev;
  fbnode_t *next = node->next;
  prev->next = next;
  next->prev = prev;
}

static inline size_t blk_size(size_t size) {
  return roundup(size + USEDBLK_SZ, ALIGNMENT);
}

/* First fit */
static word_t *find_fit(arena_t *ar, size_t reqsz) {
  fbnode_t *head = &ar->freelst;
  for (fbnode_t *n = head->next; n != head; n = n->next) {
    word_t *bt = bt_fromptr(n);
    if (bt_size(bt) >= reqsz)
      return bt;
  }
  return NULL;
}

static void ar_init(arena_t *ar, void *end) {
  size_t sz = end - (void *)ar->start;
  fbnode_t *head = &ar->freelst;

  sz &= -ALIGNMENT;

  head->next = head;
  head->prev = head;
  ar->end = (void *)ar->start + sz;
  word_t *bt = ar->start;
  bt_make(bt, sz, FREE | ISLAST);
  ar_free_insert(ar, bt);
}

static void *ar_malloc(arena_t *ar, size_t reqsz) {
  assert(is_aligned(reqsz, ALIGNMENT));

  word_t *bt = find_fit(ar, reqsz);
  if (bt != NULL) {
    bt_flags is_last = bt_get_islast(bt);
    size_t memsz = reqsz - USEDBLK_SZ;
    /* Mark found block as used. */
    size_t sz = bt_size(bt);
    free_remove(bt);
    bt_make(bt, reqsz, USED | is_last);
    /* Split free block if needed. */
    word_t *next = bt_next(bt);
    if (sz > reqsz) {
      bt_make(next, sz - reqsz, FREE | is_last);
      bt_clr_islast(bt);
      memsz += USEDBLK_SZ;
      ar_free_insert(ar, next);
    } else if (!is_last) {
      /* Nothing to split? Then previous block is not free anymore! */
      bt_clr_prevfree(next);
    }
  }

  return bt ? bt_payload(bt) : NULL;
}

static void ar_free(arena_t *ar, void *ptr) {
  word_t *bt = bt_fromptr(ptr);

  /* Is block free and has canary? */
  assert(bt_used(bt));
  assert(bt_has_canary(bt));

  /* Mark block as free. */
  size_t memsz = bt_size(bt) - USEDBLK_SZ;
  size_t sz = bt_size(bt);
  bt_make(bt, sz, FREE | bt_get_flags(bt));

  word_t *next = bt_next(bt);
  if (next) {
    if (bt_free(next)) {
      /* Coalesce with next block. */
      free_remove(next);
      sz += bt_size(next);
      bt_make(bt, sz, FREE | bt_get_prevfree(bt) | bt_get_islast(next));
      memsz += USEDBLK_SZ;
    } else {
      /* Mark next used block with prevfree flag. */
      bt_set_prevfree(next);
    }
  }

  /* Check if can coalesce with previous block. */
  if (bt_get_prevfree(bt)) {
    word_t *prev = bt_prev(bt);
    free_remove(prev);
    sz += bt_size(prev);
    bt_make(prev, sz, FREE | bt_get_islast(bt));
    memsz += USEDBLK_SZ;
    bt = prev;
  }

  ar_free_insert(ar, bt);
}

static void *ar_realloc(arena_t *ar, void *old_ptr, size_t size) {
  void *new_ptr = NULL;
  word_t *bt = bt_fromptr(old_ptr);
  size_t reqsz = blk_size(size);
  size_t sz = bt_size(bt);

  if (reqsz == sz) {
    /* Same size: nothing to do. */
    return old_ptr;
  }

  if (reqsz < sz) {
    bt_flags is_last = bt_get_islast(bt);
    /* Shrink block: split block and free second one. */
    bt_make(bt, reqsz, USED | bt_get_prevfree(bt));
    word_t *next = bt_next(bt);
    bt_make(next, sz - reqsz, USED | is_last);
    ar_free(ar, bt_payload(next));
    new_ptr = old_ptr;
  } else {
    /* Expand block */
    word_t *next = bt_next(bt);
    if (next && bt_free(next)) {
      /* Use next free block if it has enough space. */
      bt_flags is_last = bt_get_islast(next);
      size_t nextsz = bt_size(next);
      if (sz + nextsz >= reqsz) {
        free_remove(next);
        bt_make(bt, reqsz, USED | bt_get_prevfree(bt));
        word_t *next = bt_next(bt);
        if (sz + nextsz > reqsz) {
          bt_make(next, sz + nextsz - reqsz, FREE | is_last);
          ar_free_insert(ar, next);
        } else {
          bt_clr_prevfree(next);
        }
        new_ptr = old_ptr;
      }
    }
  }

  return new_ptr;
}

#define msg(...)                                                               \
  if (verbose) {                                                               \
    kprintf(__VA_ARGS__);                                                      \
  }

static void ar_check(arena_t *ar, int verbose) {
  word_t *bt = ar->start;

  word_t *prev = NULL;
  int prevfree = 0;
  unsigned dangling = 0;

  msg("--=[ all block list ]=---\n");

  for (; bt < ar->end; prev = bt, bt = bt_next(bt)) {
    int flag = !!bt_get_prevfree(bt);
    int is_last = !!bt_get_islast(bt);
    msg("%p: [%c%c:%d] %c\n", bt, "FU"[bt_used(bt)], " P"[flag], bt_size(bt),
        " *"[is_last]);
    if (bt_free(bt)) {
      word_t *ft = bt_footer(bt);
      assert(*bt == *ft); /* Header and footer do not match? */
      assert(!prevfree);  /* Free block not coalesced? */
      prevfree = 1;
      dangling++;
    } else {
      assert(flag == prevfree);  /* PREVFREE flag mismatch? */
      assert(bt_has_canary(bt)); /* Canary damaged? */
      prevfree = 0;
    }
  }

  assert(bt_get_islast(prev)); /* Last block set incorrectly? */

  msg("--=[ free block list start ]=---\n");

  fbnode_t *head = &ar->freelst;
  for (fbnode_t *n = head->next; n != head; n = n->next) {
    word_t *bt = bt_fromptr(n);
    msg("%p: [%p, %p]\n", n, n->prev, n->next);
    assert(bt_free(bt));
    dangling--;
  }

  msg("--=[ free block list end ]=---\n");

  assert(dangling == 0 && "Dangling free blocks!");
}

static arena_t *ar_find(void *ptr) {
  arena_t *ar = NULL;
  TAILQ_FOREACH (ar, &arena_list, link) {
    if (ptr >= (void *)ar->start && ptr < (void *)ar->end)
      break;
  }
  assert(ar != NULL);
  return ar;
}

/* ========================================================================== */

static void kmalloc_add_arena(void) {
  arena_t *ar = kmem_alloc(ARENA_SIZE, M_WAITOK);
  if (ar == NULL)
    panic("memory exhausted!");

  ar_init(ar, (void *)ar + ARENA_SIZE);
  TAILQ_INSERT_TAIL(&arena_list, ar, link);
  klog("%s: add arena %08x - %08x", __func__, ar->start, ar->end);
}

void *kmalloc(kmalloc_pool_t *mp, size_t size, unsigned flags) {
  if (size == 0)
    return NULL;

#if KASAN
  /* the alignment is within the redzone */
  size_t redzone_size =
    align(size, ALIGNMENT) - size + KASAN_KMALLOC_REDZONE_SIZE;
#endif /* !KASAN */

  size_t req_size = blk_size(size);
#if KASAN
  req_size += redzone_size;
#endif /* !KASAN */

  void *ptr = NULL;

  assert(req_size <= BLOCK_MAXSIZE);

  for (;;) {
    WITH_MTX_LOCK (&arena_lock) {
      /* Search for the first arena in the list that has enough space. */
      arena_t *ar = NULL;
      TAILQ_FOREACH (ar, &arena_list, link) {
        if ((ptr = ar_malloc(ar, req_size)))
          goto found;
      }

      /* Couldn't find any continuous memory with the requested size. */
      if (flags & M_NOWAIT)
        return NULL;

      kmalloc_add_arena();
    }
  }

found:
  /* Create redzone after the buffer */
  kasan_mark(ptr, size, size + redzone_size, KASAN_CODE_KMALLOC_OVERFLOW);
  if (flags == M_ZERO)
    bzero(ptr, size);
  return ptr;
}

static void kfree_nokasan(kmalloc_pool_t *mp, void *ptr) {
  assert(mtx_owned(&arena_lock));
  ar_free(ar_find(ptr), ptr);
}

void kfree(kmalloc_pool_t *mp, void *ptr) {
  if (ptr == NULL)
    return;

  WITH_MTX_LOCK (&arena_lock) {
#if KASAN
    word_t *bt = bt_fromptr(ptr);
    kasan_mark_invalid(ptr, bt_size(bt), KASAN_CODE_KMALLOC_FREED);
    kasan_quar_additem(&block_quarantine, ptr);
#else
    kfree_nokasan(mp, ptr);
#endif /* !KASAN */
  }
}

void *krealloc(kmalloc_pool_t *mp, void *old_ptr, size_t size, unsigned flags) {
  void *new_ptr;

  if (size == 0) {
    kfree(mp, old_ptr);
    return NULL;
  }

  if (old_ptr == NULL)
    return kmalloc(mp, size, flags);

  WITH_MTX_LOCK (&arena_lock) {
    if ((new_ptr = ar_realloc(ar_find(old_ptr), old_ptr, size)))
      return new_ptr;
  }

  /* Run out of options - need to move block physically. */
  if ((new_ptr = kmalloc(mp, size, flags))) {
    word_t *bt = bt_fromptr(old_ptr);
    memcpy(new_ptr, old_ptr, bt_size(bt) - sizeof(word_t));
    kfree(mp, old_ptr);
    return new_ptr;
  }

  return NULL;
}

char *kstrndup(kmalloc_pool_t *mp, const char *s, size_t maxlen) {
  size_t n = strnlen(s, maxlen) + 1;
  char *copy = kmalloc(mp, n, M_ZERO);
  memcpy(copy, s, n);
  return copy;
}

void kmcheck(int verbose) {
  arena_t *ar = NULL;
  TAILQ_FOREACH (ar, &arena_list, link)
    ar_check(ar, verbose);
}

void init_kmalloc(void) {
  TAILQ_INIT(&arena_list);
  mtx_init(&arena_lock, 0);
  kasan_quar_init(&block_quarantine, NULL, (quar_free_t)kfree_nokasan);
}

KMALLOC_DEFINE(M_TEMP, "temporaries pool");
KMALLOC_DEFINE(M_STR, "strings");
