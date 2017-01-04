#include <file.h>
#include <filedesc.h>
#include <malloc.h>
#include <stdc.h>
#include <errno.h>
#include <mutex.h>
#include <thread.h>

static MALLOC_DEFINE(fd_pool, "file descriptors pool");
static MALLOC_DEFINE(file_pool, "file pool");

void fds_init() {
  vm_page_t *pg = pm_alloc(2);
  kmalloc_init(fd_pool);
  kmalloc_add_arena(fd_pool, pg->vaddr, PG_SIZE(pg));
  vm_page_t *pg2 = pm_alloc(2);
  kmalloc_init(file_pool);
  kmalloc_add_arena(file_pool, pg2->vaddr, PG_SIZE(pg2));
}

/* Test whether a file descriptor is in use. */
static int fd_is_used(fd_table_t *fdt, int fd) {
  return bit_test(fdt->fdt_map, fd);
}

static void fd_mark_used(fd_table_t *fdt, int fd) {
  assert(!fd_is_used(fdt, fd));
  bit_set(fdt->fdt_map, fd);
}

static void fd_mark_unused(fd_table_t *fdt, int fd) {
  assert(fd_is_used(fdt, fd));
  bit_clear(fdt->fdt_map, fd);
}

/* Called when the last reference to a file is dropped. */
static void file_free(file_t *f) {
  assert(f->f_count == 0);

  /* Note: If the file failed to open, we shall not close it. In such case its
     fileops are set to badfileops. */
  /* TODO: What if an error happens during close? */
  if (f->f_ops != &badfileops)
    FOP_CLOSE(f, thread_self());

  kfree(file_pool, f);
}

void file_ref(file_t *f) {
  mtx_lock(&f->f_mtx);
  ++f->f_count;
  mtx_unlock(&f->f_mtx);
}

void file_unref(file_t *f) {
  mtx_lock(&f->f_mtx);
  int i = --f->f_count;
  mtx_unlock(&f->f_mtx);
  assert(i >= 0);
  if (i == 0)
    file_free(f);
}

/* Allocates a new file descriptor in a file descriptor table. Returns 0 on
 * success and sets *result to new descriptor number. Must be called with
 * fd->fd_mtx already locked. */
static int fd_alloc(fd_table_t *fdt, int *fdp) {
  assert(mtx_owned(&fdt->fdt_mtx));

  int first_free = MAXFILES;
  bit_ffc(fdt->fdt_map, fdt->fdt_nfiles, &first_free);

  if (first_free >= fdt->fdt_nfiles) {
    /* No more space to allocate a descriptor! The descriptor table should grow,
       but we can't do that yet. */
    return -EMFILE;
  }
  fd_mark_used(fdt, first_free);
  *fdp = first_free;
  return 0;
}

static void fd_free(fd_table_t *fdt, int fd) {
  file_t *f = fdt->fdt_files[fd];
  assert(f != NULL);
  file_unref(f);
  fdt->fdt_files[fd] = NULL;
  fd_mark_unused(fdt, fd);
}

static void fd_table_free(fd_table_t *fdt) {
  assert(fdt->fdt_count == 0);
  /* No need to lock mutex, we have the only reference left. */

  /* Cleanup used descriptors. This possibly closes underlying files. */
  for (int i = 0; i < fdt->fdt_nfiles; i++)
    if (fd_is_used(fdt, i))
      fd_free(fdt, i);

  kfree(fd_pool, fdt);
}

void fd_table_ref(fd_table_t *fdt) {
  mtx_lock(&fdt->fdt_mtx);
  ++fdt->fdt_count;
  mtx_unlock(&fdt->fdt_mtx);
}

void fd_table_unref(fd_table_t *fdt) {
  mtx_lock(&fdt->fdt_mtx);
  int i = --fdt->fdt_count;
  mtx_unlock(&fdt->fdt_mtx);
  /* Don't worry that something might happen to the filedesc after we released a
   * mutex, since we know nobody else has a reference to it. */
  if (i == 0)
    fd_table_free(fdt);
}

/* Called e.g. when a thread exits. */
void fd_table_destroy(fd_table_t *fdt) {
  fd_table_unref(fdt);
}

