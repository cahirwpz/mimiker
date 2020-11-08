#include <sys/mimiker.h>
#include <sys/ktest.h>
#include <sys/rman.h>

#define ADDR_MIN 0
#define ADDR_MAX (~(rman_addr_t)1)

/* The test is implementation independent
 * because the region is private to the rman module. */
static int test_rman_regions(void) {
  rman_t rm;
  rman_init(&rm, "test rman", RT_MEMORY);
  rman_manage_region(&rm, 0x00000000, 0x0000ffff);
  rman_manage_region(&rm, 0x00010000, 0x0001ffff);
  rman_manage_region(&rm, 0x10000000, 0x10000000);
  rman_manage_region(&rm, 0x10100000, 0x101fffff);

  assert(rman_alloc_resource(&rm, ADDR_MIN, ADDR_MAX, ADDR_MAX, 1, 0, NULL) ==
         NULL);
  assert(rman_alloc_resource(&rm, ADDR_MIN, ADDR_MIN, 2, 1, 0, NULL) == NULL);
  assert(rman_alloc_resource(&rm, 0x11000000, ADDR_MAX, 1, 1, 0, NULL) == NULL);

  resource_t *r =
    rman_alloc_resource(&rm, ADDR_MIN, ADDR_MAX, 0x100000, 0x100000, 0, NULL);
  assert(r);
  for (int i = 0; i < 2; i++)
    assert(
      rman_alloc_resource(&rm, ADDR_MIN, ADDR_MAX, 0x10000, 0x10000, 0, NULL));
  assert(rman_alloc_resource(&rm, ADDR_MIN, ADDR_MAX, 0x10000, 0x10000, 0,
                             NULL) == NULL);
  rman_release_resource(r);
  assert(
    rman_alloc_resource(&rm, ADDR_MIN, ADDR_MAX, 0x10000, 0x10000, 0, NULL));
  rman_destroy(&rm);

  return KTEST_SUCCESS;
}

KTEST_ADD(rman_regions, test_rman_regions, 0);
