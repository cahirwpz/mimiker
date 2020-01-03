#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/malloc.h>
#include <sys/libkern.h>
#include <sys/errno.h>
#include <sys/mutex.h>

static KMALLOC_DEFINE(M_FD, "filedesc", PAGESIZE * 2);

/* Test whether a file descriptor is in use. */
static int fd_is_used(fdtab_t *fdt, int fd) {
  return bit_test(fdt->fdt_map, fd);
}

static void fd_mark_used(fdtab_t *fdt, int fd) {
  assert(!fd_is_used(fdt, fd));
  bit_set(fdt->fdt_map, fd);
}

static void fd_mark_unused(fdtab_t *fdt, int fd) {
  assert(fd_is_used(fdt, fd));
  bit_clear(fdt->fdt_map, fd);
}

static inline bool is_bad_fd(fdtab_t *fdt, int fd) {
  return (fd < 0 || fd > (int)fdt->fdt_nfiles);
}

void fdtab_hold(fdtab_t *fdt) {
  refcnt_acquire(&fdt->fdt_count);
}

void fdtab_drop(fdtab_t *fdt) {
  if (refcnt_release(&fdt->fdt_count))
    fdtab_destroy(fdt);
}

/* Grows given file descriptor table to contain new_size file descriptors
 * (up to MAXFILES) */
static void fd_growtable(fdtab_t *fdt, int new_size) {
  assert(fdt->fdt_nfiles < new_size && new_size <= MAXFILES);
  assert(mtx_owned(&fdt->fdt_mtx));

  file_t **old_fdt_files = fdt->fdt_files;
  bitstr_t *old_fdt_map = fdt->fdt_map;

  file_t **new_fdt_files = kmalloc(M_FD, sizeof(file_t *) * new_size, M_ZERO);
  bitstr_t *new_fdt_map = kmalloc(M_FD, bitstr_size(new_size), M_ZERO);

  memcpy(new_fdt_files, old_fdt_files, sizeof(file_t *) * fdt->fdt_nfiles);
  memcpy(new_fdt_map, old_fdt_map, bitstr_size(fdt->fdt_nfiles));
  kfree(M_FD, old_fdt_files);
  kfree(M_FD, old_fdt_map);

  fdt->fdt_files = new_fdt_files;
  fdt->fdt_map = new_fdt_map;
  fdt->fdt_nfiles = new_size;
}

/* Allocates a new file descriptor in a file descriptor table.
 * The new file descriptor will be at least equal to minfd.
 * Returns 0 on success and sets *result to new descriptor number.
 * Must be called with fd->fd_mtx already locked. */
static int fd_alloc(fdtab_t *fdt, int minfd, int *fdp) {
  assert(mtx_owned(&fdt->fdt_mtx));

  if (minfd >= MAXFILES)
    return EMFILE;

  int first_free;
  bit_ffc_from(fdt->fdt_map, fdt->fdt_nfiles, minfd, &first_free);

  if (first_free < 0) {
    /* No more space to allocate a descriptor... grow describtor table! */
    if (fdt->fdt_nfiles == MAXFILES) {
      /* Reached limit of opened files. */
      return EMFILE;
    }
    int new_size = min(max(minfd + 1, fdt->fdt_nfiles * 2), MAXFILES);
    first_free = max(minfd, fdt->fdt_nfiles);
    fd_growtable(fdt, new_size);
  }
  fd_mark_used(fdt, first_free);
  *fdp = first_free;
  return 0;
}

static void fd_free(fdtab_t *fdt, int fd) {
  file_t *f = fdt->fdt_files[fd];
  assert(f != NULL);
  file_drop(f);
  fdt->fdt_files[fd] = NULL;
  fd_mark_unused(fdt, fd);
}

