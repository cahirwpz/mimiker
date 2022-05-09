#ifndef _DEVGPIO_H_
#define _DEV_GPIO_H_

#include <inttypes.h>
#include <sys/mimiker.h>
#include <sys/device.h>

typedef size_t gpio_pin_t;
typedef size_t gpio_word_t;

typedef struct gpio_pinrange {
  gpio_pin_t beg;
  gpio_pin_t end; 
} gpio_pin_range_t;

#define GPIO_PIN_INVALID ((gpio_pin_t)(-1))
#define GPIO_PIN_RANGE_INVALID \
  ((gpio_pinrange_t){GPIO_PIN_INVALID, GPIO_PIN_INVALID})

typedef enum gpio_pull_mode {
  GPIO_PULL_UP = 0x01,
  GPIO_PULL_DOWN = 0x02
} gpio_pull_mode_t;

typedef enum gpio_intr_trig {
  GPIO_INTR_RISING = 0x0001,
  GPIO_INTR_FALLING = 0x0002,
  GPIO_INTR_HIGH = 0x0004,
  GPIO_INTR_LOW = 0x0008
} gpio_intr_trig_t;

typedef enum gpio_intr_mode {
  GPIO_INTR_MODE_SYNC,
  GPIO_INTR_MODE_ASYNC
} gpio_intr_mode_t;

typedef enum gpio_dir {
  GPIO_DIR_IN = 0x01,
  GPIO_DIR_OUT = 0x02,
  GPIO_DIR_SPECIAL = 0x04 /* Used when direction is set via routing config */
} gpio_dir_t;

typedef struct gpio_pin_caps {
  gpio_pull_mode_t sup_pull_modes;     /* Supported pull modes */
  gpio_intr_trig_t sup_intr_triggers;  /* Suppotrted interrupt triggers */
  gpio_intr_mode_t sup_intr_modes;     /* SUpported interrupt modes */
  gpio_dir_t sup_dirs;                 /* Supported directions */
} gpio_pin_caps_t;

typedef int (*gpio_activate_routing_t)(device_t *dev, const char *config_name,
                                       const char **other_config);
typedef int (*gpio_deactivate_routing_t)(device_t *dev,
                                         const char *config_name);
typedef gpio_pin_t (*gpio_get_pin_t)(device_t *dev, const char *name);
typedef int (*gpio_get_pin_caps_t)(device_t *dev, gpio_pin_t pin,
                                   gpio_pin_caps_t *caps);
typedef int (*gpio_set_pin_pull_t)(device_t *dev, gpio_pin_t pin,
                                   gpio_pull_mode_t mode);
typedef int (*gpio_set_pin_intr_t)(device_t *dev, gpio_pin_t pin,
                                   gpio_intr_trig_t trigger,
                                   gpio_intr_mode_t mode);
typedef int (*gpio_set_pin_dir_t)(device_t *dev, gpio_pin_t pin,
                                  gpio_dir_t dir);
typedef gpio_pin_range_t (*gpio_get_pin_range_t)(device_t *dev,
                                                 const char *from,
                                                 const char *to);
typedef size_t (*gpio_max_pin_range_t)(device_t *dev);
typedef int (*gpio_read_t)(device_t *dev, gpio_pin_range_t range,
                           gpio_word_t *word);
typedef int (*gpio_write_t)(device_t *dev, gpio_pin_range_t range,
                            gpio_word_t word);

typedef struct gpio_methods {
  gpio_activate_routing_t activate_routing;
  gpio_deactivate_routing_t deactivate_routing;
  gpio_get_pin_t get_pin;
  gpio_get_pin_caps_t get_pin_caps;
  gpio_set_pin_pull_t set_pin_pull;
  gpio_set_pin_intr_t set_pin_intr;
  gpio_set_pin_dir_t set_pin_dir;
  gpio_get_pin_range_t get_pin_range;
  gpio_max_pin_range_t max_pin_range;
  gpio_read_t read;
  gpio_write_t write;
} gpio_methods_t;


static inline gpio_methods_t *gpio_methods(device_t *dev) {
  return (gpio_methods_t *)dev->driver->interfaces[DIF_GPIO];
}

#define GPIO_METHOD_PROVIDER(dev, method)                                      \
  (device_method_provider((dev), DIF_GPIO,                                     \
                          offsetof(struct gpio_methods, method)))
#endif

/**
 * \brief Use a routing configuration specific to the board.
 * On some platforms GPIO is used for routing certain signals between different
 * devices. GPIO drivers on such platform use named configurations for switching
 * muxes used for such routing.
 * \param dev GPIO device
 * \param config_name name of the routing configuration
 * \param other_config if not NULL, in case of conflicting configurations this
 * will be set to a name of the configuration conflicting with the requested
 * configuration.
 * \return 0 on success, EINVAL if configuration is not recognized.
 * EBUSY in case of conflicting configurations.
 */
static inline int gpio_activate_routing(device_t *dev, const char *config_name,
                                        const char **other_config) {
  device_t *idev = GPIO_METHOD_PROVIDER(dev, activate_routing);
  return gpio_methods(idev->parent)->activate_routing(dev, config_name,
                                                      other_config);
}

/**
 * \brief Get GPIO pin by its name
 * \param dev GPIO device
 * \param name GPIO pin name
 * \return pin number that can be used to reference the pin. GPIO_PIN_INVALID if
 * the requested pin is not found.
 */
