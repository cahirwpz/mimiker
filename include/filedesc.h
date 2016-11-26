#ifndef __FILEDESC_H__
#define __FILEDESC_H__
#include <file.h>
#include <bitstring.h>
#include <mutex.h>

struct filedescent {
  struct file *fde_file; /* Link to file structure */
  /* Also: Per-process and per-descriptor file struct overrrides */
};

struct filedesc {
  struct filedescent *fd_ofiles; /* Open files array */
  bitstr_t *fd_map;              /* Bitmap of used fds */
  int fd_nfiles;                 /* Number of files allocated */
  uint32_t fd_refcount;          /* Thread reference count */
  mtx_t fd_mtx;
};

void fd_init();

extern struct fileops badfileops;

#endif /* __FILEDESC_H__ */
