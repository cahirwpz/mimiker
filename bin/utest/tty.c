#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/termios.h>
#include <sys/filio.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <errno.h>

#include "utest.h"
#include "util.h"

int test_tty_canon(void) {
  int master_fd, slave_fd;
  open_pty(&master_fd, &slave_fd);

  /* Set canonical mode. */
  struct termios t;
  assert(tcgetattr(slave_fd, &t) == 0);
  t.c_lflag |= ICANON;
  t.c_lflag &= ~(ECHO | ECHONL);
  assert(tcsetattr(slave_fd, TCSANOW, &t) == 0);

  /* Write an incomplete line to the master side -- the slave side shouldn't be
   * able to read any of it yet. */
  assert(write(master_fd, "hello", 5) == 5);
  int avail;
  assert(ioctl(slave_fd, FIONREAD, &avail) == 0);
  assert(avail == 0);

  /* Erase 2 characters and type them back in. */
  unsigned char verase = t.c_cc[VERASE];
  assert(verase != _POSIX_VDISABLE);
  assert(write(master_fd, &verase, 1) == 1);
  assert(write(master_fd, &verase, 1) == 1);
  assert(write(master_fd, "lo", 2) == 2);

  /* Still, no input should be available on the slave side. */
  assert(ioctl(slave_fd, FIONREAD, &avail) == 0);
  assert(avail == 0);

  /* Finish the line. The slave side should be able to read "hello\n". */
  assert(write(master_fd, &(char){'\n'}, 1) == 1);

  /* Trying to erase characters shouldn't affect the finished line. */
  assert(write(master_fd, &verase, 1) == 1);

  char buf[8];
  assert(read(slave_fd, buf, sizeof(buf)) == 6);
  assert(strncmp(buf, "hello\n", 6) == 0);

  /* We should have consumed all input. */
  assert(ioctl(slave_fd, FIONREAD, &avail) == 0);
  assert(avail == 0);

  /* Write 2 lines and verify that only 1 line can be read at a time. */
  assert(write(master_fd, "foo\nbar\n", 8) == 8);
  assert(read(slave_fd, buf, sizeof(buf)) == 4);
  assert(strncmp(buf, "foo\n", 4) == 0);

  /* After reading the first line, there should 4 bytes left available. */
  assert(ioctl(slave_fd, FIONREAD, &avail) == 0);
  assert(avail == 4);

  /* Write an incomplete line and erase the whole line using VKILL. */
  assert(write(master_fd, "hello", 5) == 5);
  unsigned char vkill = t.c_cc[VKILL];
  assert(vkill != _POSIX_VDISABLE);
  assert(write(master_fd, &vkill, 1) == 1);
  assert(write(master_fd, "baz\n", 4) == 4);

  assert(ioctl(slave_fd, FIONREAD, &avail) == 0);
  assert(avail == 8);

  assert(read(slave_fd, buf, sizeof(buf)) == 4);
  assert(strncmp(buf, "bar\n", 4) == 0);
  assert(read(slave_fd, buf, sizeof(buf)) == 4);
  assert(strncmp(buf, "baz\n", 4) == 0);

  return 0;
}

int test_tty_echo(void) {
  int master_fd, slave_fd;
  open_pty(&master_fd, &slave_fd);

  /* Set raw mode and ECHO flag. */
  struct termios t;
  assert(tcgetattr(slave_fd, &t) == 0);
  cfmakeraw(&t);
  t.c_lflag &= ~ECHOCTL;
  t.c_lflag |= ECHO;
  assert(tcsetattr(slave_fd, TCSANOW, &t) == 0);

  /* Non-control characters should be echoed. */
  assert(write(master_fd, "hello", 5) == 5);
  char buf[8];
  assert(read(master_fd, buf, 5) == 5);
  assert(strncmp(buf, "hello", 5) == 0);

  /* Control characters should be echoed verbatim unless
   * the ECHOCTL flag is set. */
#define CTRL(x) (x & 037)
  assert(write(master_fd, &(char){CTRL('c')}, 1) == 1);
  assert(write(master_fd, "hello", 5) == 5);
  assert(read(master_fd, buf, 6) == 6);
  assert(buf[0] == CTRL('c'));
  assert(strncmp(buf + 1, "hello", 5) == 0);

  t.c_lflag |= ECHOCTL;
  assert(tcsetattr(slave_fd, TCSANOW, &t) == 0);

  assert(write(master_fd, &(char){CTRL('c')}, 1) == 1);
  assert(write(master_fd, "hello", 5) == 5);
  assert(read(master_fd, buf, 7) == 7);
  assert(strncmp(buf, "^Chello", 7) == 0);

  /* Turn off ECHO, but turn on ECHONL. Only newlines should be echoed. */
  t.c_lflag &= ~(ECHO | ECHOCTL);
  t.c_lflag |= ECHONL;
  assert(tcsetattr(slave_fd, TCSANOW, &t) == 0);

  assert(write(master_fd, "hello\n", 6) == 6);
  assert(read(master_fd, buf, 1) == 1);
  assert(buf[0] == '\n');

  return 0;
}