/* Create empty file descriptor table. */
fdtab_t *fdtab_create(void) {
  fdtab_t *fdt = kmalloc(M_FD, sizeof(fdtab_t), M_ZERO);
  fdt->fdt_nfiles = NDFILE;
  fdt->fdt_files = kmalloc(M_FD, sizeof(file_t *) * NDFILE, M_ZERO);
  fdt->fdt_map = kmalloc(M_FD, bitstr_size(NDFILE), M_ZERO);
  mtx_init(&fdt->fdt_mtx, 0);
  return fdt;
}

fdtab_t *fdtab_copy(fdtab_t *fdt) {
  fdtab_t *newfdt = fdtab_create();

  if (fdt == NULL)
    return newfdt;

  SCOPED_MTX_LOCK(&fdt->fdt_mtx);

  if (fdt->fdt_nfiles > newfdt->fdt_nfiles) {
    SCOPED_MTX_LOCK(&newfdt->fdt_mtx);
    fd_growtable(newfdt, fdt->fdt_nfiles);
  }

  for (int i = 0; i < fdt->fdt_nfiles; i++) {
    if (fd_is_used(fdt, i)) {
      file_t *f = fdt->fdt_files[i];
      newfdt->fdt_files[i] = f;
      file_hold(f);
    }
  }

  memcpy(newfdt->fdt_map, fdt->fdt_map,
         sizeof(bitstr_t) * bitstr_size(fdt->fdt_nfiles));

  fdtab_hold(newfdt);
  return newfdt;
}

void fdtab_destroy(fdtab_t *fdt) {
  assert(fdt->fdt_count <= 0);
  /* No need to lock mutex, we have the only reference left. */

  /* Clean up used descriptors. This possibly closes underlying files. */
  for (int i = 0; i < fdt->fdt_nfiles; i++)
    if (fd_is_used(fdt, i))
      fd_free(fdt, i);

  kfree(M_FD, fdt->fdt_files);
  kfree(M_FD, fdt->fdt_map);
  kfree(M_FD, fdt);
}

int fdtab_install_file(fdtab_t *fdt, file_t *f, int minfd, int *fd) {
  assert(f != NULL);
  assert(fd != NULL);

  SCOPED_MTX_LOCK(&fdt->fdt_mtx);

  int error;
  if ((error = fd_alloc(fdt, minfd, fd)))
    return error;
  fdt->fdt_files[*fd] = f;
  file_hold(f);
  return 0;
}

int fdtab_install_file_at(fdtab_t *fdt, file_t *f, int fd) {
  assert(f);
  assert(fdt);

  WITH_MTX_LOCK (&fdt->fdt_mtx) {
    if (is_bad_fd(fdt, fd))
      return EBADF;

    if (fd_is_used(fdt, fd)) {
      if (fdt->fdt_files[fd] == f)
        break;
      fd_free(fdt, fd);
    }
    fdt->fdt_files[fd] = f;
    fd_mark_used(fdt, fd);
  }

  file_hold(f);
  return 0;
}

/* Extracts file pointer from descriptor number in given table.
 * If flags are non-zero, returns EBADF if the file does not match flags. */
int fdtab_get_file(fdtab_t *fdt, int fd, int flags, file_t **fp) {
  if (!fdt)
    return EBADF;

  file_t *f = NULL;

  WITH_MTX_LOCK (&fdt->fdt_mtx) {
    if (is_bad_fd(fdt, fd) || !fd_is_used(fdt, fd))
      return EBADF;

    f = fdt->fdt_files[fd];
    file_hold(f);

    if ((flags & FF_READ) && !(f->f_flags & FF_READ))
      break;
    if ((flags & FF_WRITE) && !(f->f_flags & FF_WRITE))
      break;

    *fp = f;
    return 0;
  }

  file_drop(f);
  return EBADF;
}

/* Closes a file descriptor. If it was the last reference to a file, the file is
 * also closed. */
int fdtab_close_fd(fdtab_t *fdt, int fd) {
  SCOPED_MTX_LOCK(&fdt->fdt_mtx);

  if (is_bad_fd(fdt, fd) || !fd_is_used(fdt, fd))
    return EBADF;
  fd_free(fdt, fd);
  return 0;
}
