#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <term.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>

int main(void) {
  int err = 1;
  struct termios old, t;
  FILE *termfile;
  if ((termfile = fopen("/dev/tty", "r+")) == NULL)
    return err;
  int term_fd = fileno(termfile);
  if (setupterm(NULL, term_fd, NULL))
    return err;
  setbuf(termfile, NULL);
  /* Check if we can move the cursor and save/restore cursor position. */
  if (cursor_address == NULL || save_cursor == NULL || restore_cursor == NULL)
    return err;
  if (tcgetattr(term_fd, &old))
    return err;
  t = old;
  cfmakeraw(&t);
  if (tcsetattr(term_fd, TCSAFLUSH, &t))
    return err;
  /* Save current cursor position. */
  fputs(save_cursor, termfile);
  /* Move the cursor to the bottom right corner of the screen. */
  const char *buf = tiparm(cursor_address, 1000, 1000);
  if (!buf)
    goto term_restore;
  fputs(buf, termfile);
  /* Send the DSR (Device Status Report) ANSI escape sequence. */
  fputs("\033[6n", termfile);
  /* Format of response is "^[[rows;colsR". */
  struct winsize sz;
  memset(&sz, 0, sizeof(sz));
  if (fscanf(termfile, "\033[%hu;%huR", &sz.ws_row, &sz.ws_col) != 2)
    goto cursor_restore;
  if (ioctl(term_fd, TIOCSWINSZ, &sz) == 0)
    err = 0;
cursor_restore:
  /* Restore cursor to the saved position. */
  fputs(restore_cursor, termfile);
term_restore:
  tcsetattr(term_fd, TCSAFLUSH, &old);
  return err;
}
