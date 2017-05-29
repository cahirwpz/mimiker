#ifndef _SYS_FILEDESC_H_
#define _SYS_FILEDESC_H_

#include <file.h>
#include <bitstring.h>
#include <mutex.h>

/* The initial size of space allocated for file descriptors. According
   to FreeBSD, this is more than enough for most applications. Each
   process starts with this many descriptors, and more are allocated
   on demand. */
#define NDFILE 20U
/* Separate macro defining a hard limit on open files. */
#define MAXFILES 1024U

typedef struct fdtab {
  file_t **fdt_files; /* Open files array */
  bitstr_t *fdt_map;  /* Bitmap of used fds */
  unsigned fdt_flags;
  unsigned fdt_nfiles; /* Number of files allocated */
  int fdt_count;       /* Reference count, ready for disposal if -1 */
  mtx_t fdt_mtx;
} fdtab_t;

void fdtab_ref(fdtab_t *fdt);
void fdtab_unref(fdtab_t *fdt);

/* Allocates a new descriptor table. */
fdtab_t *fdtab_alloc();
/* Allocates a new descriptor table making it a copy of an existing one. */
fdtab_t *fdtab_copy(fdtab_t *fdt);
/* Frees the table and possibly closes underlying files. */
void fdtab_destroy(fdtab_t *fdt);
/* Drop reference counter and possibly destroy the table. */
void fdtab_release(fdtab_t *fdt);
/* Assign a file structure to a new descriptor. */
int fdtab_install_file(fdtab_t *fdt, file_t *f, int *fdp);
/* Assign a file structure to a certain descriptor */
int fdtab_install_file_at(fdtab_t *fdt, file_t *f, int fdp);
/* Extracts a reference to file from descriptor table for given number. */
int fdtab_get_file(fdtab_t *fdt, int fd, int flags, file_t **fp);
/* Closes a file descriptor.
 * If it was the last reference to a file, the file is closed as well. */
int fdtab_close_fd(fdtab_t *fdt, int fd);

#endif /* !_SYS_FILEDESC_H_ */
