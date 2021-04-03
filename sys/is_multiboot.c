#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUF_SIZE 4096

static const char multiboot_magic[] = {
  0xd6, 0x50, 0x52, 0xe8
};

static bool is_magic(const char *ptr) {
  for (int i = 0; i < sizeof(multiboot_magic); i++) {
    if (*ptr++ != multiboot_magic[i])
      return false;
  }
  return true;
}

static ssize_t search_multiboot2_header(int fd) {
  static char buf[BUF_SIZE];
  ssize_t pos = 0;

  ssize_t n;
  while ((n = read(fd, buf, BUF_SIZE))) {
    const char *ptr = buf;

    for (int i = 0; i <= n - sizeof(multiboot_magic); i++, pos++)
      if (is_magic(ptr++))
        return pos;
  }
  printf("end, n = %zd\n", n);

  return -1;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "usage: %s: <filename>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "failed to open %s\n", argv[1]);
    exit(EXIT_FAILURE);
  }

  ssize_t pos = search_multiboot2_header(fd);
  if (pos < 0)
    printf("Couldn't find a multiboot2 header\n");
  else
    printf("Multiboot header found at position %ld\n", (long)pos);

  close(fd);

  exit(EXIT_SUCCESS);
}
