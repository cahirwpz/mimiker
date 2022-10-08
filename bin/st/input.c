#include "input.h"

#include <sys/types.h>
#include <sys/input.h>
#include <wchar.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "kbio.h"
#include "kdbtable.h"

#include "st.h"

struct {
  int fd;
} input;

static void handle_event(struct input_event *ie) {
  char buf[64];

  if (ie->type != EV_KEY)
    return;

  return;
  if (ie->value != 0) {
    buf[0] = key_map.key[ie->code].map[0];
    buf[1] = 0;
    ttywrite(buf, 1, 1);
  }
}

int input_init(const char *ev_dev) {
  if ((input.fd = open(ev_dev, O_RDONLY)) < 0)
    return 0;
  return 1;
}

void input_read(void) {
  struct input_event ie;
  int ret;

  if ((ret = read(input.fd, &ie, sizeof(struct input_event))) > 0)
    handle_event(&ie);
}

int input_getfd(void) {
  return input.fd;
}