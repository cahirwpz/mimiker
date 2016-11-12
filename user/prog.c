#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define TEXTAREA_SIZE 100
// This should land in .bss, accessed by a pointer in .data
char textarea[TEXTAREA_SIZE];
/* The string wil land in .rodata, but str will be stored in .sdata! */
char *str = "A hard-coded string.";

// Copy warped string with changing offsets
void marquee(const char *string, int offset) {
  size_t n = strlen(string);
  for (int i = 0; i < TEXTAREA_SIZE - 1; i++) {
    textarea[i] = string[(i + offset) % n];
  }
  textarea[TEXTAREA_SIZE - 1] = 0;
}

int main(int argc, char **argv) {
  /* TODO: Actually, the 0-th argument should be the program name. */
  if (argc < 1)
    abort();

  /* Test some libstd functions. They will all fail, because system calls are
     not hooked up yet, but this should at least compile and link
     successfully. */
  printf("Hello libc!\n");

  int *ptr = malloc(10 * sizeof(int));
  free(ptr);

  FILE *f = fopen("/etc/passwd", "rw");
  fclose(f);

  printf("Test complete.\n");

  uint32_t o = 0;
  while (1) {
    o++;
    /* Test both the passed argument, and data accessed with $gp */
    marquee(argv[0], o);
    marquee(str, o);
  }

  return 0;
}
