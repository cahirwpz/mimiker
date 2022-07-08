#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/malloc.h>
#include <sys/libkern.h>
#include <sys/errno.h>
#include <sys/mutex.h>
#include <sys/refcnt.h>
#include <bitstring.h>

static KMALLOC_DEFINE(M_FD, "filedesc");

/* The initial size of space allocated for file descriptors. According
   to FreeBSD, this is more than enough for most applications. Each
   process starts with this many descriptors, and more are allocated
   on demand. */
#define NDFILE 20
/* Separate macro defining a hard limit on open files. */
#define MAXFILES 1024

typedef struct fdent {
  file_t *fde_file;
  bool fde_cloexec;
} fdent_t;

struct fdtab {
  fdent_t *fdt_entries; /* Open files array */
  bitstr_t *fdt_map;    /* Bitmap of used fds */
  unsigned fdt_flags;
  int fdt_nfiles;     /* Number of files allocated */
  refcnt_t fdt_count; /* Reference count */
  mtx_t fdt_mtx;
};

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

/* Grows given file descriptor table to contain new_size file descriptors
 * (up to MAXFILES) */
static void fd_growtable(fdtab_t *fdt, int new_size) {
  assert(fdt->fdt_nfiles < new_size && new_size <= MAXFILES);
  assert(mtx_owned(&fdt->fdt_mtx));

  fdent_t *old_fdt_entries = fdt->fdt_entries;
  bitstr_t *old_fdt_map = fdt->fdt_map;

  fdent_t *new_fdt_entries = kmalloc(M_FD, sizeof(fdent_t) * new_size, M_ZERO);
  bitstr_t *new_fdt_map = kmalloc(M_FD, bitstr_size(new_size), M_ZERO);

  memcpy(new_fdt_entries, old_fdt_entries, sizeof(fdent_t) * fdt->fdt_nfiles);
  memcpy(new_fdt_map, old_fdt_map, bitstr_size(fdt->fdt_nfiles));
  kfree(M_FD, old_fdt_entries);
  kfree(M_FD, old_fdt_map);

  fdt->fdt_entries = new_fdt_entries;
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
  fdent_t *fde = &fdt->fdt_entries[fd];
  assert(fde->fde_file != NULL);
  file_drop(fde->fde_file);
  fde->fde_file = NULL;
  fde->fde_cloexec = false;
  fd_mark_unused(fdt, fd);
}

/* Create empty file descriptor table. */
fdtab_t *fdtab_create(void) {
  fdtab_t *fdt = kmalloc(M_FD, sizeof(fdtab_t), M_ZERO);
  fdt->fdt_nfiles = NDFILE;
  fdt->fdt_entries = kmalloc(M_FD, sizeof(fdent_t) * NDFILE, M_ZERO);
  fdt->fdt_map = kmalloc(M_FD, bitstr_size(NDFILE), M_ZERO);
  fdt->fdt_count = 1;
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
      fdent_t *f = &fdt->fdt_entries[i];
      newfdt->fdt_entries[i] = *f;
      file_hold(f->fde_file);
    }
  }

  memcpy(newfdt->fdt_map, fdt->fdt_map,
         sizeof(bitstr_t) * bitstr_size(fdt->fdt_nfiles));

  return newfdt;
}

void fdtab_drop(fdtab_t *fdt) {
  if (!refcnt_release(&fdt->fdt_count))
    return;

  assert(fdt->fdt_count <= 0);
  /* No need to lock mutex, we have the only reference left. */

  /* Clean up used descriptors. This possibly closes underlying files. */
  for (int i = 0; i < fdt->fdt_nfiles; i++)
    if (fd_is_used(fdt, i))
      fd_free(fdt, i);

  kfree(M_FD, fdt->fdt_entries);
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
  fdt->fdt_entries[*fd].fde_file = f;
  fdt->fdt_entries[*fd].fde_cloexec = false;
  file_hold(f);
  return 0;
}

int fdtab_install_file_at(fdtab_t *fdt, file_t *f, int fd) {
  assert(f != NULL);
  assert(fdt != NULL);

  WITH_MTX_LOCK (&fdt->fdt_mtx) {
    if (is_bad_fd(fdt, fd))
      return EBADF;

    fdent_t *fde = &fdt->fdt_entries[fd];

    if (fd_is_used(fdt, fd)) {
      if (fde->fde_file == f)
        break;
      fd_free(fdt, fd);
    }
    fde->fde_file = f;
    fde->fde_cloexec = false;
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

    f = fdt->fdt_entries[fd].fde_file;
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

int fd_set_cloexec(fdtab_t *fdt, int fd, bool cloexec) {
  SCOPED_MTX_LOCK(&fdt->fdt_mtx);

  if (is_bad_fd(fdt, fd) || !fd_is_used(fdt, fd))
    return EBADF;

  fdt->fdt_entries[fd].fde_cloexec = cloexec;
  return 0;
}

int fd_get_cloexec(fdtab_t *fdt, int fd, int *resp) {
  SCOPED_MTX_LOCK(&fdt->fdt_mtx);

  if (is_bad_fd(fdt, fd) || !fd_is_used(fdt, fd))
    return EBADF;

  *resp = fdt->fdt_entries[fd].fde_cloexec;
  return 0;
}

int fdtab_onexec(fdtab_t *fdt) {
  int error;
  for (int fd = 0; fd < fdt->fdt_nfiles; ++fd) {
    if (fd_is_used(fdt, fd) && fdt->fdt_entries[fd].fde_cloexec)
      if ((error = fdtab_close_fd(fdt, fd)))
        return error;
  }
  return 0;
}

int fdtab_onfork(fdtab_t *fdt) {
  int error;
  for (int fd = 0; fd < fdt->fdt_nfiles; ++fd) {
    /* Kqueues aren't inherited by a child created with fork. */
    if (fd_is_used(fdt, fd) &&
        fdt->fdt_entries[fd].fde_file->f_type == FT_KQUEUE)
      if ((error = fdtab_close_fd(fdt, fd)))
        return error;
  }
  return 0;
}
