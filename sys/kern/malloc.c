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
static mtx_t km_lock; /* guards `kmalloc_pool_t` structures */

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

static void mp_update_alloc(kmalloc_pool_t *mp, void *ptr, size_t blksz) {
  SCOPED_MTX_LOCK(&km_lock);
  mp->nrequests++;
  if (!ptr)
    return;
  mp->used += blksz;
  mp->maxused = max(mp->used, mp->maxused);
  mp->active++;
}

static void mp_update_free(kmalloc_pool_t *mp, size_t blksz) {
  SCOPED_MTX_LOCK(&km_lock);
  assert(blksz);
  mp->used -= blksz;
  mp->active--;
}

/*
 * Large block allocator.
 */

static inline size_t lblk_size(size_t size) {
  return roundup2(size, PAGESIZE);
}

static void *lblk_alloc(size_t size, kmem_flags_t flags, size_t *blkszp) {
  assert(size);

#if KASAN
  size_t blksz = lblk_size(size + KASAN_KMALLOC_REDZONE_SIZE);
#else /* !KASAN */
  size_t blksz = lblk_size(size);
#endif

  void *ptr = kmem_alloc(blksz, flags);
  if (!ptr)
    return NULL;

  kasan_mark(ptr, size, blksz, KASAN_CODE_KMALLOC_OVERFLOW);

  *blkszp = blksz;
  return ptr;
}

void lblk_free(void *ptr, size_t *blkszp) {
  assert(ptr);

  size_t blksz = kmem_size(ptr);
  *blkszp = blksz;

  kmem_free(ptr, blksz);
}

/*
 * Slab block allocator.
 */

static inline size_t blk_idx(size_t size) {
  if (size <= KM_SLAB_MINBLKSZ)
    return 0;
  size_t clg = powerof2(size) ? log2(size) : log2(size) + 1;
  return clg - log2(KM_SLAB_MINBLKSZ);
}

static inline slab_t *blk_slab(void *ptr) {
  assert(ptr);
  vm_page_t *pg = kva_find_page((vaddr_t)ptr);
  return pg->slab;
}

static void *blk_alloc(size_t size, kmem_flags_t flags, size_t *blkszp) {
  assert(size);

#if KASAN
  size_t req_size = size + KASAN_KMALLOC_REDZONE_SIZE;
#else /* !KASAN */
  size_t req_size = size;
#endif

  size_t idx = blk_idx(req_size);
  assert(idx < KM_NPOOLS);

  pool_t *pool = &km_pools[idx];
  void *ptr = pool_alloc(pool, flags);
  if (!ptr)
    return NULL;

  size_t blksz = KM_SLAB_MINBLKSZ << idx;

  kasan_mark(ptr, size, blksz, KASAN_CODE_KMALLOC_OVERFLOW);

  *blkszp = blksz;
  return ptr;
}

static void blk_free(void *ptr, size_t *blkszp) {
  assert(ptr);

  slab_t *slab = blk_slab(ptr);
  pool_t *pool = slab->ph_pool;

#if KASAN
  *blkszp = slab->ph_itemsize - pool->pp_redzone;
#else /* !KASAN */
  *blkszp = slab->ph_itemsize;
#endif

  pool_free(pool, ptr);
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
  size_t blksz;

  if (size > KM_SLAB_MAXBLKSZ)
    ptr = lblk_alloc(size, flags, &blksz);
  else
    ptr = blk_alloc(size, flags, &blksz);

  mp_update_alloc(mp, ptr, blksz);

  assert(!ptr || is_aligned(ptr, KM_ALIGNMENT));
  return ptr;
}

void kfree(kmalloc_pool_t *mp, void *ptr) {
  if (!ptr)
    return;

  size_t blksz;

  if (is_large(ptr))
    lblk_free(ptr, &blksz);
  else
    blk_free(ptr, &blksz);

  mp_update_free(mp, blksz);
}

char *kstrndup(kmalloc_pool_t *mp, const char *s, size_t maxlen) {
  size_t n = strnlen(s, maxlen);
  char *copy = kmalloc(mp, n + 1, 0);
  memcpy(copy, s, n);
  copy[n] = '\0';
  return copy;
}

static alignas(PAGESIZE) uint8_t km_boot_area[KM_BOOT_AREASZ];

void init_kmalloc(void) {
  mtx_init(&km_lock, 0);

  void *slab_pages = km_boot_area;

  for (size_t i = 0; i < KM_NPOOLS; i++) {
    pool_t *pool = &km_pools[i];
    const char *desc = km_slab_cfg[i].desc;
    size_t slabsz = km_slab_cfg[i].size;
    size_t blksz = KM_SLAB_MINBLKSZ << i;

    pool_init(pool, desc, blksz, KM_ALIGNMENT, slabsz);
    pool_add_page(pool, slab_pages, slabsz);

    slab_pages += slabsz;
  }

  assert(slab_pages == &km_boot_area[KM_BOOT_AREASZ]);
}

KMALLOC_DEFINE(M_TEMP, "temporaries");
KMALLOC_DEFINE(M_STR, "strings");
