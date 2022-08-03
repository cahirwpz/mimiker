#define KL_LOG KL_KMEM
#include <sys/kasan.h>
#include <sys/klog.h>
#include <sys/kmem.h>
#include <sys/libkern.h>
#include <sys/malloc.h>
#include <sys/mimiker.h>
#include <sys/pool.h>
#include <sys/vm.h>
#include <machine/vm_param.h>

#define ALIGNMENT sizeof(long)

#define SLAB_MINBLKSZ 16
#define SLAB_MAXBLKSZ (1 << 16)

#define NPOOLS (ffs(SLAB_MAXBLKSZ) - ffs(SLAB_MINBLKSZ) + 1)

#define CANARY ((u_long)0x1EE7CAFEDEADC0DE)
#define KASAN_CANARY_RANGE max(sizeof(u_long), (size_t)KASAN_SHADOW_SCALE_SIZE)

/* Large block header. */
typedef struct {
  union {
    size_t size;
    uint8_t _pad[ALIGNMENT];
  };
} lblk_hdr_t;

static pool_t pools[NPOOLS];
static mtx_t mplock;

/* clang-format off */
static const size_t slab_sizes[] = {
  [0] =    4096, /* blksz=    16  blkcnt= 256 */
  [1] =    4096, /* blksz=    32  blkcnt= 128 */
  [2] =    4096, /* blksz=    64  blkcnt=  64 */
  [3] =    8192, /* blksz=   128  blkcnt=  64 */
  [4] =    8192, /* blksz=   256  blkcnt=  32 */
  [5] =   16384, /* blksz=   512  blkcnt=  32 */
  [6] =   16384, /* blksz=  1024  blkcnt=  16 */
  [7] =   32768, /* blksz=  2048  blkcnt=  16 */
  [8] =   32768, /* blksz=  4096  blkcnt=   8 */
  [9] =   65536, /* blksz=  8192  blkcnt=   8 */
  [10] =  65536, /* blksz= 16384  blkcnt=   4 */
  [11] = 131072, /* blksz= 32768  blkcnt=   4 */
  [12] = 131072, /* blksz= 65536  blkcnt=   2 */
};
/* clang-format on */

#define BOOT_AREASZ 520192

/*
 * Large block allocator.
 */

static inline size_t lblk_size(size_t size) {
  /* header + payload + canary */
  return roundup2(size + sizeof(lblk_hdr_t) + sizeof(u_long), PAGESIZE);
}

static inline void *lblk_payload(lblk_hdr_t *hdr) {
  return (void *)hdr + sizeof(lblk_hdr_t);
}

static inline u_long *lblk_canary(lblk_hdr_t *hdr) {
  return (void *)hdr + hdr->size - sizeof(u_long);
}

static inline lblk_hdr_t *lblk_hdr_fromptr(void *ptr) {
  return ptr - sizeof(lblk_hdr_t);
}

static void *lblk_alloc(kmalloc_pool_t *mp, size_t size, kmem_flags_t flags) {
  assert(size);

#if KASAN
  size_t req_size = lblk_size(size + KASAN_KMALLOC_REDZONE_SIZE);
#else /* !KASAN */
  size_t req_size = lblk_size(size);
#endif

  void *ptr = NULL;

  lblk_hdr_t *hdr = kmem_alloc(req_size, flags);
  if (!hdr)
    goto end;

  hdr->size = req_size;

  u_long *canaryp = lblk_canary(hdr);
  *canaryp = CANARY;

  ptr = lblk_payload(hdr);

  kasan_mark(ptr, size, req_size - KASAN_CANARY_RANGE,
             KASAN_CODE_KMALLOC_OVERFLOW);

end:
  WITH_MTX_LOCK (&mplock) {
    mp->nrequests++;
    if (!ptr)
      return NULL;
    mp->used += req_size;
    mp->maxused = max(mp->used, mp->maxused);
    mp->active++;
  }
  return ptr;
}

