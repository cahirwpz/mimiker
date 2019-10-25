#define KL_LOG KL_FILESYS
#include <sys/klog.h>
#include <sys/tmpfs.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/errno.h>
#include <sys/libkern.h>
#include <sys/mutex.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/linker_set.h>
#include <sys/dirent.h>

static POOL_DEFINE(P_TMPFS_NODE, "tmpfs node", sizeof(tmpfs_node_t));
static POOL_DEFINE(P_TMPFS_DIRENT, "tmpfs dirent", sizeof(tmpfs_dirent_t));

/* Functions to convert VFS structures to tmpfs internal ones. */
static __inline tmpfs_mount_t *VFS_TO_TMPFS(mount_t *mp) {
  return (tmpfs_mount_t *)mp->mnt_data;
}

static __inline tmpfs_node_t *VFS_TO_TMPFS_NODE(vnode_t *vp) {
  return (tmpfs_node_t *)vp->v_data;
}

static vnode_lookup_t tmpfs_vop_lookup;
static vnode_readdir_t tmpfs_vop_readdir;
static vnode_close_t tmpfs_vop_close;
static vnode_read_t tmpfs_vop_read;
static vnode_write_t tmpfs_vop_write;
static vnode_seek_t tmpfs_vop_seek;
static vnode_getattr_t tmpfs_vop_getattr;
static vnode_create_t tmpfs_vop_create;
static vnode_remove_t tmpfs_vop_remove;
static vnode_mkdir_t tmpfs_vop_mkdir;
static vnode_rmdir_t tmpfs_vop_rmdir;

static vnodeops_t tmpfs_vnodeops = {.v_lookup = tmpfs_vop_lookup,
                                    .v_readdir = tmpfs_vop_readdir,
                                    .v_open = vnode_open_generic,
                                    .v_close = tmpfs_vop_close,
                                    .v_read = tmpfs_vop_read,
                                    .v_write = tmpfs_vop_write,
                                    .v_seek = tmpfs_vop_seek,
                                    .v_getattr = tmpfs_vop_getattr,
                                    .v_create = tmpfs_vop_create,
                                    .v_remove = tmpfs_vop_remove,
                                    .v_mkdir = tmpfs_vop_mkdir,
                                    .v_rmdir = tmpfs_vop_rmdir,
                                    .v_access = vnode_access_generic};

/* tmpfs vnode operations */

static int tmpfs_vop_lookup(vnode_t *dv, const char *name, vnode_t **vp) {
  return EOPNOTSUPP;
}

static int tmpfs_vop_readdir(vnode_t *dv, uio_t *uio, void *state) {
  return EOPNOTSUPP;
}

static int tmpfs_vop_close(vnode_t *v, file_t *fp) {
  return EOPNOTSUPP;
}

static int tmpfs_vop_read(vnode_t *v, uio_t *uio) {
  return EOPNOTSUPP;
}

static int tmpfs_vop_write(vnode_t *v, uio_t *uio) {
  return EOPNOTSUPP;
}

static int tmpfs_vop_seek(vnode_t *v, off_t oldoff, off_t newoff, void *state) {
  return EOPNOTSUPP;
}

static int tmpfs_vop_getattr(vnode_t *v, vattr_t *va) {
  return EOPNOTSUPP;
}

static int tmpfs_vop_create(vnode_t *dv, const char *name, vattr_t *va,
                            vnode_t **vp) {
  return EOPNOTSUPP;
}

static int tmpfs_vop_remove(vnode_t *dv, const char *name) {
  return EOPNOTSUPP;
}

static int tmpfs_vop_mkdir(vnode_t *dv, const char *name, vattr_t *va,
                           vnode_t **vp) {
  return EOPNOTSUPP;
}

static int tmpfs_vop_rmdir(vnode_t *dv, const char *name) {
  return EOPNOTSUPP;
}

/* tmpfs internal routines */

/* tmpfs vfs operations */

static int tmpfs_mount(mount_t *mp) {
  // Temporary solution to mute unused warning.
  // Will be deleted in next commit.
  tmpfs_vnodeops.v_lookup = NULL;
  return 0;
}

static int tmpfs_root(mount_t *mp, vnode_t **vp) {
  return 0;
}

static int tmpfs_init(vfsconf_t *vfc) {
  return 0;
}

static vfsops_t tmpfs_vfsops = {
  .vfs_mount = tmpfs_mount, .vfs_root = tmpfs_root, .vfs_init = tmpfs_init};

static vfsconf_t tmpfs_conf = {.vfc_name = "tmpfs",
                               .vfc_vfsops = &tmpfs_vfsops};

SET_ENTRY(vfsconf, tmpfs_conf);
