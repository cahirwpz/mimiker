#include <malloc.h>
#include <ringbuf.h>

bool ringbuf_putb(ringbuf_t *buf, uint8_t byte) {
  if (buf->count == buf->size)
    return false;
  buf->data[buf->head++] = byte;
  if (buf->head >= buf->size)
    buf->head = 0;
  buf->count++;
  return true;
}

bool ringbuf_getb(ringbuf_t *buf, uint8_t *byte_p) {
  if (buf->count == 0)
    return false;
  *byte_p = buf->data[buf->tail++];
  if (buf->tail >= buf->size)
    buf->tail = 0;
  buf->count--;
  return true;
}
