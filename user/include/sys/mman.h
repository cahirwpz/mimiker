#include <stdint.h>

/* Newlib does not provide mmap prototype, so we need to use our own. */

#define MMAP_ANON 1

#define PROT_NONE 0
#define PROT_READ 1
#define PROT_WRITE 2
#define PROT_EXEC 4

void *mmap(void *addr, size_t length, int prot, int flags);
