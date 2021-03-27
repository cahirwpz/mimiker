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
  /* Open controlling terminal. */
  if ((termfile = fopen("/dev/tty", "r+")) == NULL)
    return err;
  int term_fd = fileno(termfile);
  /* Initialize libterminfo state. */
  if (setupterm(NULL, term_fd, NULL))
    return err;
  setbuf(termfile, NULL);
  /* Check if we can move the cursor and save/restore cursor position. */
  if (cursor_address == NULL || save_cursor == NULL || restore_cursor == NULL)
    return err;
  /* Save previous tty settings. */
  if (tcgetattr(term_fd, &old))
    return err;
  t = old;
  cfmakeraw(&t);
  /* Switch to raw mode. */
  if (tcsetattr(term_fd, TCSAFLUSH, &t))
    return err;
  /* Save current cursor position. */
  fputs(save_cursor, termfile);
  /* Move the cursor to the bottom right corner of the screen.
   * Actually, we're moving to row 1000, column 1000, so we're assuming that
   * the screen has no more than 1000 lines or columns.
   * The top left corner is at row 1, column 1.
   * The second argument is the row number we want to go to,
   * and the third argument is the column number. */
  const char *buf = tiparm(cursor_address, 1000, 1000);
  if (!buf)
    goto term_restore;
  fputs(buf, termfile);
  /* Send the DSR (Device Status Report) ANSI escape sequence. */
  fputs("\033[6n", termfile);
  /* Format of response is "^[[rows;colsR".
   * The row and column numbers are 1-based. */
  struct winsize sz;
  memset(&sz, 0, sizeof(sz));
  if (fscanf(termfile, "\033[%hu;%huR", &sz.ws_row, &sz.ws_col) != 2)
    goto cursor_restore;
  /* Set the deduced window size. */
  if (ioctl(term_fd, TIOCSWINSZ, &sz) == 0)
    err = 0;
cursor_restore:
  /* Restore cursor to the saved position. */
  fputs(restore_cursor, termfile);
term_restore:
  /* Restore old terminal settings. */
  tcsetattr(term_fd, TCSAFLUSH, &old);
  return err;
}
