#ifndef _SYS_RINGBUF_H_
#define _SYS_RINGBUF_H_

#include <sys/cdefs.h>

typedef struct uio uio_t;

typedef struct ringbuf {
  size_t head;   /*!< producing data moves head forward */
  size_t tail;   /*!< consuming data moves tail forward */
  size_t count;  /*!< number of bytes currently stored in the buffer */
  size_t size;   /*!< total size of the buffer */
  uint8_t *data; /*!< buffer that stores data */
} ringbuf_t;

static inline bool ringbuf_empty(ringbuf_t *buf) {
  return buf->count == 0;
}

static inline bool ringbuf_full(ringbuf_t *buf) {
  return buf->count == buf->size;
}

void ringbuf_init(ringbuf_t *rb, void *buf, size_t size);
bool ringbuf_putb(ringbuf_t *buf, uint8_t byte);
bool ringbuf_getb(ringbuf_t *buf, uint8_t *byte_p);
int ringbuf_read(ringbuf_t *buf, uio_t *uio);
int ringbuf_write(ringbuf_t *buf, uio_t *uio);
void ringbuf_reset(ringbuf_t *buf);

#endif /* !_SYS_RINGBUF_H_ */