static inline gpio_pin_t gpio_get_pin(device_t *dev, const char *name) {
  device_t *idev = GPIO_METHOD_PROVIDER(dev, get_pin);
  return gpio_methods(idev->parent)->get_pin(dev, name);
}

/**
 * \brief Get GPIO pin capabilities
 * \param dev GPIO device
 * \param pin GPIO pin
 * \param caps pointer to gpio_pin_caps_t to be write the results to
 * \return 0 on success, EINVAL if the pin is invalid. 
 */
static inline int gpio_get_pin_caps(device_t *dev, gpio_pin_t pin,
                                    gpio_pin_caps_t *caps) {
  device_t *idev = GPIO_METHOD_PROVIDER(dev, get_pin_caps);
  return gpio_methods(idev->parent)->get_pin_caps(dev, pin, caps);
}

/**
 * \brief Check if a GPIO pin offers requested capabilities
 * \param dev GPIO device
 * \param pin GPIO pin
 * \param caps pointer to gpio_pin_caps_t to be write the results to
 * \return 0 on success, EINVAL if the pin is invalid. ENOTSUP if the
 * capabilites are not provided by the pin.
 */
int gpio_check_pin_caps(device_t *dev, gpio_pin_t pin,
                        const gpio_pin_caps_t *caps);

/**
 * \brief Set pin's pull mode
 * \param dev GPIO device
 * \param pin GPIO pin
 * \param mode GPIO pin pull mode
 * \return 0 on success, EINVAL if the requested mode is not supported or pin
 * is not valid.
 */
static inline int gpio_set_pin_pull(device_t *dev, gpio_pin_t pin,
                                    gpio_pull_mode_t mode) {
  device_t *idev = GPIO_METHOD_PROVIDER(dev, set_pin_pull);
  return gpio_methods(idev->parent)->set_pin_pull(dev, pin, mode);
}

/**
 * \brief Set pin's interrupt detection mode
 * \param dev GPIO device
 * \param pin GPIO pin
 * \param trig interrupt trigger condition
 * \param mode interrupt mode. GPIO_INTR_MODE_SYNC for interrupts synchronized
 * to system clock, GPIO_INTR_MODE_ASYNC for asynchronous interrupts
 * \return 0 on success, EINVAL if the requested mode is not supported or pin is
 * not valid.
 */
static inline int gpio_set_pin_intr(device_t *dev, gpio_pin_t pin,
                                    gpio_intr_trig_t trig,
                                    gpio_intr_mode_t mode) {
  device_t *idev = GPIO_METHOD_PROVIDER(dev, set_pin_intr);
  return gpio_methods(idev->parent)->set_pin_intr(dev, pin, trig, mode);
}

/**
 * \brief Set pin's direction
 * \param dev GPIO device
 * \param pin GPIO pin
 * \param dir GPIO pin direction
 * \return 0 on success, EINVAL if the requested mode is not supported or pin is
 * not valid.
 */
static inline int gpio_set_pin_dir(device_t *dev, gpio_pin_t pin,
                                    gpio_dir_t dir) {
  device_t *idev = GPIO_METHOD_PROVIDER(dev, set_pin_dir);
  return gpio_methods(idev->parent)->set_pin_dir(dev, pin, dir);
}

/**
 * \brief Helper funstion for setting-up a pin using gpio_pin_caps_t struct.
 * caps are required to have at most one flag set for each field.
 */
int gpio_setup_pin_by_caps(device_t *dev, gpio_pin_t pin,
                           const gpio_pin_caps_t *caps);

/**
 * \brief Get a range of GPIO pins that can be mapped to one word.
 * \param dev GPIO device
 * \param pin GPIO pin
 * \param dir GPIO pin direction
 * \return 0 on success, GPIO_POIN_RANGE_INVALID if the range is not valid.
 */
static inline gpio_pin_range_t gpio_get_pin_range(device_t *dev,
                                                  const char *from,
                                                  const char *to) {
  device_t *idev = GPIO_METHOD_PROVIDER(dev, get_pin_range);
  return gpio_methods(idev->parent)->get_pin_range(dev, from, to);
}

/**
 * \brief Get a range of GPIO pins that can be mapped to one word.
 * \param dev GPIO device
 * \return Maximum length of a word supported by the GPIO.
 */
static inline size_t gpio_max_pin_range(device_t *dev) {
  device_t *idev = GPIO_METHOD_PROVIDER(dev, max_pin_range);
  return gpio_methods(idev->parent)->max_pin_range(dev);
}

/**
 * \brief Read bits from a range of GPIO pins
 * \param dev GPIO device
 * \param range range of bits to read from
 * \param word pointer to a word to write the results to
 * \return 0 on success. EINVAL if the range is invalid.
 */
static inline int gpio_read(device_t *dev, gpio_pin_range_t range,
                            gpio_word_t *word) {
  device_t *idev = GPIO_METHOD_PROVIDER(dev, read);
  return gpio_methods(idev->parent)->read(dev, range, word);
}

/**
 * \brief Write bits frtoom a range of GPIO pins
 * \param dev GPIO device
 * \param range range of bits to write to
 * \param word a word to write to the range
 * \return 0 on success. EINVAL if the range is invalid.
 */
static inline int gpio_write(device_t *dev, gpio_pin_range_t range,
                             gpio_word_t word) {
  device_t *idev = GPIO_METHOD_PROVIDER(dev, read);
  return gpio_methods(idev->parent)->write(dev, range, word);
}
