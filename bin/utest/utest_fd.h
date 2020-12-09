#ifndef FD_OFFSET
#define FD_OFFSET 0
#endif

#undef assert_open_ok
#define assert_open_ok(fd, file, mode, flag)                                   \
  n = open(file, flag, mode);                                                     \
  assert(n == fd + FD_OFFSET);

#undef assert_open_fail
#define assert_open_fail(file, mode, flag, err)                                \
  n = open(file, flag, 0);                                                     \
  assert(n < 0);                                                               \
  assert(errno == err);

#undef assert_read_ok
#define assert_read_ok(fd, buf, len)                                           \
  n = read(fd + FD_OFFSET, buf, len);                                          \
  assert(n >= 0);

#undef assert_read_equal
#define assert_read_equal(fd, buf, str)                                        \
  {                                                                            \
    int len = strlen(str);                                                     \
    n = read(fd + FD_OFFSET, buf, len);                                        \
    assert(strncmp(str, buf, len) == 0);                                       \
    assert(n >= 0);                                                            \
  }

#undef assert_read_fail
#define assert_read_fail(fd, buf, len, err)                                    \
  n = read(fd + FD_OFFSET, buf, len);                                          \
  assert(n < 0);                                                               \
  assert(errno == err);

#undef assert_write_ok
#define assert_write_ok(fd, buf, len)                                          \
  n = write(fd + FD_OFFSET, buf, len);                                         \
  assert(n >= 0);

#undef assert_write_fail
#define assert_write_fail(fd, buf, len, err)                                   \
  n = write(fd + FD_OFFSET, buf, len);                                         \
  assert(n < 0);                                                               \
  assert(errno == err);

#undef assert_close_ok
#define assert_close_ok(fd)                                                    \
  n = close(fd + FD_OFFSET);                                                   \
  assert(n == 0);

#undef assert_close_fail
#define assert_close_fail(fd, err)                                             \
  n = close(fd + FD_OFFSET);                                                   \
  assert(n < 0);                                                               \
  assert(errno == err);

#undef assert_lseek_ok
#define assert_lseek_ok(fd, offset, whence)                                    \
  n = lseek(fd + FD_OFFSET, offset, whence);                                   \
  assert(n >= 0);

#undef assert_pipe_ok
#define assert_pipe_ok(fds)                                                    \
  n = pipe(fds);                                                               \
  assert(n == 0);
