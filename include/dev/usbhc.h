#ifndef _USBHC_H_
#define _USBHC_H_

#include <dev/usb.h>

typedef uint8_t (*number_of_ports_t)(void *);
typedef bool (*device_present_t)(void *, uint8_t);
typedef void (*reset_port_t)(void *, uint8_t);
typedef void (*transfer_t)(void *, usb_device_t *, usb_buf_t *,
                           usb_device_request_t *);

typedef struct usbhc_space {
  void *hc;
  number_of_ports_t number_of_ports;
  device_present_t device_present;
  reset_port_t reset_port;
  transfer_t transfer;
} usbhc_space_t;

extern usbhc_space_t *__hcs;

#define USBHC_SPACE_DEFINE usbhc_space_t *__hcs

#define USBHC_SPACE_SET_STATE(state) (__hcs->hc = (state))

#define USBHC_SPACE_SET(name, val) .name = (name##_t)(val)

#define USBHC_SPACE_REGISTER(s) (__hcs = &(s))

#define USBHC_SPACE_CALL(name, ...) __hcs->name(__hcs->hc, ##__VA_ARGS__)

static inline uint8_t usbhc_number_of_ports(void) {
  return USBHC_SPACE_CALL(number_of_ports);
}

static inline bool usbhc_device_present(uint8_t port) {
  return USBHC_SPACE_CALL(device_present, port);
}

static inline void usbhc_reset_port(uint8_t port) {
  USBHC_SPACE_CALL(reset_port, port);
}

static inline void usbhc_transfer(usb_device_t *usbd, usb_buf_t *usbb,
                                  usb_device_request_t *req) {
  USBHC_SPACE_CALL(transfer, usbd, usbb, req);
}

#endif /* _USBHC_H_ */
