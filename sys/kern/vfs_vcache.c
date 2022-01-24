/* vcache
 *
 * This subsystem manages vnodes used by filesystems allowing them  to "borrow"
 * a vnode whenever they need one and to return it later for cached storage.
 * A filesystem can use vcache to manage its internal inode data by tying its
 * lifetime to a vcached vnode.
 */

#define KL_LOG KL_VFS

#include <sys/mimiker.h>
#include <sys/klog.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/mutex.h>

static POOL_DEFINE(P_VCACHE, "vcache", sizeof(vnode_t));

#define VCACHE_BUCKET_CNT 64
#define VCACHE_INITIAL_FREE_CNT 128

typedef TAILQ_HEAD(vnode_list, vnode) vnode_list_t;

static vnode_list_t vcache_free;
static vnode_list_t vcache_buckets[VCACHE_BUCKET_CNT];
static mtx_t vcache_giant_lock;

typedef size_t vcache_t;

static vcache_t vcache_hash(mount_t *mp, ino_t ino) {
  return (((vcache_t)mp->mnt_vnodecovered >> 3) + ino) % VCACHE_BUCKET_CNT;
}

/* XXX: this is taken from vfs_vnode.c */
static void vnlock_init(vnlock_t *vl) {
  spin_init(&vl->vl_interlock, 0);
  cv_init(&vl->vl_cv, "vnode sleep cv");
}

static vnode_t *vcache_new_vnode(void) {
  vnode_t *vn = pool_alloc(P_VCACHE, M_ZERO);
  vn->v_type = V_NONE;
  vn->v_flags = VF_CACHED;
  vnlock_init(&vn->v_lock);
  return vn;
}

vnode_t *vcache_hashget(mount_t *mp, ino_t ino) {
  vcache_t bucket = vcache_hash(mp, ino);

  vnode_t *vn = NULL;
  TAILQ_FOREACH (vn, &vcache_buckets[bucket], v_cached) {
    if ((vn->v_mount == mp) && (vn->v_ino == ino)) {
      TAILQ_REMOVE(&vcache_free, vn, v_free);
      TAILQ_REMOVE(&vcache_buckets[bucket], vn, v_cached);
      vnode_hold(vn);
      return vn;
    }
  }

  return NULL;
}

static int vfs_vcache_remove_from_bucket(vnode_t *vn) {
  if (!vn->v_mount)
    return 0; /* vnode was not in a any bucket and cannot be hashed */
  vcache_t bucket = vcache_hash(vn->v_mount, vn->v_ino);
  vnode_t *bucket_node = NULL;

  TAILQ_FOREACH (bucket_node, &vcache_buckets[bucket], v_cached) {
    if (bucket_node == vn) {
      TAILQ_REMOVE(&vcache_buckets[bucket], vn, v_cached);
      break;
    }
  }

  return 0;
}

int vfs_vcache_detach(vnode_t *vn) {
  int error = 0;
  /* Reclaim vnode (if supported) */
  if (vn->v_data && vn->v_ops->v_reclaim)
    error = VOP_RECLAIM(vn);
  if (error)
    return error;

  /* Seting v_data to NULL marks the vnode as "detached" */
  vn->v_data = NULL;

  return 0;
}

void vfs_vcache_init(void) {
  TAILQ_INIT(&vcache_free);
  for (size_t i = 0; i < VCACHE_BUCKET_CNT; i++)
    TAILQ_INIT(&vcache_buckets[i]);

  for (size_t i = 0; i < VCACHE_INITIAL_FREE_CNT; i++) {
    vnode_t *vn = vcache_new_vnode();
    TAILQ_INSERT_TAIL(&vcache_free, vn, v_free);
  }

  mtx_init(&vcache_giant_lock, 0);
}

vnode_t *vfs_vcache_reborrow(mount_t *mp, ino_t ino) {
  SCOPED_MTX_LOCK(&vcache_giant_lock);

  return vcache_hashget(mp, ino);
}

vnode_t *vfs_vcache_borrow_new(void) {
  int error = 0;

  SCOPED_MTX_LOCK(&vcache_giant_lock);

  vnode_t *vn;

  /* Allocate new vnode if no free vnodes are available */
  if (TAILQ_EMPTY(&vcache_free)) {
    klog("Added a new vnode to vcache pool.");
    vn = vcache_new_vnode();
    vn->v_usecnt = 1;
    return vn;
  }

  /* Get the Least Recently Used vnode */
  vn = TAILQ_FIRST(&vcache_free);

  /* Remove it from free list */
  TAILQ_REMOVE(&vcache_free, vn, v_free);

  /* Remove it from its bucket (if present in any) */
  error = vfs_vcache_detach(vn);
  assert(error == 0);
  vfs_vcache_remove_from_bucket(vn);

  /* Reset some fields */
  vn->v_usecnt = 1;
  vn->v_mount = NULL;
  vn->v_ops = NULL;

  return vn;
}

void vfs_vcache_return(vnode_t *vn) {
  assert(vn->v_usecnt == 0);

  SCOPED_MTX_LOCK(&vcache_giant_lock);

  TAILQ_INSERT_TAIL(&vcache_free, vn, v_free);
  if (vn->v_data) {
    vcache_t bucket = vcache_hash(vn->v_mount, vn->v_ino);
    TAILQ_INSERT_HEAD(&vcache_buckets[bucket], vn, v_cached);
  }
}
