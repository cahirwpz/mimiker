#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <sys/uio.h>

static int iov_validate(const struct iovec *iov, int iovcnt) {
  if (iovcnt <= 0 || iovcnt > IOV_MAX)
    return EINVAL;
  size_t len = 0;
  for (int i = 0; i < iovcnt; i++) {
    len += iov[i].iov_len;
    if (len > SSIZE_MAX || iov[i].iov_len > SSIZE_MAX)
      return EINVAL;
  }
  return 0;
}

/* XXX: ideally, this should be a syscall that does all this atomically,
 * same for readv(). */
ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
  int error;
  if ((error = iov_validate(iov, iovcnt)))
    return error;
  ssize_t done = 0;
  off_t off = lseek(fd, 0, SEEK_CUR);
  for (int i = 0; i < iovcnt; i++) {
    ssize_t ret = write(fd, iov[i].iov_base, iov[i].iov_len);
    if (ret < 0) {
      /* Reset file pointer to the old position. */
      lseek(fd, off, SEEK_SET);
      return ret;
    }
    done += iov[i].iov_len;
    if ((size_t)ret < iov[i].iov_len)
      break;
  }
  return done;
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
  int error;
  if ((error = iov_validate(iov, iovcnt)))
    return error;
  ssize_t done = 0;
  off_t off = lseek(fd, 0, SEEK_CUR);
  for (int i = 0; i < iovcnt; i++) {
    ssize_t ret = read(fd, iov[i].iov_base, iov[i].iov_len);
    if (ret < 0) {
      /* Reset file pointer to the old position. */
      lseek(fd, off, SEEK_SET);
      return ret;
    }
    done += iov[i].iov_len;
    if ((size_t)ret < iov[i].iov_len)
      break;
  }
  return done;
}
