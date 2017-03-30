#include <file.h>
#include <malloc.h>
#include <stdc.h>
#include <errno.h>
#include <mutex.h>
#include <thread.h>
#include <vnode.h>

static MALLOC_DEFINE(file_pool, "file pool");

void file_init() {
  kmalloc_init(file_pool, 2);
}

void file_ref(file_t *f) {
  mtx_lock(&f->f_mtx);
  assert(f->f_count >= 0);
  f->f_count++;
  mtx_unlock(&f->f_mtx);
}

void file_unref(file_t *f) {
  mtx_lock(&f->f_mtx);
  assert(f->f_count > 0);
  if (--f->f_count == 0)
    f->f_count = -1;
  mtx_unlock(&f->f_mtx);
}

file_t *file_alloc() {
  file_t *f = kmalloc(file_pool, sizeof(file_t), M_ZERO);
  f->f_ops = &badfileops;
  mtx_init(&f->f_mtx, MTX_DEF);
  return f;
}

/* May be called after the last reference to a file has been dropped. */
void file_destroy(file_t *f) {
  assert(f->f_count <= 0);

  /* Note: If the file failed to open, we shall not close it. In such case its
     fileops are set to badfileops. */
  /* TODO: What if an error happens during close? */
  if (f->f_ops != &badfileops)
    FOP_CLOSE(f, thread_self());

  kfree(file_pool, f);
}

void file_release(file_t *f) {
  if (f) {
    file_unref(f);
    if (f->f_count < 0)
      file_destroy(f);
  }
}

/* Operations on invalid file descriptors */
static int badfo_read(file_t *f, struct thread *td, uio_t *uio) {
  return -EBADF;
}

static int badfo_write(file_t *f, struct thread *td, uio_t *uio) {
  return -EBADF;
}

static int badfo_close(file_t *f, struct thread *td) {
  return -EBADF;
}

static int badfo_getattr(file_t *f, struct thread *td, vattr_t *buf) {
  return -EBADF;
}

fileops_t badfileops = {.fo_read = badfo_read,
                        .fo_write = badfo_write,
                        .fo_close = badfo_close,
                        .fo_getattr = badfo_getattr};