void lblk_free(kmalloc_pool_t *mp, void *ptr) {
  assert(ptr);

  lblk_hdr_t *hdr = lblk_hdr_fromptr(ptr);

  u_long *canaryp = lblk_canary(hdr);
  assert(*canaryp == CANARY);

  size_t blksz = hdr->size;
  kmem_free(hdr, blksz);

  WITH_MTX_LOCK (&mplock) {
    mp->used -= blksz;
    mp->active--;
  }
}

/*
 * Slab block allocator.
 */

static inline size_t blk_size(size_t size) {
  return max(pow2(size), (size_t)SLAB_MINBLKSZ);
}

size_t blk_idx(size_t size) {
  assert(powerof2(size));
  return ffs(size) - 1 - log2(SLAB_MINBLKSZ);
}

static slab_t *blk_slab(void *ptr) {
  assert(ptr);
  vm_page_t *pg = kva_find_page((vaddr_t)ptr);
  return pg->slab;
}

static void *blk_alloc(kmalloc_pool_t *mp, size_t size, kmem_flags_t flags) {
  assert(size);

  size_t req_size = blk_size(size);
  size_t idx = blk_idx(req_size);
  assert(idx < NPOOLS);

  pool_t *pool = &pools[idx];
  void *ptr = pool_alloc(pool, flags);

#if KASAN
  size_t used = req_size + pool->pp_redzone;
#else /* !KASAN */
  size_t used = req_size;
#endif

  WITH_MTX_LOCK (&mplock) {
    mp->nrequests++;
    if (!ptr)
      return NULL;
    mp->used += used;
    mp->maxused = max(mp->used, mp->maxused);
    mp->active++;
  }
  return ptr;
}

static void blk_free(kmalloc_pool_t *mp, void *ptr) {
  assert(ptr);

  slab_t *slab = blk_slab(ptr);
  pool_t *pool = slab->ph_pool;

  pool_free(pool, ptr);

  WITH_MTX_LOCK (&mplock) {
    mp->used -= slab->ph_itemsize;
    mp->active--;
  }
}

/*
 * Kernel API.
 */

static inline bool is_large(void *ptr) {
  return !blk_slab(ptr);
}

void *kmalloc(kmalloc_pool_t *mp, size_t size, kmem_flags_t flags) {
  if (size == 0)
    return NULL;

  void *ptr = NULL;

  if (size > SLAB_MAXBLKSZ)
    ptr = lblk_alloc(mp, size, flags);
  else
    ptr = blk_alloc(mp, size, flags);

  if (!ptr)
    return NULL;

  assert(is_aligned(ptr, ALIGNMENT));

  if (flags & M_ZERO)
    memset(ptr, 0, size);
  return ptr;
}

void kfree(kmalloc_pool_t *mp, void *ptr) {
  if (!ptr)
    return;

  if (is_large(ptr))
    lblk_free(mp, ptr);
  else
    blk_free(mp, ptr);
}

char *kstrndup(kmalloc_pool_t *mp, const char *s, size_t maxlen) {
  size_t n = strnlen(s, maxlen);
  char *copy = kmalloc(mp, n + 1, 0);
  memcpy(copy, s, n);
  copy[n] = '\0';
  return copy;
}

static alignas(PAGESIZE) uint8_t boot_area[BOOT_AREASZ];

void init_kmalloc(void) {
  mtx_init(&mplock, 0);

  void *slab_pages = boot_area;

  for (size_t i = 0; i < NPOOLS; i++) {
    pool_t *pool = &pools[i];
    size_t blksz = SLAB_MINBLKSZ << i;
    size_t slabsz = slab_sizes[i];

    pool_init(pool, "bin", blksz, ALIGNMENT, slabsz);
    pool_add_page(pool, slab_pages, slabsz);

    slab_pages += slabsz;
  }

  assert(slab_pages == &boot_area[BOOT_AREASZ]);
}

KMALLOC_DEFINE(M_TEMP, "temporaries");
KMALLOC_DEFINE(M_STR, "strings");
