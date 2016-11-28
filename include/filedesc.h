#ifndef __FILEDESC_H__
#define __FILEDESC_H__
#include <file.h>
#include <bitstring.h>
#include <mutex.h>

typedef struct thread thread_t;

typedef struct {
  file_t **fdt_files; /* Open files array */
  bitstr_t *fdt_map;  /* Bitmap of used fds */
  int fdt_nfiles;     /* Number of files allocated */
  uint32_t fdt_count; /* Reference count */
  mtx_t fdt_mtx;
} file_desc_table_t;

/* Prepares memory pools for files and file descriptors. */
void file_desc_init();

/* Allocates a new descriptor table. */
file_desc_table_t *file_desc_table_init();
/* Allocates a new descriptor table making it a copy of an existing one. */
file_desc_table_t *filedesc_copy(file_desc_table_t *fdt);
/* Removes a reference to file descriptor table. If this was the last reference,
 * it also frees the table and possibly closes underlying files. */
void file_desc_table_destroy(file_desc_table_t *fdt);

/* Allocate a new file structure, but do not install it as a descriptor. */
file_t *file_alloc_noinstall();
/* Install a file structure to a new descriptor. */
int file_install_desc(file_desc_table_t *fdt, file_t *f, int *fd);
/* Allocates a new file structure and installs it to a new descriptor. On
 * success, returns 0 and sets *resultf and *resultfd to the file struct and
 * descriptor no respectively. */
int file_alloc_install_desc(file_desc_table_t *fdt, file_t **resultf,
                            int *resultfd);

/* Extracts a reference to file from a descriptor number, for a particular
 * thread. */
int file_get(thread_t *td, int fd, file_t **f);
int file_get_read(thread_t *td, int fd, file_t **f);
int file_get_write(thread_t *td, int fd, file_t **f);

/* Closes a file descriptor. If it was the last reference to a file, the file is
 * also closed. */
int file_desc_close(file_desc_table_t *fdt, int fd);

extern fileops_t badfileops;

#endif /* __FILEDESC_H__ */
