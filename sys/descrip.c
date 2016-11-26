#include <file.h>
#include <filedesc.h>
#include <malloc.h>
#include <stdc.h>
#include <errno.h>
#include <mutex.h>

static MALLOC_DEFINE(fd_pool, "file descriptors pool");
static MALLOC_DEFINE(file_pool, "file pool");

/* The initial size of space allocated for file descriptors. According
   to FreeBSD, this is more than enough for most applications. Each
   process starts with this many descriptors, and more are allocated
   on demand. */
#define NDFILE 20
/* Separate macro defining a hard limit on open files. */
#define MAXFILES 20

void fd_init() {
  vm_page_t *pg = pm_alloc(2);
  kmalloc_init(fd_pool);
  kmalloc_add_arena(fd_pool, pg->vaddr, PG_SIZE(pg));
  vm_page_t *pg2 = pm_alloc(2);
  kmalloc_init(file_pool);
  kmalloc_add_arena(file_pool, pg2->vaddr, PG_SIZE(pg2));
}

/* Test whether a file descriptor is in use. */
static int filedesc_isused(struct filedesc *fdesc, int fd) {
  return bit_test(fdesc->fd_map, fd);
}

static void filedesc_mark_used(struct filedesc *fdesc, int fd) {
  assert(!filedesc_isused(fdesc, fd));
  bit_set(fdesc->fd_map, fd);
}

/* Called when the last reference to a file is dropped. */
void file_free(struct file *f) {
  assert(mtx_is_locked(&f->f_mtx));
  assert(f->f_count == 0);

  /* TODO: Do not close an invalid file (badfileops) */
  /* TODO: What if an error happens during close? */
  f->f_ops.fo_close(f, thread_self());

  kfree(file_pool, f);
}

void file_hold(struct file *f) {
  mtx_lock(&f->f_mtx);
  ++f->f_count;
  mtx_unlock(&f->f_mtx);
}

void file_drop(struct file *f) {
  mtx_lock(&f->f_mtx);
  if (0 == --f->f_count)
    file_free(f);
  mtx_unlock(&f->f_mtx);
}

/* Allocates a new file descriptor in a file descriptor table. Returns 0 on
 * success and sets *result to new descriptor number. Must be called with
 * fd->fd_mtx already locked. */
int filedesc_alloc(struct filedesc *fd, int *result) {
  assert(mtx_is_locked(&fd->fd_mtx));

  int first_free = MAXFILES;
  bit_ffc(fd->fd_map, fd->fd_nfiles, &first_free);

  if (first_free >= fd->fd_nfiles) {
    /* No more space to allocate a descriptor! The descriptor table should grow,
       but we can't do that yet. */
    return -EMFILE;
  }
  filedesc_mark_used(fd, first_free);
  *result = first_free;
  return 0;
}

/* Allocate a new file structure, but do not install it as a descriptor. */
struct file *file_alloc_noinstall() {
  struct file *f;
  f = kmalloc(file_pool, sizeof(struct file), M_ZERO);
  f->f_ops = badfileops;
  f->f_count = 1;
  f->f_data = 0;
  return f;
}

/* Install a file structure to a new descriptor. */
int file_install(struct filedesc *fdesc, struct file *f, int *fd) {
  assert(f != NULL);
  assert(fd != NULL);

  mtx_lock(&fdesc->fd_mtx);
  int res = filedesc_alloc(fdesc, fd);
  if (res < 0) {
    mtx_unlock(&fdesc->fd_mtx);
    return res;
  }

  struct filedescent *fde = &fdesc->fd_ofiles[*fd];
  fde->fde_file = f;
  file_hold(f);
  mtx_unlock(&fdesc->fd_mtx);
  return 0;
}

/* Allocates a new file structure and installs it to a new descriptor. On
 * success, returns 0 and sets *resultf and *resultfd to the file struct and
 * descriptor no respectively. */
int file_alloc_install(struct filedesc *fdesc, struct file **resultf,
                       int *resultfd) {
  struct file *f;
  int fd;
  f = file_alloc_noinstall();
  int res = file_install(fdesc, f, &fd);
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
struct filedesc *fdinit() {
  struct filedesc *fdesc;
  fdesc = kmalloc(fd_pool, sizeof(struct filedesc), M_ZERO);
  fdesc->fd_ofiles =
    kmalloc(fd_pool, NDFILE * sizeof(struct filedescent), M_ZERO);
  fdesc->fd_map =
    kmalloc(fd_pool, bitstr_size(NDFILE) * sizeof(bitstr_t), M_ZERO);
  fdesc->fd_nfiles = NDFILE;
  fdesc->fd_refcount = 1;
  return fdesc;
}

struct filedesc *fdcopy(struct filedesc *fd) {
  struct filedesc *newfd;

  newfd = fdinit();

  if (!fd)
    return newfd;

  mtx_lock(&fd->fd_mtx);

  /* We can assume both filedescs use 20 descriptors, because we don't support
   * other numbers. */
  assert(fd->fd_nfiles == newfd->fd_nfiles);

  for (int i = 0; i < fd->fd_nfiles; i++) {
    struct filedescent *fde = &fd->fd_ofiles[i];

    newfd->fd_ofiles[i] = *fde;

    file_hold(fde->fde_file);
  }

  mtx_unlock(&fd->fd_mtx);

  return newfd;
}

/* Operations on invalid file descriptors */
static int badfo_read(struct file *f, struct thread *td, char *buf,
                      size_t count) {
  return -EBADF;
}
static int badfo_write(struct file *f, struct thread *td, char *buf,
                       size_t count) {
  return -EBADF;
}
static int badfo_close(struct file *f, struct thread *td) {
  return -EBADF;
}

struct fileops badfileops = {
  .fo_read = badfo_read, .fo_write = badfo_write, .fo_close = badfo_close,
};
