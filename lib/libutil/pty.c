#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/termios.h>
#include <sys/types.h>
#include <sys/ioctl.h>

int openpty(int *amaster, int *aslave, char *name, struct termios *term,
            struct winsize *winp) {
  int master, slave;
  char name_buf[PATH_MAX];

  _DIAGASSERT(amaster != NULL);
  _DIAGASSERT(aslave != NULL);
  /* name, term and winp may be NULL */

  if ((master = posix_openpt(O_RDWR | O_NOCTTY)) >= 0) {
    if (ptsname_r(master, name_buf, sizeof(name_buf)) == 0) {
      if ((slave = open(name_buf, O_RDWR | O_NOCTTY)) >= 0) {
        *amaster = master;
        *aslave = slave;
        if (name)
          strcpy(name, name_buf);
        if (term)
          tcsetattr(slave, TCSAFLUSH, term);
        if (winp)
          ioctl(slave, TIOCSWINSZ, winp);
        return 0;
      }
    }
    close(master);
  }
  errno = ENOENT; /* out of ptys */
  return -1;
}
