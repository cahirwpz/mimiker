#include <ringbuf.h>
#include <ktest.h>
#include <malloc.h>
#include <uio.h>
#include <vm_map.h>

MALLOC_DEFINE(M_TEST, "test", 4, 8);

static void ringbuf_trivial(void) {

  ringbuf_t rbt = ringbuf_alloc(M_TEST, 1);
  uint8_t c = 'c';

  assert(ringbuf_empty(&rbt));
  assert(ringbuf_putb(&rbt, c) == true);
  assert(ringbuf_full(&rbt));
  assert(ringbuf_putb(&rbt, c) == false);

  uint8_t result;

  assert(ringbuf_getb(&rbt, &result) == true);
  assert(result == c);
  assert(ringbuf_empty(&rbt));

  ringbuf_destroy(M_TEST, rbt);
}

static void put_succeeds(ringbuf_t *rb, uint8_t byte) {
  assert(ringbuf_putb(rb, byte) == true);
  assert(!ringbuf_empty(rb));
}

static void put_fails(ringbuf_t *rb, uint8_t byte) {
  assert(ringbuf_putb(rb, byte) == false);
  assert(!ringbuf_empty(rb));
  assert(ringbuf_full(rb));
}

static void get_succeeds(ringbuf_t *rb, uint8_t byte) {
  uint8_t result;

  assert(!ringbuf_empty(rb));
  assert(ringbuf_getb(rb, &result) == true);
  assert(result == byte);
  assert(!ringbuf_full(rb));
}

static void ringbuf_nontrivial(void) {
  ringbuf_t rbt = ringbuf_alloc(M_TEST, 5);
  uint8_t c = 'c', d = 'd', e = 'e', f = 'f', g = 'g';

  assert(ringbuf_empty(&rbt));

  put_succeeds(&rbt, c);
  put_succeeds(&rbt, d);
  put_succeeds(&rbt, e);
  put_succeeds(&rbt, f);
  put_succeeds(&rbt, g);
  assert(ringbuf_full(&rbt));

  put_fails(&rbt, d);

  get_succeeds(&rbt, c);
  put_succeeds(&rbt, g);

  get_succeeds(&rbt, d);
  put_succeeds(&rbt, f);

  get_succeeds(&rbt, e);
  put_succeeds(&rbt, e);

  get_succeeds(&rbt, f);
  put_succeeds(&rbt, d);

  get_succeeds(&rbt, g);
  put_succeeds(&rbt, c);

  put_fails(&rbt, d);

  get_succeeds(&rbt, g);
  get_succeeds(&rbt, f);
  get_succeeds(&rbt, e);
  get_succeeds(&rbt, d);
  get_succeeds(&rbt, c);
  assert(ringbuf_empty(&rbt));

  ringbuf_destroy(M_TEST, rbt);
}

static void uio_ringbuf_trivial(void) {

  ringbuf_t rbt = ringbuf_alloc(M_TEST, 1);

  uint8_t src[] = "c";
  uint8_t dst[] = " ";

  uio_t uio_src = UIO_SINGLE_KERNEL(UIO_WRITE, 0, src, 1);
  uio_t uio_dst = UIO_SINGLE_KERNEL(UIO_READ, 0, dst, 1);

  assert(ringbuf_write(&rbt, &uio_src) == 0);
  assert(ringbuf_full(&rbt));

  assert(ringbuf_read(&rbt, &uio_dst) == 0);
  assert(ringbuf_empty(&rbt));

  assert(dst[0] == 'c');
  ringbuf_destroy(M_TEST, rbt);
}

static void uio_ringbuf_one_transfer(void) {

  ringbuf_t rbt = ringbuf_alloc(M_TEST, 5);

  uint8_t src[] = "cdefg";
  uint8_t dst[] = "     ";

  uio_t uio_src = UIO_SINGLE_KERNEL(UIO_WRITE, 0, src, 5);
  uio_t uio_dst = UIO_SINGLE_KERNEL(UIO_READ, 0, dst, 5);

  assert(ringbuf_write(&rbt, &uio_src) == 0);
  assert(ringbuf_full(&rbt));

  assert(ringbuf_read(&rbt, &uio_dst) == 0);
  assert(ringbuf_empty(&rbt));

  assert(dst[0] == 'c');
  assert(dst[1] == 'd');
  assert(dst[2] == 'e');
  assert(dst[3] == 'f');
  assert(dst[4] == 'g');

  ringbuf_destroy(M_TEST, rbt);
}

static void uio_ringbuf_two_transfers(void) {

  ringbuf_t rbt = ringbuf_alloc(M_TEST, 5);
  uint8_t src[] = "cdefg";
  uint8_t dst[] = "     ";

  uio_t uio_src = UIO_SINGLE_KERNEL(UIO_WRITE, 0, src, 2);
  assert(ringbuf_write(&rbt, &uio_src) == 0);
  uio_src = UIO_SINGLE_KERNEL(UIO_WRITE, 0, src + 2, 3);
  assert(ringbuf_write(&rbt, &uio_src) == 0);

  uio_t uio_dst = UIO_SINGLE_KERNEL(UIO_READ, 0, dst, 3);
  assert(ringbuf_read(&rbt, &uio_dst) == 0);
  uio_dst = UIO_SINGLE_KERNEL(UIO_READ, 0, dst + 3, 2);
  assert(ringbuf_read(&rbt, &uio_dst) == 0);

  assert(dst[0] == 'c');
  assert(dst[1] == 'd');
  assert(dst[2] == 'e');
  assert(dst[3] == 'f');
  assert(dst[4] == 'g');

  ringbuf_destroy(M_TEST, rbt);
}

static void uio_ringbuf_cyclic_transfers(void) {

  ringbuf_t rbt = ringbuf_alloc(M_TEST, 5);
  uint8_t src[] = "cdef";
  uint8_t dst[] = "     ";

  uio_t uio_src = UIO_SINGLE_KERNEL(UIO_WRITE, 0, src, 4);
  assert(ringbuf_write(&rbt, &uio_src) == 0);
  uio_t uio_dst = UIO_SINGLE_KERNEL(UIO_READ, 0, dst, 2);
  assert(ringbuf_read(&rbt, &uio_dst) == 0);

  assert(dst[0] == 'c');
  assert(dst[1] == 'd');
  assert(dst[2] == ' ');
  assert(dst[3] == ' ');
  assert(dst[4] == ' ');

  uint8_t src1[] = "abc";
  uio_t uio_src1 = UIO_SINGLE_KERNEL(UIO_WRITE, 0, src1, 3);
  assert(ringbuf_write(&rbt, &uio_src1) == 0);

  uio_dst = UIO_SINGLE_KERNEL(UIO_READ, 0, dst, 5);
  assert(ringbuf_read(&rbt, &uio_dst) == 0);

  assert(dst[0] == 'e');
  assert(dst[1] == 'f');
  assert(dst[2] == 'a');
  assert(dst[3] == 'b');
  assert(dst[4] == 'c');

  ringbuf_destroy(M_TEST, rbt);
}

static int test_ringbuf(void) {
  ringbuf_trivial();
  ringbuf_nontrivial();
  uio_ringbuf_trivial();
  uio_ringbuf_one_transfer();
  uio_ringbuf_two_transfers();
  uio_ringbuf_cyclic_transfers();

  return KTEST_SUCCESS;
}

KTEST_ADD(ringbuf, test_ringbuf, 0);
