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
  assert(read(master_fd, buf, sizeof(buf)) == 5);
  assert(strncmp(buf, "hello", 5) == 0);

  /* Control characters should be echoed verbatim unless
   * the ECHOCTL flag is set. */
#define CTRL(x) (x & 037)
  assert(write(master_fd, &(char){CTRL('c')}, 1) == 1);
  assert(write(master_fd, "hello", 5) == 5);
  assert(read(master_fd, buf, sizeof(buf)) == 6);
  assert(buf[0] == CTRL('c'));
  assert(strncmp(buf + 1, "hello", 5) == 0);

  t.c_lflag |= ECHOCTL;
  assert(tcsetattr(slave_fd, TCSANOW, &t) == 0);

  assert(write(master_fd, &(char){CTRL('c')}, 1) == 1);
  assert(write(master_fd, "hello", 5) == 5);
  assert(read(master_fd, buf, sizeof(buf)) == 7);
  assert(strncmp(buf, "^Chello", 7) == 0);

  /* Turn off ECHO, but turn on ECHONL. Only newlines should be echoed. */
  t.c_lflag &= ~(ECHO | ECHOCTL);
  t.c_lflag |= ECHONL;
  assert(tcsetattr(slave_fd, TCSANOW, &t) == 0);

  assert(write(master_fd, "hello\n", 6) == 6);
  assert(read(master_fd, buf, sizeof(buf)) == 1);
  assert(buf[0] == '\n');

  return 0;
}

int test_tty_signals(void) {
  signal_setup(SIGUSR1);
  int master_fd, slave_fd;
  open_pty(&master_fd, &slave_fd);

  /* Set raw mode and ISIG flag. */
  struct termios t;
  assert(tcgetattr(slave_fd, &t) == 0);
  cfmakeraw(&t);
  t.c_lflag |= ISIG;
  assert(tcsetattr(slave_fd, TCSANOW, &t) == 0);

  pid_t ppid = getpid();
  pid_t cpid = fork();
  if (cpid == 0) {
    signal_setup(SIGINT);
    signal_setup(SIGQUIT);
    signal_setup(SIGTSTP);

    cpid = getpid();
    /* The child creates a new session and opens the slave tty, so that it
     * becomes the controlling terminal for its session, and the child is in the
     * foreground process group. */
    assert(setsid() == cpid);
    assert(close(slave_fd) == 0);
    assert((slave_fd = open(ptsname(master_fd), 0)) >= 0);
    assert(close(master_fd) == 0);
    /* We should be in the foreground process group now. */
    assert(tcgetpgrp(slave_fd) == cpid);

    /* We're ready to take the signals now. */
    kill(ppid, SIGUSR1);
    wait_for_signal(SIGINT);

    /* Check if the "foo" was discarded. */
    char c;
    assert(read(slave_fd, &c, 1) == 1);
    assert(c == 'x');

    kill(ppid, SIGUSR1);
    wait_for_signal(SIGQUIT);

    kill(ppid, SIGUSR1);
    wait_for_signal(SIGTSTP);

    return 0;
  }

  assert(write(master_fd, "foo", 3) == 3);

  /* Wait until the child is ready. */
  wait_for_signal(SIGUSR1);
  unsigned char vintr = t.c_cc[VINTR];
  assert(vintr != _POSIX_VDISABLE);
  assert(write(master_fd, &vintr, 1) == 1);

  /* The "foo" should be discarded in response to receiving VINTR,
   * so the child should only be able to read the following "x". */
  assert(write(master_fd, &(char){'x'}, 1) == 1);

  wait_for_signal(SIGUSR1);
  unsigned char vquit = t.c_cc[VQUIT];
  assert(vquit != _POSIX_VDISABLE);
  assert(write(master_fd, &vquit, 1) == 1);

  wait_for_signal(SIGUSR1);
  unsigned char vsusp = t.c_cc[VSUSP];
  assert(vsusp != _POSIX_VDISABLE);
  assert(write(master_fd, &vsusp, 1) == 1);

  wait_for_child_exit(cpid, 0);

  return 0;
}
