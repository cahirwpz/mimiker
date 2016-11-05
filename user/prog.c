#include <stdint.h>
#include <stdio.h>

#define TEXTAREA_SIZE 100
// This should land in .bss, accessed by a pointer in .data
char textarea[TEXTAREA_SIZE];

void abort() {
  while (1)
    ; /* Sigh. */
}

size_t my_strlen(const char *str) {
  size_t n = 0;
  while (*(str++))
    n++;
  return n;
}

void marquee(const char *string) {
  uint32_t o = 0;
  size_t n = my_strlen(string);
  while (1) {
    o++;
    // Copy string three times with changing offsets
    for (int i = 0; i < TEXTAREA_SIZE - 1; i++) {
      textarea[i] = string[(i + o) % n];
    }
    // Null-terminate
    textarea[TEXTAREA_SIZE - 1] = 0;
  }
}

int main(int argc, char **argv) {
  /* TODO: Actually, the 0-th argument should be the program name. */
  if (argc < 1)
    abort();

  marquee(argv[0]);

  return 0;
}
