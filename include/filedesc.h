#ifndef _SYS_FILEDESC_H_
#define _SYS_FILEDESC_H_
#include <file.h>
#include <bitstring.h>
#include <mutex.h>

typedef struct thread thread_t;

/* The initial size of space allocated for file descriptors. According
   to FreeBSD, this is more than enough for most applications. Each
   process starts with this many descriptors, and more are allocated
   on demand. */
#define NDFILE 20
/* Separate macro defining a hard limit on open files. */
#define MAXFILES 20

typedef struct fd_table {
  file_t *fdt_files[NDFILE];             /* Open files array */
  bitstr_t fdt_map[bitstr_size(NDFILE)]; /* Bitmap of used fds */
  int fdt_nfiles;                        /* Number of files allocated */
  uint32_t fdt_count;                    /* Reference count */
  mtx_t fdt_mtx;
} fd_table_t;

void fd_table_ref(fd_table_t *fdt);
void fd_table_unref(fd_table_t *fdt);

/* Prepares memory pools for files and file descriptors. */
void fds_init();

/* Allocates a new descriptor table. */
fd_table_t *fd_table_init();
/* Allocates a new descriptor table making it a copy of an existing one. */
fd_table_t *fd_table_copy(fd_table_t *fdt);
/* Removes a reference to file descriptor table. If this was the last reference,
 * it also frees the table and possibly closes underlying files. */
void fd_table_destroy(fd_table_t *fdt);

/* Allocate a new file structure, but do not install it as a descriptor. */
file_t *file_alloc_noinstall();
/* Install a file structure to a new descriptor. */
int file_install_desc(fd_table_t *fdt, file_t *f, int *fdp);
/* Allocates a new file structure and installs it to a new descriptor.
 * On success, returns 0 and sets *fp and *fdp to the file struct and
 * descriptor number respectively. */
int file_alloc_install_desc(fd_table_t *fdt, file_t **fp, int *fdp);

/* Extracts a reference to file from a descriptor number, for a particular
 * thread. */
int fd_file_get(thread_t *td, int fd, int flags, file_t **fp);

/* Closes a file descriptor. If it was the last reference to a file, the file is
 * also closed. */
int fd_close(fd_table_t *fdt, int fd);

extern fileops_t badfileops;

#endif /* !_SYS_FILEDESC_H_ */
