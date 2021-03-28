/*
 * The following routines define an interface implemented by each USB host
 * controller. This interface is meant to be used by the USB bus methods
 * to service requests issued by USB device drivers.
 */
#ifndef _DEV_USBHC_H_
#define _DEV_USBHC_H_

#include <dev/usb.h>

typedef uint8_t (*usbhc_number_of_ports_t)(device_t *dev);
typedef bool (*usbhc_device_present_t)(device_t *dev, uint8_t port);
typedef void (*usbhc_reset_port_t)(device_t *dev, uint8_t port);
typedef void (*usbhc_transfer_t)(device_t *dev, usb_buf_t *usbb,
                                 usb_device_request_t *req);

typedef struct usbhc_methods {
  usbhc_number_of_ports_t number_of_ports;
  usbhc_device_present_t device_present;
  usbhc_reset_port_t reset_port;
  usbhc_transfer_t transfer;
} usbhc_methods_t;

static inline usbhc_methods_t *usbhc_methods(device_t *dev) {
  return (usbhc_methods_t *)dev->driver->interfaces[DIF_USBHC];
}

#define USBHC_METHOD_PROVIDER(dev, method)                                     \
  (device_method_provider((dev), DIF_USBHC,                                    \
                          offsetof(struct usbhc_methods, method)))

/*! \brief Returns the number of root hub ports.
 *
 * This function will be called during enumeration of ports
 * controlled by specified host controller.
 *
 * \param dev host controller device
 */
static inline uint8_t usbhc_number_of_ports(device_t *dev) {
  return usbhc_methods(dev)->number_of_ports(dev);
}

/*! \brief Checks whether any device is attached to the specified
 * root hub port.
 *
 * This function will be called during enumeration of ports
 * controlled by specified host controller.
 *
 * \param dev host controller device
 * \param port number of the root hub port to examine
 */
static inline bool usbhc_device_present(device_t *dev, uint8_t port) {
  return usbhc_methods(dev)->device_present(dev, port);
}

/*! \brief Resets to the specified root hub port.
 *
 * This function is essential to bring the attached device to the
 * default state.
 *
 * \param dev host controller device
 * \param port number of the root hub port to examine
 */
static inline void usbhc_reset_port(device_t *dev, uint8_t port) {
  usbhc_methods(dev)->reset_port(dev, port);
}

/*! \brief Schedules a transfer between the host and the specified USB device.
 *
 * USB bus transfers are main actions performed by each USB device.
 * This is an asynchronic function.
 *
 * \param usbd USB device requesting the transfer
 * \param usbb USB buffer defining the transfer
 * \param req USB device request in case of controll transfers
 */
static inline void usbhc_transfer(device_t *dev, usb_buf_t *usbb,
                                  usb_device_request_t *req) {
  device_t *idev = USBHC_METHOD_PROVIDER(dev, transfer);
  usbhc_methods(idev->parent)->transfer(dev, usbb, req);
}

#endif /* _DEV_USBHC_H_ */
