#include "font.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <endian.h>

#define DEFWIDTH 8
#define DEFHEIGHT 16

font_t *font_load(const char *path) {
  int ffd, res;
  int len;
  struct stat st;

  if ((ffd = open(path, O_RDONLY, 0)) < 0)
    return NULL;

  font_t *font = malloc(sizeof(font_t));

  font->fontwidth = DEFWIDTH;
  font->fontheight = DEFHEIGHT;
  font->firstchar = 0;
  font->numchars = 256;
  font->stride = (DEFWIDTH + 7) / 8;
  font->name = strdup(path);

  len = font->numchars * font->fontheight * font->stride;
  if (fstat(ffd, &st) == 0) {
    if (len != st.st_size) {
      uint32_t foo = 0;
      char b[65];
      len = st.st_size;
      /* read header */
      res = read(ffd, b, 4);
      if (strncmp(b, "WSFT", 4) != 0)
        goto err;

      res = read(ffd, b, 64);
      b[64] = 0;
      free(font->name);
      font->name = strdup(b);

      res = read(ffd, &foo, 4);
      font->firstchar = le32toh(foo);
      res = read(ffd, &foo, 4);
      font->numchars = le32toh(foo);
      res = read(ffd, &foo, 4);
      font->encoding = le32toh(foo);
      res = read(ffd, &foo, 4);
      font->fontwidth = le32toh(foo);
      res = read(ffd, &foo, 4);
      font->fontheight = le32toh(foo);
      res = read(ffd, &foo, 4);
      font->stride = le32toh(foo);
      res = read(ffd, &foo, 4);
      font->bitorder = le32toh(foo);
      res = read(ffd, &foo, 4);
      font->byteorder = le32toh(foo);
      len = font->numchars * font->fontheight * font->stride;
    }
  }

  font->data = malloc(len);
  res = read(ffd, font->data, len);
  if (res != len) {
    free(font->data);
    goto err;
  }

  close(ffd);
  return font;

err:
  free(font);
  close(ffd);
  return NULL;
}

char *font_glyph(font_t *font, int c) {
  return (char *)font->data +
         (c - font->firstchar) * font->fontheight * font->stride;
}
