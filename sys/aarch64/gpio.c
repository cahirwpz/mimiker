#include <sys/rman.h>
#include <sys/bus.h>
#include <aarch64/bcm2835reg.h>
#include <aarch64/bcm2835_gpioreg.h>

/* The function select registers are used to define the operation of the
 * general-purpose I/O pins. */
#define GPFSEL(x) ((x) * sizeof(uint32_t))

#define GPFSEL_PINS_PER_REGISTER 10
#define GPFSEL_BITS_PER_PIN 3

/* The output set registers are used to set a GPIO pin. */
#define GPSET(x) (0x1C + (x) * sizeof(uint32_t))

/* The output clear registers are used to clear a GPIO pin. */
#define GPCLR(x) (0x28 + (x) * sizeof(uint32_t))

/* The pin level registers return the actual value of the pin. */
#define GPLEV(x) (0x34 + (x) * sizeof(uint32_t))

/* The event detect status registers are used to record level and edge events
 * on the GPIO pins. */
#define GPEDS(x) (0x40 + (x) * sizeof(uint32_t))

/* The rising edge detect enable registers define the pins for which a rising
 * edge transition sets a bit in the event detect status registers GPEDS. */
#define GPREN(x) (0x4C + (x) * sizeof(uint32_t))

/* The falling edge detect enable registers define the pins for which a
 * falling edge transition sets a bit in the event detect status registers
 * GPEDS. */
#define GPFEN(x) (0x58 + (x) * sizeof(uint32_t))

/* The high level detect enable registers define the pins for which a high
 * level sets a bit in the event detect status register GPEDS. */
#define GPHEN(x) (0x64 + (x) * sizeof(uint32_t))

/* The low level detect enable registers define the pins for which a low level
 * sets a bit in the event detect status register GPEDS. */
#define GPLEN(x) (0x70 + (x) * sizeof(uint32_t))

/* The asynchronous rising edge detect enable registers define the pins for
 * which a asynchronous rising edge transition sets a bit in the event detect
 * status registers GPEDS. */
#define GPAREN(x) (0x7C + (x) * sizeof(uint32_t))

/* The asynchronous falling edge detect enable registers define the pins for
 * which a asynchronous falling edge transition sets a bit in the event detect
 * status registers GPEDS. */
#define GPAFEN(x) (0x88),

/* Controls actuation of pull up/down to ALL GPIO pins. */
#define GPPUD (0x94)

#define GPPUD_PINS_PER_REGISTER 32

/* Controls actuation of pull up/down for specific GPIO pin. */
#define GPPUDCLK(x) (0x98 + (x) * sizeof(uint32_t))

/*
 * \brief delay function
 * \param delay number of cycles to delay
 *
 * This just loops <delay> times in a way that the compiler
 * won't optimize away.
 */
static void delay(int64_t count) {
  __asm__ volatile("1: subs %[count], %[count], #1; bne 1b"
                   : [count] "+r"(count));
}

void gpio_function_select(resource_t *r, unsigned pin, unsigned func) {
  unsigned mask = (1 << GPFSEL_BITS_PER_PIN) - 1;
  unsigned reg = pin / GPFSEL_PINS_PER_REGISTER;
  unsigned shift = (pin % GPFSEL_PINS_PER_REGISTER) * GPFSEL_BITS_PER_PIN;

  unsigned val = bus_read_4(r, GPFSEL(reg));
  val &= ~(mask << shift);
  val |= (func << shift);
  bus_write_4(r, GPFSEL(reg), val);
}

void gpio_set_pull(resource_t *r, unsigned pin, unsigned pud) {
  unsigned mask = 1 << (pin % GPPUD_PINS_PER_REGISTER);
  unsigned reg = pin / GPPUD_PINS_PER_REGISTER;

  bus_write_4(r, GPPUD, pud);
  delay(150);
  bus_write_4(r, GPPUDCLK(reg), mask);
  delay(150);
  bus_write_4(r, GPPUD, BCM2838_GPIO_GPPUD_PULLOFF);
  bus_write_4(r, GPPUDCLK(reg), 0);
}
