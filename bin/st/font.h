#ifndef FONT_H
#define FONT_H

typedef struct {
    char *name;
    int firstchar, numchars;
    int fontwidth, fontheight, stride;
    int encoding, bitorder, byteorder;
    void *data;
} font_t;

font_t *font_load(const char *path);
char *font_glyph(font_t *font, int c);

#endif
