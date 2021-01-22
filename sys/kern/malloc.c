#define KL_LOG KL_KMEM
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/libkern.h>
#include <sys/mutex.h>
#include <sys/malloc.h>
#include <sys/kmem.h>
#include <sys/kasan.h>
#include <sys/queue.h>

#define BLOCK_MAXSIZE (1L << 16) /* Maximum block size. */
#define ARENA_SIZE (1L << 18)    /* Size of each allocated arena. */

static TAILQ_HEAD(, arena) arena_list; /* Queue of managed arenas. */
static mtx_t arena_lock;               /* Mutex protecting arena list. */

#if KASAN
static quar_t block_quarantine;
#endif /* !KASAN */

/* Must be able to hold a pointer and boundary tag. */
typedef unsigned long word_t;

/* Free block consists of header BT, pointer to previous and next free block,
 * payload and footer BT. */
#define FREEBLK_SZ (4 * sizeof(word_t))
/* Used block consists of header BT, user memory and canary. */
#define USEDBLK_SZ (2 * sizeof(word_t))

/* User payload alignment is same as minimum block size */
#define ALIGNMENT FREEBLK_SZ
#define CANARY ((word_t)0x1EE7CAFEDEADC0DE)

/* Maximum free block size is less than 256KiB and minimum is 16, hence possible
 * range of block sizes is 14-bit space, i.e. from 0x10 to 0x3FFF0. Bin sizes
 * were selected based on jemalloc. Following code generates size ranges:
 *
 * > print(list(range(16,128,16)))
 * > for i in range(7, 16):
 * >   print(list(range(2**i, 2**(i+1), 2**(i-2))))
 */
#define NBINS 44

/* Boundary tag flags. */
typedef enum {
  FREE = 0,     /* this block is free */
  USED = 1,     /* this block is used */
  PREVFREE = 2, /* previous block is free */
  ISLAST = 4,   /* last block in an arena */
} bt_flags;

/* Stored in payload of free blocks. */
typedef struct node {
  struct node *prev;
  struct node *next;
} node_t;

/* Structure kept in the header of each managed memory region.
 *
 * Some specific assumptions were made designing this structure.
 * `start` must be: the last item in the arena, adjacent to the end of arena
 * structure. Arena size must be aligned to ALIGNMENT. */
typedef struct arena {
  TAILQ_ENTRY(arena) link; /* link on list of all arenas */
  word_t *end;             /* first address after the arena */
  word_t start[1];         /* first block in the arena (watch alignment!) */
} arena_t;

/* Each bin starts with guard of free block list. */
static node_t freebins[NBINS];

/* --=[ boundary tag handling ]=-------------------------------------------- */

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
  word_t *ft = (void *)bt + size - sizeof(word_t);

#ifndef _LP64
  kasan_mark_valid(bt - 1, 2 * sizeof(word_t));
  kasan_mark_valid(ft, 2 * sizeof(word_t));
#endif

  *bt = val;
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

/* --=[ free blocks handling using doubly linked list ]=-------------------- */

static int bin(size_t x) {
  int n = log2(x);
  switch (n) {
    case 0 ... 6:
      return x / 16 - 1; /* [16, 32, 48, 64, 80, 96, 112] -> 0 - 6 */
    case 7:
      return x / 32 + 3; /* [128, 160, 192, 224] -> 7 - 10 */
    case 8:
      return x / 64 + 7; /* [256, 320, 384, 448] -> 11 - 14 */
    case 9:
      return x / 128 + 11; /* [512, 640, 768, 896] -> 15 - 18 */
    case 10:
      return x / 256 + 15; /* [1024, 1280, 1536, 1792] -> 19 - 22 */
    case 11:
      return x / 512 + 19; /* [2048, 2560, 3072, 3584] -> 23 - 26 */
    case 12:
      return x / 1024 + 23; /* [4 KiB, 5 KiB, 6 KiB, 7 KiB] -> 27 - 30 */
    case 13:
      return x / 2048 + 27; /* [8 KiB, 10 KiB, 12 KiB, 14 KiB] -> 31 - 34 */
    case 14:
      return x / 4096 + 31; /* [16 KiB, 20 KiB, 24 KiB, 28 KiB] -> 35 - 38 */
    case 15:
      return x / 8192 + 35; /* [32 KiB, 40 KiB, 48 KiB, 54 KiB] -> 39 - 42 */
    default:
      return NBINS - 1; /* [64 KiB - 256 KiB] -> 43 */
  }
}

static inline void free_insert(word_t *bt) {
  node_t *head = &freebins[bin(bt_size(bt))];
  node_t *node = bt_payload(bt);
  node_t *prev = head->prev;

  /* Unpoison node pointers. */
  kasan_mark_valid(node, sizeof(node_t));

  /* Insert at the end of free block list. */
  node->next = head;
  node->prev = prev;
  prev->next = node;
  head->prev = node;
}

static inline void free_remove(word_t *bt) {
  node_t *node = bt_payload(bt);
  node_t *prev = node->prev;
  node_t *next = node->next;
  prev->next = next;
  next->prev = prev;

  /* Poison node pointers. */
  kasan_mark_invalid(node, sizeof(node_t), KASAN_CODE_KMALLOC_OVERFLOW);
}

static inline size_t blk_size(size_t size) {
  return roundup(size + USEDBLK_SZ, ALIGNMENT);
}

/* --=[ algorithm implementation ]=----------------------------------------- */

