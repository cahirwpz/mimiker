#include <ringbuf.h>
#include <uio.h>

#if 0
/* This should increase cache hit rate */
if (buf->count == 0) {
  buf->tail = 0;
  buf->head = 0;
}
#endif

void ringbuf_reset(ringbuf_t *buf) {
  buf->head = 0;
  buf->tail = 0;
  buf->count = 0;
}

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

int ringbuf_read(ringbuf_t *buf, uio_t *uio) {
  /* repeat when used space is split into two parts */
  while (uio->uio_resid > 0 && buf->count > 0) {
    /* used space is either [tail, head) or [tail, size) */
    size_t size =
      (buf->tail < buf->head) ? buf->head - buf->tail : buf->size - buf->tail;
    if (size > uio->uio_resid)
      size = uio->uio_resid;
    int res = uiomove((char *)buf->data + buf->tail, size, uio);
    if (res)
      return res;
    buf->tail += size;
    buf->count += size;
    if (buf->tail >= buf->size)
      buf->tail = 0;
  }
  return 0;
}

int ringbuf_write(ringbuf_t *buf, uio_t *uio) {
  /* repeat when free space is split into two parts */
  while (uio->uio_resid > 0 && buf->count < buf->size) {
    /* free space is either [head, tail) or [head, size) */
    size_t size =
      (buf->head < buf->tail) ? buf->tail - buf->head : buf->size - buf->head;
    if (size > uio->uio_resid)
      size = uio->uio_resid;
    int res = uiomove((char *)buf->data + buf->head, size, uio);
    if (res)
      return res;
    buf->head += size;
    buf->count -= size;
    if (buf->head >= buf->size)
      buf->head = 0;
  }

  return 0;
}
