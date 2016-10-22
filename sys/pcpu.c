#include <stdc.h>
#include <pcpu.h>

pcpu_t _pcpu_data[1];

void pcpu_init(void) {
  memset(_pcpu_data, 0, sizeof(_pcpu_data));
}