/* First fit */
static word_t *find_fit(size_t reqsz) {
  for (int b = bin(reqsz); b < NBINS; b++) {
    node_t *head = &freebins[b];
    for (node_t *n = head->next; n != head; n = n->next) {
      word_t *bt = bt_fromptr(n);
      if (bt_size(bt) >= reqsz)
        return bt;
    }
  }
  return NULL;
}

static void *malloc(size_t reqsz) {
  assert(is_aligned(reqsz, ALIGNMENT));

  word_t *bt = find_fit(reqsz);
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
      free_insert(next);
    } else if (!is_last) {
      /* Nothing to split? Then previous block is not free anymore! */
      bt_clr_prevfree(next);
    }
  }

  return bt ? bt_payload(bt) : NULL;
}

static void free(void *ptr) {
  word_t *bt = bt_fromptr(ptr);

  /* Is block free and has canary? */
  assert(bt_used(bt));
  assert(bt_has_canary(bt));

  /* Mark block as free. */
  size_t memsz = bt_size(bt) - USEDBLK_SZ;
  size_t sz = bt_size(bt);
  bt_make(bt, sz, FREE | bt_get_flags(bt));

  word_t *next = bt_get_islast(bt) ? NULL : bt_next(bt);
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

  free_insert(bt);
}

static void *realloc(void *old_ptr, size_t size) {
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
    free(bt_payload(next));
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
          free_insert(next);
        } else {
          bt_clr_prevfree(next);
        }
        new_ptr = old_ptr;
      }
    }
  }

  return new_ptr;
}

/* --=[ kernel API ]=------------------------------------------------------- */

static void arena_init(arena_t *ar) {
  size_t sz = rounddown(ARENA_SIZE - sizeof(arena_t), ALIGNMENT);

  /* Poison everything but arena header. */
  kasan_mark(ar, offsetof(arena_t, start), ARENA_SIZE,
             KASAN_CODE_KMALLOC_FREED);

  ar->end = (void *)ar->start + sz;

  word_t *bt = ar->start;
  bt_make(bt, sz, FREE | ISLAST);
  free_insert(bt);

  TAILQ_INSERT_TAIL(&arena_list, ar, link);
  klog("%s: add arena %08x - %08x", __func__, ar->start, ar->end);
}

static void arena_add(void) {
  arena_t *ar = kmem_alloc(ARENA_SIZE, M_WAITOK);
  if (ar == NULL)
    panic("memory exhausted!");
  arena_init(ar);
}

void *kmalloc(kmalloc_pool_t *mp, size_t size, unsigned flags) {
  if (size == 0)
    return NULL;

#if KASAN
  size_t req_size = blk_size(size + KASAN_KMALLOC_REDZONE_SIZE);
#else /* !KASAN */
  size_t req_size = blk_size(size);
#endif

  void *ptr = NULL;

  assert(req_size <= BLOCK_MAXSIZE);

  WITH_MTX_LOCK (&arena_lock) {
    while (!(ptr = malloc(req_size))) {
      /* Couldn't find any continuous memory with the requested size. */
      if (flags & M_NOWAIT)
        return NULL;
      arena_add();
    }

    mp->used += req_size;
    mp->maxused = max(mp->used, mp->maxused);
    mp->active++;
    mp->nrequests++;
  }

  /* Create redzone after the buffer. */
  kasan_mark(ptr, size, req_size - USEDBLK_SZ, KASAN_CODE_KMALLOC_OVERFLOW);
  if (flags == M_ZERO)
    bzero(ptr, size);
  return ptr;
}

static void kfree_nokasan(kmalloc_pool_t *mp, void *ptr) {
  assert(mtx_owned(&arena_lock));
  word_t *bt = bt_fromptr(ptr);
  mp->used -= bt_size(bt);
  mp->active--;
  free(ptr);
}

void kfree(kmalloc_pool_t *mp, void *ptr) {
  if (ptr == NULL)
    return;

  WITH_MTX_LOCK (&arena_lock) {
#if KASAN
    word_t *bt = bt_fromptr(ptr);
    kasan_mark_invalid(ptr, bt_size(bt) - USEDBLK_SZ, KASAN_CODE_KMALLOC_FREED);
    kasan_quar_additem(&block_quarantine, mp, ptr);
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
    if ((new_ptr = realloc(old_ptr, size)))
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

void kmcheck(void) {
  arena_t *ar = NULL;
  unsigned dangling = 0;

  TAILQ_FOREACH (ar, &arena_list, link) {
    word_t *bt = ar->start;
    word_t *prev = NULL;
    int prevfree = 0;

    for (; bt < ar->end; prev = bt, bt = bt_next(bt)) {
      int flag = !!bt_get_prevfree(bt);
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
  }

  for (int b = 0; b < NBINS; b++) {
    node_t *head = &freebins[b];
    for (node_t *n = head->next; n != head; n = n->next) {
      word_t *bt = bt_fromptr(n);
      assert(bt_free(bt));
      dangling--;
    }
  }

  assert(dangling == 0);
}

static alignas(PAGESIZE) uint8_t BOOT_ARENA[ARENA_SIZE];

void init_kmalloc(void) {
  TAILQ_INIT(&arena_list);
  mtx_init(&arena_lock, 0);
  kasan_quar_init(&block_quarantine, (quar_free_t)kfree_nokasan);
  for (int b = 0; b < NBINS; b++) {
    node_t *head = &freebins[b];
    head->next = head;
    head->prev = head;
  }
  arena_init((arena_t *)BOOT_ARENA);
}

KMALLOC_DEFINE(M_TEMP, "temporaries");
KMALLOC_DEFINE(M_STR, "strings");
