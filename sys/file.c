#include <file.h>
#include <pool.h>
#include <stdc.h>
#include <errno.h>
#include <mutex.h>
#include <thread.h>
#include <vnode.h>
#include <vfs.h>

static POOL_DEFINE(P_FILE, "file", sizeof(file_t));

file_t *file_alloc(void) {
  file_t *f = pool_alloc(P_FILE, PF_ZERO);
  f->f_ops = &badfileops;
  mtx_init(&f->f_mtx, MTX_DEF);
  return f;
}

/* May be called after the last reference to a file has been dropped. */
void file_destroy(file_t *f) {
  assert(f->f_count == 0);

  /* Note: If the file failed to open, we shall not close it. In such case its
     fileops are set to badfileops. */
  /* TODO: What if an error happens during close? */
  if (f->f_ops != &badfileops)
    FOP_CLOSE(f, thread_self());

  pool_free(P_FILE, f);
}

void file_hold(file_t *f) {
  refcnt_acquire(&f->f_count);
}

void file_drop(file_t *f) {
  if (refcnt_release(&f->f_count))
    file_destroy(f);
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

static int badfo_stat(file_t *f, struct thread *td, stat_t *sb) {
  return -EBADF;
}

static int badfo_seek(file_t *f, struct thread *td, off_t offset, int whence) {
  return -EBADF;
}

fileops_t badfileops = {.fo_read = badfo_read,
                        .fo_write = badfo_write,
                        .fo_close = badfo_close,
                        .fo_stat = badfo_stat,
                        .fo_seek = badfo_seek};
