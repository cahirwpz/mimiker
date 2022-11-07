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

#define KM_ALIGNMENT 16

#define KM_SLAB_MINBLKSZ 16
#define KM_SLAB_MAXBLKSZ (1 << 13)

#define KM_NPOOLS (ffs(KM_SLAB_MAXBLKSZ) - ffs(KM_SLAB_MINBLKSZ) + 1)

static pool_t km_pools[KM_NPOOLS];

/* clang-format off */
static const struct {
  const char *desc;
  size_t size;
} km_slab_cfg[] = {
  [0] = { "kmalloc bin 16",     8192 }, /* blkcnt= 512 */
  [1] = { "kmalloc bin 32",    16384 }, /* blkcnt= 512 */
  [2] = { "kmalloc bin 64",    16384 }, /* blkcnt= 256 */
  [3] = { "kmalloc bin 128",   32768 }, /* blkcnt= 256 */
  [4] = { "kmalloc bin 256",   32768 }, /* blkcnt= 128 */
  [5] = { "kmalloc bin 512",   65536 }, /* blkcnt= 128 */
  [6] = { "kmalloc bin 1024",  65536 }, /* blkcnt=  64 */
  [7] = { "kmalloc bin 2048", 131072 }, /* blkcnt=  64 */
  [8] = { "kmalloc bin 4096", 131072 }, /* blkcnt=  32 */
  [9] = { "kmalloc bin 8192", 262144 }, /* blkcnt=  32 */
};
/* clang-format on */

#define KM_BOOT_AREASZ 761856

/*
 * Auxiliary functions.
 */

static inline size_t blk_idx(size_t size) {
  if (size <= KM_SLAB_MINBLKSZ)
    return 0;
  return 1 + log2(size - 1) - log2(KM_SLAB_MINBLKSZ);
}

static inline slab_t *blk_slab(void *ptr) {
  vm_page_t *pg = kva_find_page((vaddr_t)ptr);
  return pg->slab;
}

/*
 * Kernel API.
 */

void *kmalloc(kmalloc_pool_t *mp, size_t size, kmem_flags_t flags) {
  if (size == 0)
    return NULL;

#if KASAN
  size_t req_size = size + KASAN_KMALLOC_REDZONE_SIZE;
#else /* !KASAN */
  size_t req_size = size;
#endif

  size_t blksz;
  void *ptr;

  if (size > KM_SLAB_MAXBLKSZ) {
    blksz = roundup2(req_size, PAGESIZE);
    ptr = kmem_alloc(blksz, flags);
  } else {
    size_t idx = blk_idx(req_size);
    assert(idx < KM_NPOOLS);

    blksz = KM_SLAB_MINBLKSZ << idx;
    ptr = pool_alloc(&km_pools[idx], flags);
  }

  if (!ptr)
    return NULL;

  assert(is_aligned(ptr, KM_ALIGNMENT));

  kasan_mark(ptr, size, blksz, KASAN_CODE_KMALLOC_OVERFLOW);

  WITH_MTX_LOCK (&mp->lock) {
    mp->nrequests++;
    mp->used += blksz;
    mp->maxused = max(mp->used, mp->maxused);
    mp->active++;
  }

  return ptr;
}

void kfree(kmalloc_pool_t *mp, void *ptr) {
  if (!ptr)
    return;

  size_t blksz;
  slab_t *slab;

  if ((slab = blk_slab(ptr))) {
    pool_t *pool = slab->ph_pool;

    blksz = slab->ph_itemsize;
#if KASAN
    blksz -= pool->pp_redzone;
#endif

    pool_free(pool, ptr);
  } else {
    blksz = kmem_size(ptr);
    kmem_free(ptr, blksz);
  }

  WITH_MTX_LOCK (&mp->lock) {
    mp->used -= blksz;
    mp->active--;
  }
}

char *kstrndup(kmalloc_pool_t *mp, const char *s, size_t maxlen) {
  size_t n = strnlen(s, maxlen);
  char *copy = kmalloc(mp, n + 1, 0);
  memcpy(copy, s, n);
  copy[n] = '\0';
  return copy;
}

void init_kmalloc(void) {
  for (size_t i = 0; i < KM_NPOOLS; i++) {
    pool_t *pool = &km_pools[i];
    const char *desc = km_slab_cfg[i].desc;
    size_t slabsz = km_slab_cfg[i].size;
    size_t blksz = KM_SLAB_MINBLKSZ << i;

    pool_init(pool, desc, blksz, KM_ALIGNMENT, slabsz);
  }
}

KMALLOC_DEFINE(M_TEMP, "temporaries");
KMALLOC_DEFINE(M_STR, "strings");
