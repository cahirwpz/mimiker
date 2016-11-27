#include <file.h>
#include <filedesc.h>
#include <malloc.h>
#include <stdc.h>
#include <errno.h>
#include <mutex.h>
#include <thread.h>

static MALLOC_DEFINE(fd_pool, "file descriptors pool");
static MALLOC_DEFINE(file_pool, "file pool");

/* The initial size of space allocated for file descriptors. According
   to FreeBSD, this is more than enough for most applications. Each
   process starts with this many descriptors, and more are allocated
   on demand. */
#define NDFILE 20
/* Separate macro defining a hard limit on open files. */
#define MAXFILES 20

void file_desc_init() {
  vm_page_t *pg = pm_alloc(2);
  kmalloc_init(fd_pool);
  kmalloc_add_arena(fd_pool, pg->vaddr, PG_SIZE(pg));
  vm_page_t *pg2 = pm_alloc(2);
  kmalloc_init(file_pool);
  kmalloc_add_arena(file_pool, pg2->vaddr, PG_SIZE(pg2));
}

/* Test whether a file descriptor is in use. */
static int file_desc_isused(file_desc_table_t *fdt, int fd) {
  return bit_test(fdt->fdt_map, fd);
}

static void file_desc_mark_used(file_desc_table_t *fdt, int fd) {
  assert(!file_desc_isused(fdt, fd));
  bit_set(fdt->fdt_map, fd);
}

static void file_desc_mark_unused(file_desc_table_t *fdt, int fd) {
  assert(!file_desc_isused(fdt, fd));
  bit_clear(fdt->fdt_map, fd);
}

/* Called when the last reference to a file is dropped. */
static void file_free(file_t *f) {
  assert(mtx_is_locked(&f->f_mtx));
  assert(f->f_count == 0);

  /* TODO: Do not close an invalid file (badfileops) */
  /* TODO: What if an error happens during close? */
  f->f_ops.fo_close(f, thread_self());

  kfree(file_pool, f);
}

void file_hold(file_t *f) {
  mtx_lock(&f->f_mtx);
  ++f->f_count;
  mtx_unlock(&f->f_mtx);
}

void file_drop(file_t *f) {
  mtx_lock(&f->f_mtx);
  int i = --f->f_count;
  mtx_unlock(&f->f_mtx);
  if (i == 0)
    file_free(f);
}

/* Allocates a new file descriptor in a file descriptor table. Returns 0 on
 * success and sets *result to new descriptor number. Must be called with
 * fd->fd_mtx already locked. */
int file_desc_alloc(file_desc_table_t *fdt, int *fd) {
  assert(mtx_is_locked(&fdt->fdt_mtx));

  int first_free = MAXFILES;
  bit_ffc(fdt->fdt_map, fdt->fdt_nfiles, &first_free);

  if (first_free >= fdt->fdt_nfiles) {
    /* No more space to allocate a descriptor! The descriptor table should grow,
       but we can't do that yet. */
    return -EMFILE;
  }
  file_desc_mark_used(fdt, first_free);
  *fd = first_free;
  return 0;
}

void file_desc_free(file_desc_table_t *fdt, int fd) {
  file_t *f = fdt->fdt_ofiles[fd];
  assert(f != NULL);
  file_drop(f);
  fdt->fdt_ofiles[fd] = NULL;
  file_desc_mark_unused(fdt, fd);
}

void file_desc_table_free(file_desc_table_t *fdt) {
  assert(fdt->fdt_count == 0);
  /* No need to lock mutex, we have the only reference left. */

  /* Cleanup used descriptors. This possibly closes underlying files. */
  for (int i = 0; i < fdt->fdt_nfiles; i++)
    if (file_desc_isused(fdt, i))
      file_desc_free(fdt, i);

  kfree(fd_pool, fdt->fdt_ofiles);
  kfree(fd_pool, fdt->fdt_map);
  kfree(fd_pool, fdt);
}

void file_desc_table_hold(file_desc_table_t *fdt) {
  mtx_lock(&fdt->fdt_mtx);
  ++fdt->fdt_count;
  mtx_unlock(&fdt->fdt_mtx);
}

void file_desc_table_drop(file_desc_table_t *fdt) {
  mtx_lock(&fdt->fdt_mtx);
  int i = --fdt->fdt_count;
  mtx_unlock(&fdt->fdt_mtx);
  /* Don't worry that something might happen to the filedesc after we released a
   * mutex, since we know nobody else has a reference to it. */
  if (i == 0)
    file_desc_table_free(fdt);
}

/* Called e.g. when a thread exits. */
void file_desc_table_destroy(file_desc_table_t *fdt) {
  file_desc_table_drop(fdt);
}

