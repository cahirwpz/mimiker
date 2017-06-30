#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*A simple hash function stolen from somewhere...*/
uint32_t jenkins_one_at_a_time_hash(char *key, size_t len) {
  uint32_t hash, i;
  for (hash = i = 0; i < len; ++i) {
    hash += key[i];
    hash += (hash << 10);
    hash ^= (hash >> 6);
  }
  hash += (hash << 3);
  hash ^= (hash >> 11);
  hash += (hash << 15);
  return hash;
}

/*...and its single-step version.*/
uint32_t jenkins_one_step(uint32_t hash, char key) {

  hash += key;
  hash += (hash << 10);
  hash ^= (hash >> 6);

  return hash;
}

uint32_t jenkins_final(uint32_t hash) {

  hash += (hash << 3);
  hash ^= (hash >> 11);
  hash += (hash << 15);

  return hash;
}

/* Returns a hash value of its arguments.*/
int main(int argc, char **argv) {

  size_t i;
  uint32_t hash = 0;

  assert(argv != NULL);
  for (i = 0; i < argc; i++) {

    // assert(argv[i] != NULL);

    for (int j = 0; j < strlen(argv[i]); j++)
      hash = jenkins_one_step(hash, argv[i][j]);
  }

  return jenkins_final(hash) & 255;
}
