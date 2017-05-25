#include <errno.h>
#include <file.h>
#include <malloc.h>
#include <mutex.h>
#include <stdc.h>
#include <vnode.h>
#include <sysinit.h>

static MALLOC_DEFINE(M_VNODE, "vnode", 2, 16);

/* Actually, vnode management should be much more complex than this, because
   this stub does not recycle vnodes, does not store them on a free list,
   etc. So at some point we may need a more sophisticated memory management here
   - but this will do for now. */

static void vnode_init() {
}

vnode_t *vnode_new(vnodetype_t type, vnodeops_t *ops) {
  vnode_t *v = kmalloc(M_VNODE, sizeof(vnode_t), M_ZERO);
  v->v_type = type;
  v->v_data = NULL;
  v->v_ops = ops;
  v->v_usecnt = 1;
  mtx_init(&v->v_mtx, MTX_DEF);

  return v;
}

void vnode_lock(vnode_t *v) {
  mtx_lock(&v->v_mtx);
}

void vnode_unlock(vnode_t *v) {
  mtx_unlock(&v->v_mtx);
}

void vnode_ref(vnode_t *v) {
  vnode_lock(v);
  v->v_usecnt++;
  vnode_unlock(v);
}

void vnode_unref(vnode_t *v) {
  vnode_lock(v);
  v->v_usecnt--;
  if (v->v_usecnt == 0)
    kfree(M_VNODE, v);
  else
    vnode_unlock(v);
}

int vnode_op_notsup() {
  return -ENOTSUP;
}

static int vnode_generic_read(file_t *f, thread_t *td, uio_t *uio) {
  return VOP_READ(f->f_vnode, uio);
}

static int vnode_generic_write(file_t *f, thread_t *td, uio_t *uio) {
  return VOP_WRITE(f->f_vnode, uio);
}

static int vnode_generic_close(file_t *f, thread_t *td) {
  /* TODO: vnode closing is not meaningful yet. */
  vnode_unref(f->f_vnode);
  return 0;
}

static int vnode_generic_getattr(file_t *f, thread_t *td, vattr_t *vattr) {
  vnode_t *v = f->f_vnode;
  return v->v_ops->v_getattr(v, vattr);
}

static fileops_t vnode_generic_fileops = {
  .fo_read = vnode_generic_read,
  .fo_write = vnode_generic_write,
  .fo_close = vnode_generic_close,
  .fo_getattr = vnode_generic_getattr,
};

int vnode_open_generic(vnode_t *v, int mode, file_t *fp) {
  vnode_ref(v);
  fp->f_ops = &vnode_generic_fileops;
  fp->f_type = FT_VNODE;
  fp->f_vnode = v;
  switch (mode) {
    case O_RDONLY:
      fp->f_flags = FF_READ;
      break;
    case O_WRONLY:
      fp->f_flags = FF_WRITE;
      break;
    case O_RDWR:
      fp->f_flags = FF_READ | FF_WRITE;
      break;
    default:
      return -EINVAL;
  }
  return 0;
}

int vnode_close_generic(vnode_t *v, file_t *fp) {
  return 0;
}

SYSINIT_ADD(vnode, vnode_init, NODEPS);
