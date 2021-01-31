#ifndef _USBHC_H_
#define _USBHC_H_

#include <dev/usb.h>

typedef uint8_t (*number_of_ports_t)(void *);
typedef bool (*device_present_t)(void *, uint8_t);
/* (MichalBlk) FTTB this only applies to root hub ports
 * (i.e. hubs are not supported). */
typedef void (*reset_port_t)(void *, uint8_t);
typedef void (*transfer_t)(void *, usb_device_t *, usb_buf_t *,
                           usb_device_request_t *);

typedef struct usbhc_space {
  void *host_controller;
  number_of_ports_t number_of_ports;
  device_present_t device_present;
  reset_port_t reset_port;
  transfer_t transfer;
} usbhc_space_t;

#define USBHC_SPACE_SET_STATE(uhs, state) ((uhs)->host_controller = (state))
#define USBHC_SPACE_SET(name, val) .name = (name##_t)(val)

#define USBHC_SPACE_CALL(uhs, name, ...)                                       \
  ((uhs)->name((uhs)->host_controller, ##__VA_ARGS__))

static inline uint8_t usbhc_number_of_ports(usbhc_space_t *uhs) {
  return USBHC_SPACE_CALL(uhs, number_of_ports);
}

static inline bool usbhc_device_present(usbhc_space_t *uhs, uint8_t port) {
  return USBHC_SPACE_CALL(uhs, device_present, port);
}

static inline void usbhc_reset_port(usbhc_space_t *uhs, uint8_t port) {
  USBHC_SPACE_CALL(uhs, reset_port, port);
}

static inline void usbhc_transfer(usb_device_t *usbd, usb_buf_t *usbb,
                                  usb_device_request_t *req) {
  USBHC_SPACE_CALL(usbd->uhs, transfer, usbd, usbb, req);
}

#endif /* _USBHC_H_ */
