#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>

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

void sbrk_test() {
  /* Initial sbrk */
  char *a1 = sbrk(10);
  /* Test write access. */
  memset(a1, 1, 10);
  /* Expand sbrk a little bit. */
  char *a2 = sbrk(40);
  assert(a2 == a1 + 10);
  /* And again. */
  char *a3 = sbrk(50);
  assert(a3 == a2 + 40);
  /* Now expand it a lot, much more than page size. */
  char *a4 = sbrk(0x5000);
  assert(a4 == a3 + 50);
  /* Test write access. */
  memset(a4, -1, 0x5000);
  /* See that previous data is unmodified. */
  assert(*(a1 + 5) == 1);
#if 0
  /* Note: sbrk shrinking not yet implemented! */
  /* Now, try shrinking data. */
  sbrk(-1 * (0x5000 + 50 + 40 + 10));
  /* Get new brk end */
  char *a5 = sbrk(0);
  assert(a1 == a5);
#endif
}

int main(int argc, char **argv) {
  if (argc < 2)
    abort();

  if (strcmp(argv[1], "abort_test") == 0)
    assert(0);

  sbrk_test();

  /* Test some libstd functions. They will mostly fail, because many system
     calls are not implemented yet, but at least printf works!*/
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
    marquee(argv[1], o);
    marquee(str, o);
  }

  return 0;
}
