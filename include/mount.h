#ifndef __SYS_MOUNT_H__
#define __SYS_MOUNT_H__

#include <queue.h>
#include <mutex.h>

/* Maximum length of a filesystem type name */
#define VFCONF_NAME_MAX 32

/* Forward declarations */
typedef struct vnode vnode_t;
typedef struct mount mount_t;
typedef struct vnode vnode_t;
typedef struct vfsconf vfsconf_t;

/* VFS operations */
typedef int vfs_mount_t(mount_t *m);
typedef int vfs_root_t(mount_t *m, vnode_t **v);
typedef int vfs_vget_t(mount_t *m, int ino, vnode_t **v);
typedef int vfs_init_t(vfsconf_t *);

typedef struct vfsops {
  vfs_mount_t *vfs_mount;
  vfs_root_t *vfs_root;
  vfs_vget_t *vfs_vget;
  vfs_init_t *vfs_init;
} vfsops_t;

/* Description of a filesystem type. There is one instance of this struct per
 * each filesystem type in the kernel (e.g. tmpfs, devfs). */
typedef struct vfsconf {
  char vfc_name[VFCONF_NAME_MAX]; /* Filesystem type name */
  vfsops_t *vfc_vfsops;           /* Filesystem operations */
  int vfc_mountcount; /* Number of mounted filesystems of this type */
  TAILQ_ENTRY(vfsconf) vfc_list; /* Entry on the list of vfsconfs */
} vfsconf_t;

/* This structure represents a mount point: a particular instance of a file
   system mounted somewhere in the file tree. */
typedef struct mount {
  TAILQ_ENTRY(mount) mnt_list; /* Entry on the mounts list */

  vfsops_t *mnt_op;          /* Filesystem operations */
  vfsconf_t *mnt_vfc;        /* Link to filesystem info */
  vnode_t *mnt_vnodecovered; /* The vnode covered by this mount */

  int mnt_ref; /* Reference count */
  mtx_t mnt_mtx;

  /* TODO: List of vnodes */

  void *mnt_data; /* Filesystem-specific arbitrary data */
} mount_t;

/* This is the / node. Since we aren't mounting anything on / just yet, there is
   also a separate global vnode for /dev .*/
extern vnode_t *vfs_root_vnode;
extern vnode_t *vfs_root_dev_vnode;

/* Look up a file system type by name. */
vfsconf_t *vfs_get_by_name(const char *name);
/* Register a file system type */
int vfs_register(vfsconf_t *vfc);

/* Allocates and initializes a new mount struct, using filesystem vfc, covering
 * vnode v. Does not modify v. Does not insert new mount onto the all mounts
 * list. */
mount_t *vfs_mount_alloc(vnode_t *v, vfsconf_t *vfc);

/* Mount a new instance of the filesystem vfc at the vnode v. Does not support
 * remounting. TODO: Additional filesystem-specific arguments. */
int vfs_domount(vfsconf_t *vfc, vnode_t *v);

/* Finds the vnode corresponding to the given path. */
int vfs_lookup(const char *pathname, vnode_t **v);

/* Initializes the VFS subsystem. */
void vfs_init();

#endif /* __SYS_MOUNT_H__ */
