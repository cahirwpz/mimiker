#include <common.h>
#include <ktest.h>
#include <stdc.h>
#include <clock.h>
#include <mount.h>
#include <uio.h>
#include <vm_map.h>
#include <vnode.h>

/*
 * Delays for at least the given number of milliseconds. May not be
 * nanosecond-accurate.
 */
void mdelay(unsigned msec) {
  realtime_t now = clock_get();
  realtime_t final = now + msec;
  while (final > clock_get())
    ;
}

static int test_rtc() {
  vnode_t *v;
  int error = vfs_lookup("/dev/mc146818", &v);
  assert(!error);

  uio_t uio;
  char buffer[100];
  int year, month, day, hour, minute, second;

  while (1) {
    uio = UIO_SINGLE_KERNEL(UIO_READ, 0, buffer, sizeof(buffer));
    error = VOP_READ(v, &uio);
    assert(!error);

    sscanf(buffer, "%d %d %d %d %d %d", &year, &month, &day, &hour, &minute,
           &second);

    kprintf("Time is %d.%d.%d %02d:%02d:%02d\n", day, month, year, hour, minute,
            second);
    mdelay(1000);
  }
  return KTEST_FAILURE;
}

KTEST_ADD(rtc, test_rtc, KTEST_FLAG_NORETURN);