file_t *file_alloc_noinstall() {
  file_t *f;
  f = kmalloc(file_pool, sizeof(struct file), M_ZERO);
  f->f_ops = &badfileops;
  f->f_count = 1;
  f->f_data = 0;
  mtx_init(&f->f_mtx);
  return f;
}

int file_install_desc(fd_table_t *fdt, file_t *f, int *fd) {
  assert(f != NULL);
  assert(fd != NULL);

  mtx_lock(&fdt->fdt_mtx);
  int res = fd_alloc(fdt, fd);
  if (res < 0) {
    mtx_unlock(&fdt->fdt_mtx);
    return res;
  }

  fdt->fdt_files[*fd] = f;
  file_ref(f);
  mtx_unlock(&fdt->fdt_mtx);
  return 0;
}

int file_alloc_install_desc(fd_table_t *fdt, file_t **resultf,
                            int *resultfd) {
  file_t *f;
  int fd;
  f = file_alloc_noinstall();
  int res = file_install_desc(fdt, f, &fd);
  if (res < 0) {
    file_unref(f);
    return res;
  }

  if (resultf == NULL) {
    /* Caller does not want a pointer to file, release local reference. */
    file_unref(f);
  } else {
    *resultf = f;
  }

  if (resultfd != NULL)
    *resultfd = fd;

  return 0;
}

/* In FreeBSD this function takes a filedesc* argument, so that
   current dir may be copied. Since we don't use these fields, this
   argument doest not make sense yet. */
fd_table_t *fd_table_init() {
  fd_table_t *fdt;
  fdt = kmalloc(fd_pool, sizeof(fd_table_t), M_ZERO);
  /* For now, fdt_files and fdt_map have a static size, so there is no need to
   * separately allocate memory for them. */
  fdt->fdt_nfiles = NDFILE;
  fdt->fdt_count = 1;
  mtx_init(&fdt->fdt_mtx);
  return fdt;
}

fd_table_t *fd_table_copy(fd_table_t *fdt) {
  fd_table_t *newfdt = fd_table_init();

  if (fdt == NULL)
    return newfdt;

  mtx_lock(&fdt->fdt_mtx);

  /* We can assume both filedescs use 20 descriptors,
   * because we don't support other numbers. */
  assert(fdt->fdt_nfiles == newfdt->fdt_nfiles);

  for (int i = 0; i < fdt->fdt_nfiles; i++) {
    file_t *f = fdt->fdt_files[i];

    /* TODO: Deep or shallow copy? */
    newfdt->fdt_files[i] = f;

    file_ref(f);
  }

  mtx_unlock(&fdt->fdt_mtx);

  return newfdt;
}

/* Extracts file pointer from descriptor number for a particular thread. If
   flags are non-zero, returns EBADF if the file does not match flags. */
int fd_file_get(thread_t *td, int fd, int flags, file_t **fp) {
  fd_table_t *fdt = td->td_fdtable;

  if (!fdt)
    return -EBADF;

  mtx_lock(&fdt->fdt_mtx);

  if (fd < 0 || fd >= fdt->fdt_nfiles || !fd_is_used(fdt, fd))
    goto fail;

  file_t *f = fdt->fdt_files[fd];
  file_ref(f);

  if ((flags & FF_READ) && !(f->f_flags & FF_READ))
    goto fail2;
  if ((flags & FF_WRITE) && !(f->f_flags & FF_WRITE))
    goto fail2;

  mtx_unlock(&fdt->fdt_mtx);
  *fp = f;
  return 0;

fail2:
  file_unref(f);
fail:
  mtx_unlock(&fdt->fdt_mtx);
  return -EBADF;
}

/* Closes a file descriptor. If it was the last reference to a file, the file is
 * also closed. */
int fd_close(fd_table_t *fdt, int fd) {
  mtx_lock(&fdt->fdt_mtx);

  if (fd < 0 || fd > fdt->fdt_nfiles || !fd_is_used(fdt, fd)) {
    mtx_unlock(&fdt->fdt_mtx);
    return -EBADF;
  }

  fd_free(fdt, fd);

  mtx_unlock(&fdt->fdt_mtx);

  return 0;
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

fileops_t badfileops = {
  .fo_read = badfo_read,
  .fo_write = badfo_write,
  .fo_close = badfo_close,
};
