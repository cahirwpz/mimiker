/* Taken from NetBSD: sys/lib/libkern/copystr.c */

#include <sys/libkern.h>
#include <sys/errno.h>
#include <sys/mimiker.h>

/*
 * int copystr(const void *kfaddr, void *kdaddr, size_t len, size_t *done)
 *
 * Copy a NIL-terminated string, at most maxlen characters long.  Return the
 * number of characters copied (including the NIL) in *done.  If the
 * string is too long, return ENAMETOOLONG; else return 0.
 */
int copystr(const void *kfaddr, void *kdaddr, size_t len, size_t *done) {
  const char *src = kfaddr;
  char *dst = kdaddr;
  size_t i;

  for (i = 0; i < len; i++) {
    if ((*dst++ = *src++) == '\0') {
      if (done)
        *done = i + 1;
      return 0;
    }
  }

  if (done)
    *done = i;

  return ENAMETOOLONG;
}
