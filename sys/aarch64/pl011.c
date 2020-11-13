#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/bus.h>
#include <sys/devclass.h>
#include <sys/rman.h>
#include <aarch64/bcm2835reg.h>
#include <aarch64/bcm2835_gpioreg.h>
#include <aarch64/plcomreg.h>
#include <aarch64/gpio.h>

#define GPIO_BASE BCM2835_PERIPHERALS_BUS_TO_PHYS(BCM2835_GPIO_BASE)
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

  /* Disable UART0. */
  bus_space_write_4(r->r_bus_tag, r->r_bus_handle, PL011COM_CR, 0);
  /* Clear pending interrupts. */
  bus_space_write_4(r->r_bus_tag, r->r_bus_handle, PL011COM_ICR,
                    PL011_INT_ALLMASK);

  /* TODO(pj) do magic with mail buffer */

  gpio_function_select(r, 14, BCM2835_GPIO_ALT0);
  gpio_function_select(r, 15, BCM2835_GPIO_ALT0);
  gpio_set_pull(r, 14, BCM2838_GPIO_GPPUD_PULLOFF);
  gpio_set_pull(r, 15, BCM2838_GPIO_GPPUD_PULLOFF);

  /*
   * Set integer & fractional part of baud rate.
   * Divider = UART_CLOCK/(16 * Baud)
   * Fraction part register = (Fractional part * 64) + 0.5
   * UART_CLOCK = 3000000; Baud = 115200.
   *
   * Divider = 4000000/(16 * 115200) = 2.17 = ~2.
   * Fractional part register = (.17 * 64) + 0.5 = 11.3 = ~11 = 0xb.
   */

  bus_space_write_4(r->r_bus_tag, r->r_bus_handle, PL011COM_IBRD, 2);
  bus_space_write_4(r->r_bus_tag, r->r_bus_handle, PL011COM_FBRD, 0xb);

  /* Enable FIFO & 8 bit data transmission (1 stop bit, no parity). */
  bus_space_write_4(r->r_bus_tag, r->r_bus_handle, PL011COM_LCRH,
                    PL01X_LCR_FEN | PL01X_LCR_8BITS);

  /* Mask all interrupts. */
  bus_space_write_4(r->r_bus_tag, r->r_bus_handle, PL011COM_IMSC,
                    PL011_INT_ALLMASK);

  /* Enable UART0, receive & transfer part of UART. */
  bus_space_write_4(r->r_bus_tag, r->r_bus_handle, PL011COM_CR,
                    PL01X_CR_UARTEN | PL011_CR_TXE | PL011_CR_RXE);

  /* Enable receive interrupt. */
  bus_space_write_4(r->r_bus_tag, r->r_bus_handle, PL011COM_IMSC, PL011_INT_RX);

  /* TODO(pj) register UART0 handler. */

  /* TODO(pj) enable UART0 IRQ. */

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
