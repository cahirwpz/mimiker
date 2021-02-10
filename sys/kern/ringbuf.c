#include <sys/klog.h>
#include <sys/ringbuf.h>
#include <sys/uio.h>

void ringbuf_init(ringbuf_t *rb, void *buf, size_t size) {
  rb->head = 0;
  rb->tail = 0;
  rb->count = 0;
  rb->size = size;
  rb->data = buf;
}

static void produce(ringbuf_t *buf, unsigned bytes) {
  assert(buf->count + bytes <= buf->size);
  assert(buf->head + bytes <= buf->size);
  buf->count += bytes;
  buf->head += bytes;
  if (buf->head == buf->size)
    buf->head = 0;
}

static void consume(ringbuf_t *buf, unsigned bytes) {
  assert(buf->count >= bytes);
  assert(buf->tail + bytes <= buf->size);
  buf->count -= bytes;
  buf->tail += bytes;
  if (buf->tail == buf->size)
    buf->tail = 0;
}

bool ringbuf_putb(ringbuf_t *buf, uint8_t byte) {
  if (buf->count == buf->size)
    return false;
  buf->data[buf->head] = byte;
  produce(buf, 1);
  return true;
}

bool ringbuf_putnb(ringbuf_t *buf, uint8_t *data, size_t n) {
  if (buf->count + n > buf->size)
    return false;
  for (size_t i = 0; i < n; i++)
    ringbuf_putb(buf, data[i]);
  return true;
}

bool ringbuf_getb(ringbuf_t *buf, uint8_t *byte_p) {
  if (buf->count == 0)
    return false;
  *byte_p = buf->data[buf->tail];
  consume(buf, 1);
  return true;
}

bool ringbuf_getnb(ringbuf_t *buf, uint8_t *data, size_t n) {
  if (buf->count < n)
    return false;
  for (size_t i = 0; i < n; i++)
    ringbuf_getb(buf, &data[i]);
  return true;
}

int ringbuf_read(ringbuf_t *buf, uio_t *uio) {
  assert(uio->uio_op == UIO_READ);
  /* repeat when used space is split into two parts */
  while (uio->uio_resid > 0 && !ringbuf_empty(buf)) {
    /* used space is either [tail, head) or [tail, size) */
    size_t size =
      (buf->tail < buf->head) ? buf->head - buf->tail : buf->size - buf->tail;
    if (size > uio->uio_resid)
      size = uio->uio_resid;
    int res = uiomove((char *)buf->data + buf->tail, size, uio);
    if (res)
      return res;
    consume(buf, size);
  }
  return 0;
}

int ringbuf_write(ringbuf_t *buf, uio_t *uio) {
  assert(uio->uio_op == UIO_WRITE);
  /* repeat when free space is split into two parts */
  while (uio->uio_resid > 0 && !ringbuf_full(buf)) {
    /* free space is either [head, tail) or [head, size) */
    size_t size =
      (buf->head < buf->tail) ? buf->tail - buf->head : buf->size - buf->head;
    if (size > uio->uio_resid)
      size = uio->uio_resid;
    int res = uiomove((char *)buf->data + buf->head, size, uio);
    if (res)
      return res;
    produce(buf, size);
  }
  return 0;
}

void ringbuf_reset(ringbuf_t *buf) {
  ringbuf_init(buf, buf->data, buf->size);
}
