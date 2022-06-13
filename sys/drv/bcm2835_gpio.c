#define KL_LOG KL_DEV

#include <sys/klog.h>
#include <sys/rman.h>
#include <sys/bus.h>
#include <dev/bcm2835reg.h>
#include <dev/bcm2835_gpio.h>
#include <sys/fdt.h>
#include <sys/errno.h>

/*
 * \brief delay function
 * \param delay number of cycles to delay
 *
 * This just loops <delay> times in a way that the compiler
 * won't optimize away.
 *
 * clang-format is off because of different clang-format versions on build
 * server and CI
 */
/* clang-format off */
static void delay(int64_t count) {
  __asm__ volatile("1: subs %[count], %[count], #1; bne 1b"
                   : [ count ] "+r"(count));
}

#define NGPIO_PIN 54
#define FDT_PINS_INVAL (-1)

typedef uint32_t gpio_function_t;
typedef uint32_t gpio_pull_t;

#define BCM2835_PULL_NONE 0
#define BCM2835_PULL_DOWN 1
#define BCM2835_PULL_UP 2

/* clang-format on */

typedef struct bcm2835_gpio {
  resource_t *gpio;
} bcm2835_gpio_t;

void bcm2835_gpio_function_select(resource_t *r, unsigned pin,
                                  bcm2835_gpio_func_t func) {
  assert(pin < NGPIO_PIN);

  unsigned mask = (1 << BCM2835_GPIO_GPFSEL_BITS_PER_PIN) - 1;
  unsigned reg = pin / BCM2835_GPIO_GPFSEL_PINS_PER_REGISTER;
  unsigned shift = (pin % BCM2835_GPIO_GPFSEL_PINS_PER_REGISTER) *
                   BCM2835_GPIO_GPFSEL_BITS_PER_PIN;

  unsigned val = bus_read_4(r, BCM2835_GPIO_GPFSEL(reg));
  val &= ~(mask << shift);
  val |= (func << shift);
  bus_write_4(r, BCM2835_GPIO_GPFSEL(reg), val);
}

void bcm2835_gpio_set_pull(resource_t *r, unsigned pin,
                           bcm2838_gpio_gppud_t pud) {
  assert(pin < NGPIO_PIN);

  unsigned mask = 1 << (pin % BCM2835_GPIO_GPPUD_PINS_PER_REGISTER);
  unsigned reg = pin / BCM2835_GPIO_GPPUD_PINS_PER_REGISTER;

  bus_write_4(r, BCM2835_GPIO_GPPUD, pud);
  /* Wait 150 cycles – this provides the required set-up time for the control
   * signal. */
  delay(150);
  bus_write_4(r, BCM2835_GPIO_GPPUDCLK(reg), mask);
  /* Wait 150 cycles – this provides the required hold time for the control
   * signal. */
  delay(150);
  bus_write_4(r, BCM2835_GPIO_GPPUD, BCM2838_GPIO_GPPUD_PULLOFF);
  bus_write_4(r, BCM2835_GPIO_GPPUDCLK(reg), 0);
}

void bcm2835_gpio_set_high_detect(resource_t *r, unsigned pin, bool enable) {
  assert(pin < NGPIO_PIN);

  unsigned mask = 1 << (pin % BCM2835_GPIO_GPHEN_PINS_PER_REGISTER);
  unsigned reg = pin / BCM2835_GPIO_GPHEN_PINS_PER_REGISTER;

  uint32_t val = bus_read_4(r, BCM2835_GPIO_GPHEN(reg));
  if (enable)
    val |= mask;
  else
    val &= ~mask;
  bus_write_4(r, BCM2835_GPIO_GPHEN(reg), val);
}

int bcm2835_gpio_configure_pin(device_t *dev, uint32_t pin, uint32_t fsel,
                               uint32_t pull, uint32_t intr_detect) {
  bcm2835_gpio_t *gpio = (bcm2835_gpio_t *)dev->state;

  bcm2835_gpio_function_select(gpio->gpio, pin, (bcm2835_gpio_func_t)fsel);
  bcm2835_gpio_set_pull(gpio->gpio, pin, (bcm2838_gpio_gppud_t)pull);
  bcm2835_gpio_set_high_detect(gpio->gpio, pin, intr_detect);

  return 0;
}

int bcm2835_gpio_read_fdt_entry(device_t *dev, phandle_t node) {
  int result = 0;
  uint32_t *pin_cfgs, *function_cfgs, *pull_cfgs, *intr_detect_cfgs;

  ssize_t pin_cnt = FDT_getprop_alloc_multi(node, "pins", sizeof(*pin_cfgs),
                                            (void **)&pin_cfgs);
  ssize_t function_cnt = FDT_getprop_alloc_multi(
    node, "function", sizeof(*function_cfgs), (void **)&function_cfgs);
  ssize_t pull_cnt = FDT_getprop_alloc_multi(node, "pull", sizeof(*pull_cfgs),
                                             (void **)&pull_cfgs);
  ssize_t intr_detect_cnt = FDT_getprop_alloc_multi(
    node, "intr_detect", sizeof(*intr_detect_cfgs), (void **)&intr_detect_cfgs);

  if (pin_cnt == FDT_PINS_INVAL) {
    klog("Warning: GPIO FDT entry with no pins property");
    result = EINVAL;
    goto bcm2835_gpio_read_fdt_entry_cleanup;
  }

  for (ssize_t i = 0; i < pin_cnt; i++) {
    uint32_t pin = pin_cfgs[i];
    uint32_t fsel =
      i < function_cnt ? function_cfgs[i] : function_cfgs[function_cnt - 1];
    uint32_t pull = i < pull_cnt ? pull_cfgs[i] : pull_cfgs[pull_cnt - 1];
    uint32_t intr_detect = i < intr_detect_cnt
                             ? intr_detect_cfgs[i]
                             : intr_detect_cfgs[intr_detect_cnt - 1];

    bcm2835_gpio_configure_pin(dev, pin, fsel, pull, intr_detect);
  }

bcm2835_gpio_read_fdt_entry_cleanup:
  if (pin_cnt > 0)
    FDT_free(pin_cfgs);
  if (function_cnt > 0)
    FDT_free(function_cfgs);
  if (pull_cnt > 0)
    FDT_free(pull_cfgs);

  return result;
}

int bcm2835_gpio_probe(device_t *dev) {
  return FDT_is_compatible(dev->node, "bcm2835,rpi3-gpio");
}

int bcm2835_gpio_attach(device_t *dev) {
  int err;
  bcm2835_gpio_t *gpio = (bcm2835_gpio_t *)dev->state;

  gpio->gpio = device_take_memory(dev, 0, RF_ACTIVE);

  /* Read configuration from FDT */
  for (phandle_t node = FDT_child(dev->node); node != FDT_NODEV;
       node = FDT_peer(node)) {
    if ((err = bcm2835_gpio_read_fdt_entry(dev, node)))
      return err;
  }

  return 0;
}

driver_t gpio_driver = {.desc = "BCM2835 RPi3 GPIO driver",
                        .size = sizeof(bcm2835_gpio_t),
                        .pass = FIRST_PASS,
                        .probe = bcm2835_gpio_probe,
                        .attach = bcm2835_gpio_attach,
                        .interfaces = {}};
