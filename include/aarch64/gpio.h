#ifndef _GPIO_H_
#define _GPIO_H_

struct resource_t;

/* Select GPIO alt function. For more information look at
 * https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf and
 * https://pinout.xyz */
void gpio_function_select(resource_t *r, unsigned pin, unsigned func);

void gpio_set_pull(resource_t *r, unsigned pin, unsigned pud);

#endif /* !_GPIO_H_ */
