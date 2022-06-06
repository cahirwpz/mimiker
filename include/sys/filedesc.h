#ifndef _SYS_FILEDESC_H_
#define _SYS_FILEDESC_H_

#include <stdbool.h>

typedef struct file file_t;
typedef struct fdtab fdtab_t;
typedef enum filetype filetype_t;

/*! \brief Increments reference counter. */
void fdtab_hold(fdtab_t *fdt);
/*! \brief Decrements `fdt_count` and destroys fd table if it has reached 0. */
void fdtab_drop(fdtab_t *fdt);

/* Allocates a new descriptor table. */
fdtab_t *fdtab_create(void);
/* Allocates a new descriptor table making it a copy of an existing one. */
fdtab_t *fdtab_copy(fdtab_t *fdt);
/* Assign a file structure to a new descriptor with number >= minfd.
 * Increments reference counter of `f` file. */
int fdtab_install_file(fdtab_t *fdt, file_t *f, int minfd, int *fdp);
/* Assign a file structure to a certain descriptor number.
 * Increments reference counter of `f` file. */
int fdtab_install_file_at(fdtab_t *fdt, file_t *f, int fd);
/* Extracts a reference to file from descriptor table for given number.
 * Increments reference counter of `f` file on success. */
int fdtab_get_file(fdtab_t *fdt, int fd, int flags, file_t **fp);
/* Closes a file descriptor.
 * If it was the last reference to a file, the file is closed as well. */
int fdtab_close_fd(fdtab_t *fdt, int fd);
/* Set cloexec flag. */
int fd_set_cloexec(fdtab_t *fdt, int fd, bool exclose);
/* Get cloexec flag. */
int fd_get_cloexec(fdtab_t *fdt, int fd, int *resp);
/* Close file descriptors with cloexec flag. */
int fdtab_onexec(fdtab_t *fdt);
/* Close file descriptors which cannot be inherited by forked process. */
int fdtab_onfork(fdtab_t *fdt);

#endif /* !_SYS_FILEDESC_H_ */
