#ifndef __SYS_VNODE_H__
#define __SYS_VNODE_H__

#include <mutex.h>
#include <uio.h>

/* Forward declarations */
typedef struct vnode vnode_t;
typedef struct mount mount_t;
typedef struct file file_t;

enum vnode_type {
  V_NONE,
  V_REG,
  V_DIR,
  V_DEV,
};

typedef int vnode_lookup_t(vnode_t *dir, const char *name, vnode_t **res);
typedef int vnode_readdir_t(vnode_t *dir, uio_t *uio);
typedef int vnode_open_t(vnode_t *v, file_t **f);
typedef int vnode_read_t(vnode_t *v, uio_t *uio);
typedef int vnode_write_t(vnode_t *v, uio_t *uio);

typedef struct vnodeops {
  vnode_lookup_t *v_lookup;
  vnode_readdir_t *v_readdir;
  vnode_open_t *v_open;
  vnode_read_t *v_read;
  vnode_write_t *v_write;
} vnodeops_t;

typedef struct vnode {
  enum vnode_type v_type; /* Vnode type, see above */

  vnodeops_t *v_ops; /* Vnode operations */
  void *v_data;      /* Filesystem-specific arbitrary data */

  mount_t *v_mount; /* Pointer to the mount we are in */

  /* Type-specific fields */
  union {
    mount_t *vu_mount; /* The mount covering this vnode */
  } v_un;

  int v_ref; /* Reference count */
  mtx_t v_mtx;

} vnode_t;

#define v_mountedhere v_un.vu_mount

/* Initializes vnode subsystem */
void vnode_init();

/* Allocates and initializes a new vnode */
vnode_t *vnode_new(enum vnode_type type, vnodeops_t *ops);

/* Increasing and decreasing the reference counter. */
void vnode_hold(vnode_t *v);
void vnode_release(vnode_t *v);
void vnode_lock_release(vnode_t *v);

/* Convenience function for filling in not supported vnodeops */
int vnode_op_notsup();

#endif /* __SYS_VNODE_H__ */
