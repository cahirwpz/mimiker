#include "bitmap.h"
#include "libkern.h"

void __bitmap_printer_hex(word_t *bf, size_t size) {
  int i;
  for (i = size - 1; i >= 0; i--)
    kprintf("%0" STR(NBBY) "x", bf[i]);
}

void __bitmap_printer_bin(word_t *bf, size_t size) {
  int i, bit;
  for (i = size - 1; i >= 0; i--) {
    word_t curr = bf[i];
    char bin[WORD_SIZE + 1];
    for (bit = 0; bit < WORD_SIZE; bit++) {
      bin[WORD_SIZE - bit - 1] = (curr & 0x1) ? '1' : '0';
      curr >>= 1;
    }
    bin[WORD_SIZE] = 0;
    kprintf("%s", bin);
  }
}
