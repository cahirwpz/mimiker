#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/bus.h>
#include <sys/devclass.h>
#include <sys/rman.h>
#include <aarch64/bcm2835reg.h>

#define UART0_BASE BCM2835_PERIPHERALS_BUS_TO_PHYS(BCM2835_UART0_BASE)

typedef struct pl011_state {
  resource_t *regs;
} pl011_state_t;

static int pl011_probe(device_t *dev) {
  /* (pj) so far we don't have better way to associate driver with device for
   * buses which do not automatically enumerate their children. */
  return (dev->unit == 1);
}

static int pl011_attach(device_t *dev) {
  pl011_state_t *state = dev->state;

  resource_t *r = bus_alloc_resource(dev, RT_MEMORY, 0, UART0_BASE,
                                     UART0_BASE + BCM2835_UART0_SIZE - 1,
                                     BCM2835_UART0_SIZE, 0);

  /* (pj) BCM2835_UART0_SIZE is much smaller than PAGESIZE */
  bus_space_map(r->r_bus_tag, r->r_start, PAGESIZE, &r->r_bus_handle);

  assert(r != NULL);

  state->regs = r;

  return 1;
}

/* clang-format off */
static driver_t pl011_driver = {
  .desc = "PL011 UART driver",
  .size = sizeof(pl011_state_t),
  .attach = pl011_attach,
  .probe = pl011_probe,
};
/* clang-format on */

DEVCLASS_ENTRY(root, pl011_driver);
