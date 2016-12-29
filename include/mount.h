#ifndef _SYS_MOUNT_H_
#define _SYS_MOUNT_H_

#include <queue.h>
#include <mutex.h>

/* Maximum length of a filesystem type name */
#define VFSCONF_NAME_MAX 32
/* Maximum length of a path to lookup */
#define VFS_PATH_MAX 256

/* Forward declarations */
typedef struct vnode vnode_t;
typedef struct mount mount_t;
typedef struct vnode vnode_t;
typedef struct vfsconf vfsconf_t;
typedef struct statfs statfs_t;

/* VFS operations */
typedef int vfs_mount_t(mount_t *m);
typedef int vfs_root_t(mount_t *m, vnode_t **vp);
typedef int vfs_statfs_t(mount_t *m, statfs_t *sb);
typedef int vfs_vget_t(mount_t *m, ino_t ino, vnode_t **vp);
typedef int vfs_init_t(vfsconf_t *vfc);

typedef struct vfsops {
  vfs_mount_t *vfs_mount;
  vfs_root_t *vfs_root;
  vfs_statfs_t *vfs_statfs;
  vfs_vget_t *vfs_vget;
  vfs_init_t *vfs_init;
} vfsops_t;

/* Description of a filesystem type. There is one instance of this struct per
 * each filesystem type in the kernel (e.g. tmpfs, devfs). */
typedef struct vfsconf {
  char vfc_name[VFSCONF_NAME_MAX]; /* Filesystem type name */
  vfsops_t *vfc_vfsops;            /* Filesystem operations */
  int vfc_mountcnt; /* Number of mounted filesystems of this type */
  TAILQ_ENTRY(vfsconf) vfc_list; /* Entry on the list of vfsconfs */
} vfsconf_t;

/* This structure represents a mount point: a particular instance of a file
   system mounted somewhere in the file tree. */
typedef struct mount {
  TAILQ_ENTRY(mount) mnt_list;    /* Entry on the mounts list */
  TAILQ_HEAD(, vnode) mnt_vnodes; /* List of vnodes on this mount point */

  vfsops_t *mnt_vfsops;      /* Filesystem operations */
  vfsconf_t *mnt_vfc;        /* Link to filesystem info */
  vnode_t *mnt_vnodecovered; /* The vnode covered by this mount */

  int mnt_refcnt; /* Reference count */
  mtx_t mnt_mtx;

  void *mnt_data; /* Filesystem-specific arbitrary data */
} mount_t;

static inline int VFS_MOUNT(mount_t *m) {
  return m->mnt_vfsops->vfs_mount(m);
}

static inline int VFS_ROOT(mount_t *m, vnode_t **vp) {
  return m->mnt_vfsops->vfs_root(m, vp);
}

static inline int VFS_STATFS(mount_t *m, statfs_t *sb) {
  return m->mnt_vfsops->vfs_statfs(m, sb);
}

static inline int VFS_VGET(mount_t *m, ino_t ino, vnode_t **vp) {
  return m->mnt_vfsops->vfs_vget(m, ino, vp);
}

/* This is the / node. Since we aren't mounting anything on / just yet, there is
   also a separate global vnode for /dev .*/
extern vnode_t *vfs_root_vnode;
extern vnode_t *vfs_root_dev_vnode;

/* Look up a file system type by name. */
vfsconf_t *vfs_get_by_name(const char *name);

/* Allocates and initializes a new mount struct, using filesystem vfc, covering
 * vnode v. Does not modify v. Does not insert new mount onto the all mounts
 * list. */
mount_t *vfs_mount_alloc(vnode_t *v, vfsconf_t *vfc);

/* Mount a new instance of the filesystem vfc at the vnode v. Does not support
 * remounting. TODO: Additional filesystem-specific arguments. */
int vfs_domount(vfsconf_t *vfc, vnode_t *v);

/* Finds the vnode corresponding to the given path.
 * Increases use count on returned vnode. */
int vfs_lookup(const char *pathname, vnode_t **vp);

/* Initializes the VFS subsystem. */
void vfs_init();

#endif /* !_SYS_MOUNT_H_ */
