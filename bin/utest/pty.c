#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/termios.h>
#include <stdio.h>
#include <errno.h>

#include "utest.h"
#include "util.h"

static const char *test_str = "hello";

int test_pty_simple(void) {
  int master_fd, slave_fd;
  open_pty(&master_fd, &slave_fd);

  /* Put the tty into raw mode. */
  struct termios t;
  assert(tcgetattr(slave_fd, &t) == 0);
  cfmakeraw(&t);
  assert(tcsetattr(slave_fd, TCSANOW, &t) == 0);

  int len = strlen(test_str);

  /* Writes to the master device should appear to the slave device as input. */
  assert(write(master_fd, test_str, len) == len);
  char c;
  for (int i = 0; i < len; i++) {
    assert(read(slave_fd, &c, 1) == 1);
    assert(c == test_str[i]);
  }

  /* Writes to the slave device should appear to the master device as input. */
  assert(write(slave_fd, test_str, len) == len);
  for (int i = 0; i < len; i++) {
    assert(read(master_fd, &c, 1) == 1);
    assert(c == test_str[i]);
  }

  close(slave_fd);

  /* Slave device isn't opened: master reads should report EOF. */
  assert(read(master_fd, &c, 1) == 0);

  /* Write something to the master, and then open the slave.
   * The slave should see the data written. */
  assert(write(master_fd, test_str, len) == len);
  slave_fd = open(ptsname(master_fd), O_NOCTTY | O_RDWR);
  for (int i = 0; i < len; i++) {
    assert(read(slave_fd, &c, 1) == 1);
    assert(c == test_str[i]);
  }

  /* Close the master device: reads on the slave device should report EOF, and
   * writes should return an ENXIO error. */
  close(master_fd);
  assert(read(slave_fd, &c, 1) == 0);
  assert(write(slave_fd, test_str, len) == -1);
  assert(errno == ENXIO);

  close(slave_fd);

  return 0;
}
