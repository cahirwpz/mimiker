#ifndef _DEV_BCM2835_GPIO_H_
#define _DEV_BCM2835_GPIO_H_

#include <dev/bcm2835_gpioreg.h>

struct resource_t;

/* Select GPIO alt function. For more information look at
 * https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf and
 * https://pinout.xyz */
void bcm2835_gpio_function_select(resource_t *r, unsigned pin,
                                  bcm2835_gpio_func_t func);

void bcm2835_gpio_set_pull(resource_t *r, unsigned pin,
                           bcm2838_gpio_gppud_t pud);

#define GPHEN1 0x0068

#endif /* !_DEV_BCM2835_GPIO_H_ */
