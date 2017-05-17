#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>

int main() {
  int x = open("/tests/dup_test_file", O_RDONLY, 0);
  int y = dup(0);
  dup2(x, 0);
  char word[100];
  scanf("%[^\n]s", word);
  assert(strcmp(word, "Hello, World!") == 0);
  dup2(y, 0);
  return 0;
}
