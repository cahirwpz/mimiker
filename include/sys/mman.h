#ifndef _SYS_MMAN_H_
#define _SYS_MMAN_H_

#include <sys/types.h>

#define MAP_FAILED ((void *)-1)

/* Mapping type */
#define MAP_FILE 0x0000  /* map from file (default) */
#define MAP_ANON 0x1000  /* anonymous memory (stored in swap) */
#define MAP_STACK 0x2000 /* as above, grows down automatically */

/* Flags contain sharing type - exactly one of them must be specified! */
#define MAP_SHARED 0x0001
#define MAP_PRIVATE 0x0002

/* Extra flags for mmap. */
#define MAP_FIXED 0x0004

/* Protections for mapped pages (bitwise or'ed). */
#define PROT_NONE 0
#define PROT_READ 1
#define PROT_WRITE 2
#define PROT_EXEC 4

/* Original advice values, equivalent to POSIX definitions. */
#define MADV_NORMAL 0     /* No further special treatment */
#define MADV_RANDOM 1     /* Expect random page references */
#define MADV_SEQUENTIAL 2 /* Expect sequential page references */

#ifndef _KERNEL

/* Newlib does not provide mmap prototype, so we need to use our own. */
void *mmap(void *addr, size_t length, int prot, int flags, int fd,
           off_t offset);
int munmap(void *addr, size_t len);
int mprotect(void *addr, size_t len, int prot);
int madvise(void *addr, size_t len, int advice);

#endif /* !_KERNEL */

#endif /* !_SYS_MMAN_H_ */
