#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define HIDKBD_NKEYCODES 6

struct report {
  uint8_t modifier_keys;
  uint8_t reserved;
  uint8_t keycodes[HIDKBD_NKEYCODES];
} __packed;

typedef struct report report_t;

static void print_report(report_t *r) {
  printf("%02hhx", r->modifier_keys);
  for (int i = 0; i < HIDKBD_NKEYCODES; i++)
    printf(" %02hhx", r->keycodes[i]);
  printf("\n");
  fflush(stdout);
}

int main(int argc, char **argv) {
  printf("HID keyboard test:\n");
  fflush(stdout);

  int fd = open("/dev/hidkbd", O_RDONLY, 0);

  while (1) {
    report_t r;
    if (read(fd, &r, sizeof(report_t)) != sizeof(report_t))
      return EXIT_FAILURE;
    print_report(&r);
  }

  return EXIT_SUCCESS;
}