file_t *file_alloc_noinstall() {
  file_t *f;
  f = kmalloc(file_pool, sizeof(struct file), M_ZERO);
  f->f_ops = badfileops;
  f->f_count = 1;
  f->f_data = 0;
  return f;
}

int file_install_desc(file_desc_table_t *fdt, file_t *f, int *fd) {
  assert(f != NULL);
  assert(fd != NULL);

  mtx_lock(&fdt->fdt_mtx);
  int res = file_desc_alloc(fdt, fd);
  if (res < 0) {
    mtx_unlock(&fdt->fdt_mtx);
    return res;
  }

  fdt->fdt_ofiles[*fd] = f;
  file_hold(f);
  mtx_unlock(&fdt->fdt_mtx);
  return 0;
}

int file_alloc_install_desc(file_desc_table_t *fdt, file_t **resultf,
                            int *resultfd) {
  file_t *f;
  int fd;
  f = file_alloc_noinstall();
  int res = file_install_desc(fdt, f, &fd);
  if (res < 0) {
    file_drop(f);
    return res;
  }

  if (resultf == NULL) {
    /* Caller does not want a pointer to file, release local reference. */
    file_drop(f);
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
file_desc_table_t *file_desc_table_init() {
  file_desc_table_t *fdt;
  fdt = kmalloc(fd_pool, sizeof(file_desc_table_t), M_ZERO);
  fdt->fdt_ofiles = kmalloc(fd_pool, NDFILE * sizeof(file_t *), M_ZERO);
  fdt->fdt_map =
    kmalloc(fd_pool, bitstr_size(NDFILE) * sizeof(bitstr_t), M_ZERO);
  fdt->fdt_nfiles = NDFILE;
  fdt->fdt_count = 1;
  return fdt;
}

file_desc_table_t *file_desc_table_copy(file_desc_table_t *fdt) {
  file_desc_table_t *newfdt;

  newfdt = file_desc_table_init();

  if (!fdt)
    return newfdt;

  mtx_lock(&fdt->fdt_mtx);

  /* We can assume both filedescs use 20 descriptors, because we don't support
   * other numbers. */
  assert(fdt->fdt_nfiles == newfdt->fdt_nfiles);

  for (int i = 0; i < fdt->fdt_nfiles; i++) {
    file_t *f = fdt->fdt_ofiles[i];

    /* TODO: Deep or shallow copy? */
    newfdt->fdt_ofiles[i] = f;

    file_hold(f);
  }

  mtx_unlock(&fdt->fdt_mtx);

  return newfdt;
}

/* Extracts file pointer from descriptor number for a particular thread. If
   flags are non-zero, returns -EBADF if the file does not match flags. */
static int _file_get(thread_t *td, int fd, int flags, file_t **resultf) {
  file_desc_table_t *fdt = td->td_fdt;
  if (!fdt)
    return -EBADF;

  mtx_lock(&fdt->fdt_mtx);

  if (fd < 0 || fd >= fdt->fdt_nfiles || !file_desc_isused(fdt, fd))
    goto fail;

  file_t *f = fdt->fdt_ofiles[fd];
  file_hold(f);

  if (flags | FREAD && !(f->f_flag | FREAD))
    goto fail2;
  if (flags | FWRITE && !(f->f_flag | FWRITE))
    goto fail2;

  mtx_unlock(&fdt->fdt_mtx);
  *resultf = f;
  return 0;

fail2:
  file_drop(f);
fail:
  mtx_unlock(&fdt->fdt_mtx);
  return -EBADF;
}

int file_get(thread_t *td, int fd, file_t **f) {
  return _file_get(td, fd, 0, f);
}
int file_get_read(thread_t *td, int fd, file_t **f) {
  return _file_get(td, fd, FREAD, f);
}
int file_get_write(thread_t *td, int fd, file_t **f) {
  return _file_get(td, fd, FWRITE, f);
}

/* Closes a file descriptor. If it was the last reference to a file, the file is
 * also closed. */
int file_desc_close(file_desc_table_t *fdt, int fd) {
  mtx_lock(&fdt->fdt_mtx);

  if (fd < 0 || fd > fdt->fdt_nfiles || !file_desc_isused(fdt, fd))
    return -EBADF;

  file_desc_free(fdt, fd);

  mtx_unlock(&fdt->fdt_mtx);

  return 0;
}

/* Operations on invalid file descriptors */
static int badfo_read(file_t *f, struct thread *td, char *buf, size_t count) {
  return -EBADF;
}
static int badfo_write(file_t *f, struct thread *td, char *buf, size_t count) {
  return -EBADF;
}
static int badfo_close(file_t *f, struct thread *td) {
  return -EBADF;
}

fileops_t badfileops = {
  .fo_read = badfo_read, .fo_write = badfo_write, .fo_close = badfo_close,
};
