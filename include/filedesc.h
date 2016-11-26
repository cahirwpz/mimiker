#ifndef __FILEDESC_H__
#define __FILEDESC_H__
#include <file.h>
#include <bitstring.h>
#include <mutex.h>

typedef struct {
  file_t **fdt_ofiles; /* Open files array */
  bitstr_t *fdt_map;   /* Bitmap of used fds */
  int fdt_nfiles;      /* Number of files allocated */
  uint32_t fdt_count;  /* Thread reference count */
  mtx_t fdt_mtx;
} file_desc_table_t;

void file_desc_init();

/* Allocates a new descriptor table. */
file_desc_table_t *file_desc_table_init();
/* Allocates a new descriptor table making it a copy of an existing one. */
file_desc_table_t *filedesc_copy(file_desc_table_t *fdt);

void file_desc_table_free(file_desc_table_t *fdt);

extern fileops_t badfileops;

#endif /* __FILEDESC_H__ */
