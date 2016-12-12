#include <vnode.h>
#include <malloc.h>
#include <mutex.h>
#include <stdc.h>
#include <errno.h>
#include <file.h>

static MALLOC_DEFINE(vnode_pool, "vnode pool");

/* Actually, vnode management should be much more complex than this, because
   this stub does not recycle vnodes, does not store them on a free list,
   etc. So at some point we may need a more sophisticated memory management here
   - but this will do for now. */

void vnode_init() {
  kmalloc_init(vnode_pool);
  kmalloc_add_arena(vnode_pool, pm_alloc(2)->vaddr, PAGESIZE * 2);
}

vnode_t *vnode_new(vnodetype_t type, vnodeops_t *ops) {
  vnode_t *v = kmalloc(vnode_pool, sizeof(vnode_t), M_ZERO);
  v->v_type = type;
  v->v_data = NULL;
  v->v_ops = ops;
  v->v_usecnt = 1;
  mtx_init(&v->v_mtx);

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
  /* TODO: if v_usecnt reaches zero, the vnode should be released */
  vnode_unlock(v);
}

int vnode_op_notsup() {
  return ENOTSUP;
}

static int vnode_generic_read(file_t *f, thread_t *td, uio_t *uio) {
  vnode_t *v = f->f_vnode;
  return v->v_ops->v_read(v, uio);
}
static int vnode_generic_write(file_t *f, thread_t *td, uio_t *uio) {
  vnode_t *v = f->f_vnode;
  return v->v_ops->v_write(v, uio);
}
static int vnode_generic_close(file_t *f, thread_t *td) {
  /* TODO: vnode closing is not meaningful yet. */
  return 0;
}

static fileops_t vnode_generic_fileops = {
  .fo_read = vnode_generic_read,
  .fo_write = vnode_generic_write,
  .fo_close = vnode_generic_close,
};

int vnode_open_generic(vnode_t *v, int mode, file_t *fp) {
  vnode_ref(v);
  fp->f_ops = &vnode_generic_fileops;
  fp->f_type = FILE_TYPE_VNODE;
  fp->f_vnode = v;
  switch (mode) {
  case O_RDONLY:
    fp->f_flags = FILE_FLAG_READ;
    break;
  case O_WRONLY:
    fp->f_flags = FILE_FLAG_WRITE;
    break;
  case O_RDWR:
    fp->f_flags = FILE_FLAG_READ | FILE_FLAG_WRITE;
    break;
  default:
    return EINVAL;
  }
  return 0;
}
