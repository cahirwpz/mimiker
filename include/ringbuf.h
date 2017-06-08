#ifndef _SYS_RINGBUF_H_
#define _SYS_RINGBUF_H_

#include <common.h>

typedef struct ringbuf {
  size_t head, tail;
  size_t count, size;
  uint8_t *data;
} ringbuf_t;

bool ringbuf_putb(ringbuf_t *buf, uint8_t byte);
bool ringbuf_getb(ringbuf_t *buf, uint8_t *byte_p);

#endif
