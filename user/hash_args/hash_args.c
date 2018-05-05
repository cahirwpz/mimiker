#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/hash.h>

/* Returns a hash value of its arguments.*/
int main(int argc, char **argv) {

  int i;
  uint32_t hash = HASH32_STR_INIT;

  assert(argv != NULL);
  for (i = 0; i < argc; i++) {

    assert(argv[i] != NULL);

    hash = hash32_str(argv[i], hash);
  }

  return hash & 255;
}
