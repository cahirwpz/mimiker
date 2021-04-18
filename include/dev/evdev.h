#ifndef _DEV_EVDEV_H_
#define _DEV_EVDEV_H_

#include <sys/input.h>

#define EVDEV_FLAG_SOFTREPEAT 0x00 /* use evdev to repeat keys */
#define EVDEV_FLAG_MAX 0x1F
#define EVDEV_FLAG_CNT (EVDEV_FLAG_MAX + 1)

typedef enum {
  KEY_EVENT_UP,
  KEY_EVENT_DOWN,
  KEY_EVENT_REPEAT
} evdev_key_events_t;

typedef struct evdev_dev evdev_dev_t;

/* Input device interface: */
evdev_dev_t *evdev_alloc(void);
void evdev_free(evdev_dev_t *evdev);
void evdev_set_name(evdev_dev_t *evdev, const char *name);
void evdev_set_id(evdev_dev_t *evdev, uint16_t bustype, uint16_t vendor,
                  uint16_t product, uint16_t version);
void evdev_support_event(evdev_dev_t *evdev, uint16_t);
void evdev_support_key(evdev_dev_t *evdev, uint16_t);
int evdev_register(evdev_dev_t *evdev);
int evdev_unregister(evdev_dev_t *evdev);
int evdev_push_event(evdev_dev_t *evdev, uint16_t type, uint16_t code,
                     int32_t value);

/* Utility functions: */
uint16_t evdev_hid2key(int scancode);
uint16_t evdev_scancode2key(int *state, int scancode);
void evdev_support_all_known_keys(evdev_dev_t *evdev);

/* Event reporting shortcuts: */
static inline int evdev_sync(evdev_dev_t *evdev) {

  return evdev_push_event(evdev, EV_SYN, SYN_REPORT, 1);
}

static inline int evdev_push_key(evdev_dev_t *evdev, uint16_t code,
                                 int32_t value) {
  return evdev_push_event(evdev, EV_KEY, code, value != 0);
}

#endif /* _DEV_EVDEV_H_ */