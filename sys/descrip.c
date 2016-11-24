#include <file.h>
#include <fildesc.h>
#include <malloc.h>

/* The initial size of space allocated for file descriptors. According
   to FreeBSD, this is more than enough for most applications. Each
   process starts with this many descriptors, and more are allocated
   on demand. */
#define NDFILE 20

/* In FreeBSD this function takes a filedesc* argument, so that
   current dir may be copied. Since we don't use these fields, this
   argument doest not make sense yet. */
struct filedesc* fdinit(){
    struct filedesc* fdesc;
    fdesc = kmalloc(mpool, sizeof(struct filedesc), M_ZERO);
    fdesc->fd_ofiles = kmalloc(mpool, NDFILE * sizeof(struct filedecsent), M_ZERO);
    fdesc->fd_map = kmalloc(mpool, bitstr_size(NDFILE) * sizeof(bitstr_t), M_ZERO);
    fdesc->nfiles = NDFILE;
    fdesc->fd_refcount = 1;
    return fdesc;
}

int fdisused(struct filedesc* fdesc, int fd){
    return bit_test(fdesc->fd_map, fd);
}

