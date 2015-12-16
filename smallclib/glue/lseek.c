#include <unistd.h>
#include <errno.h>

off_t lseek(int fd, off_t offset, int whence){
    errno = ENOTSUP;
    return (off_t)-1;
}
