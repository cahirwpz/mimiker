#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main() {
  int x = open("/tests/dup_test_file", O_RDONLY, 0);
  int y = dup(0);
  printf("x = %d, y = %d\n", x, y);
  dup2(x, 0);
  char word[100];
  scanf("%[^\n]s", word);
  printf("%s\n", word);
  dup2(y, 0);
  return 0;
}
