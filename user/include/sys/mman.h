#include <stdint.h>

/* Newlib does not provide mmap prototype, so we need to use our own. */

#define MMAP_FLAG_ANONYMOUS 1

#define MMAP_PROT_NONE 0
#define MMAP_PROT_READ 1
#define MMAP_PROT_WRITE 2
#define MMAP_PROT_EXEC 4

void *mmap(void *addr, size_t length, int prot, int flags);
