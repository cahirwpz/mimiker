#include <stdc.h>

int main(int argc, char **argv) {
  for (int i = 0; i < argc; ++i) {
    kprintf("arg %d: >%s<\n", i, argv[i]);
  }
  return 0;
}
