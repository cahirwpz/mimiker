#include <sys/kasan.h>
#include <sys/mimiker.h>

static void release_oldest_item(quar_t *q) {
  assert(q->q_buf.count > 0);
  void *oldest_item = q->q_buf.items[q->q_buf.tail];

  q->q_free(q->q_pool, oldest_item);
  q->q_buf.count--;
  q->q_buf.tail++;
  if (q->q_buf.tail == KASAN_QUAR_BUFSIZE)
    q->q_buf.tail = 0;
}

void kasan_quar_releaseall(quar_t *q) {
  while (q->q_buf.count > 0)
    release_oldest_item(q);
}

void kasan_quar_additem(quar_t *q, void *ptr) {
  /* Not enough space, release least recently added item */
  if (q->q_buf.count == KASAN_QUAR_BUFSIZE)
    release_oldest_item(q);

  q->q_buf.items[q->q_buf.head] = ptr;
  q->q_buf.count++;
  q->q_buf.head++;
  if (q->q_buf.head == KASAN_QUAR_BUFSIZE)
    q->q_buf.head = 0;
}

void kasan_quar_init(quar_t *q, void *pool, quar_free_t free) {
  q->q_buf.head = 0;
  q->q_buf.tail = 0;
  q->q_buf.count = 0;
  q->q_free = free;
  q->q_pool = pool;
}
