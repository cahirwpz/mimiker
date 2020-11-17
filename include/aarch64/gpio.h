#ifndef _GPIO_H_
#define _GPIO_H_

struct resource_t;

void gpio_function_select(resource_t *r, unsigned pin, unsigned func);

void gpio_set_pull(resource_t *r, unsigned pin, unsigned pud);

#endif /* !_GPIO_H_ */
