#include <sys/klog.h>
#include <sys/ringbuf.h>
#include <sys/ktest.h>
#include <sys/uio.h>
#include <sys/vm_map.h>

static int test_ringbuf_trivial(void) {
  ringbuf_t rbt;
  char buf[1];
  ringbuf_init(&rbt, buf, 1);

  uint8_t c = 'c';

  assert(ringbuf_empty(&rbt));
  assert(ringbuf_putb(&rbt, c));
  assert(ringbuf_full(&rbt));
  assert(!ringbuf_putb(&rbt, c));

  uint8_t result;

  assert(ringbuf_getb(&rbt, &result));
  assert(result == c);
  assert(ringbuf_empty(&rbt));

  return KTEST_SUCCESS;
}

static void put_succeeds(ringbuf_t *rb, uint8_t byte) {
  assert(ringbuf_putb(rb, byte));
  assert(!ringbuf_empty(rb));
}

static void put_fails(ringbuf_t *rb, uint8_t byte) {
  assert(!ringbuf_putb(rb, byte));
  assert(!ringbuf_empty(rb));
  assert(ringbuf_full(rb));
}

static void get_succeeds(ringbuf_t *rb, uint8_t byte) {
  uint8_t result;

  assert(!ringbuf_empty(rb));
  assert(ringbuf_getb(rb, &result));
  assert(result == byte);
  assert(!ringbuf_full(rb));
}

static int test_ringbuf_nontrivial(void) {
  ringbuf_t rbt;
  char buf[5];
  ringbuf_init(&rbt, buf, 5);

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

  return KTEST_SUCCESS;
}

static int test_ringbuf_move(void) {
  ringbuf_t src, dst;
  char buf0[5], buf1[5];
  ringbuf_init(&src, buf0, 5);
  ringbuf_init(&dst, buf1, 5);

  uint8_t testbuf[] = "abcde";

  uio_t uio_src = UIO_SINGLE_KERNEL(UIO_WRITE, 0, testbuf, 5);
  assert(ringbuf_write(&src, &uio_src) == 0);

  assert(ringbuf_movenb(&src, &dst, 5) == true);

  assert(buf1[0] == 'a');
  assert(buf1[1] == 'b');
  assert(buf1[2] == 'c');
  assert(buf1[3] == 'd');
  assert(buf1[4] == 'e');

  return KTEST_SUCCESS;
}

static int test_uio_ringbuf_trivial(void) {
  ringbuf_t rbt;
  char buf[5];
  ringbuf_init(&rbt, buf, 1);

  uint8_t src[] = "c";
  uint8_t dst[] = " ";

  uio_t uio_src = UIO_SINGLE_KERNEL(UIO_WRITE, 0, src, 1);
  uio_t uio_dst = UIO_SINGLE_KERNEL(UIO_READ, 0, dst, 1);

  assert(ringbuf_write(&rbt, &uio_src) == 0);
  assert(ringbuf_full(&rbt));

  assert(ringbuf_read(&rbt, &uio_dst) == 0);
  assert(ringbuf_empty(&rbt));

  assert(dst[0] == 'c');

  return KTEST_SUCCESS;
}

static int test_uio_ringbuf_one_transfer(void) {
  ringbuf_t rbt;
  char buf[5];
  ringbuf_init(&rbt, buf, 5);

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

  return KTEST_SUCCESS;
}

static int test_uio_ringbuf_two_transfers(void) {
  ringbuf_t rbt;
  char buf[5];
  ringbuf_init(&rbt, buf, 5);

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

  return KTEST_SUCCESS;
}

static int test_uio_ringbuf_cyclic_transfers(void) {
  ringbuf_t rbt;
  char buf[5];
  ringbuf_init(&rbt, buf, 5);

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

  return KTEST_SUCCESS;
}

KTEST_ADD(ringbuf_trivial, test_ringbuf_trivial, 0);
KTEST_ADD(ringbuf_nontrivial, test_ringbuf_nontrivial, 0);
KTEST_ADD(ringbuf_move, test_ringbuf_move, 0);
KTEST_ADD(uio_ringbuf_trivial, test_uio_ringbuf_trivial, 0);
KTEST_ADD(uio_ringbuf_one_transfer, test_uio_ringbuf_one_transfer, 0);
KTEST_ADD(uio_ringbuf_two_transfers, test_uio_ringbuf_two_transfers, 0);
KTEST_ADD(uio_ringbuf_cyclic_transfers, test_uio_ringbuf_cyclic_transfers, 0);
