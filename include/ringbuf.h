#ifndef _SYS_RINGBUF_H_
#define _SYS_RINGBUF_H_

#include <common.h>

typedef struct uio uio_t;
typedef struct kmem_pool kmem_pool_t;

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

ringbuf_t ringbuf_alloc(kmem_pool_t *pool, size_t size);
void ringbuf_destroy(kmem_pool_t *pool, ringbuf_t buf);
void ringbuf_reset(ringbuf_t *buf);
bool ringbuf_putb(ringbuf_t *buf, uint8_t byte);
bool ringbuf_getb(ringbuf_t *buf, uint8_t *byte_p);
int ringbuf_read(ringbuf_t *buf, uio_t *uio);
int ringbuf_write(ringbuf_t *buf, uio_t *uio);
#endif
