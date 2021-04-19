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
typedef void (*usbhc_control_transfer_t)(device_t *dev, uint16_t maxpkt,
                                         uint8_t port, uint8_t addr,
                                         usb_buf_t *buf,
                                         usb_device_request_t *req);
typedef void (*usbhc_interrupt_transfer_t)(device_t *dev, uint16_t maxpkt,
                                           uint8_t port, uint8_t addr,
                                           uint8_t endpt, uint8_t interval,
                                           usb_buf_t *buf);
typedef void (*usbhc_bulk_transfer_t)(device_t *dev, uint16_t maxpkt,
                                      uint8_t port, uint8_t addr, uint8_t endpt,
                                      usb_buf_t *buf);

typedef struct usbhc_methods {
  usbhc_number_of_ports_t number_of_ports;
  usbhc_device_present_t device_present;
  usbhc_reset_port_t reset_port;
  usbhc_control_transfer_t control_transfer;
  usbhc_interrupt_transfer_t interrupt_transfer;
  usbhc_bulk_transfer_t bulk_transfer;
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
 * controlled by the specified host controller.
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

/*! \brief Resets the specified root hub port.
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

/*! \brief Schedules a control transfer between the host and
 * the specified USB device.
 *
 * This is an asynchronic function.
 *
 * \param dev USB device
 * \param maxpkt max packet size handled by the device
 * \param port port the device is attached to
 * \param addr USB device address on the USB bus
 * \param buf USB buffer used for the transfer
 * \param req USB device request
 */
static inline void usbhc_control_transfer(device_t *dev, uint16_t maxpkt,
                                          uint8_t port, uint8_t addr,
                                          usb_buf_t *buf,
                                          usb_device_request_t *req) {
  device_t *idev = USBHC_METHOD_PROVIDER(dev, control_transfer);
  usbhc_methods(idev->parent)
    ->control_transfer(idev, maxpkt, port, addr, buf, req);
}

/*! \brief Schedules an interrupt transfer between the host
 * and the specified USB device.
 *
 * This is an asynchronic function.
 *
 * \param dev USB device
 * \param maxpkt max packet size handled by the device
 * \param port port the device is attached to
 * \param addr USB device address on the USB bus
 * \param endpt endpoint address within the device
 * \param interval when to perform the transfer
 * \param buf USB buffer used for the transfer
 */
static inline void usbhc_interrupt_transfer(device_t *dev, uint16_t maxpkt,
                                            uint8_t port, uint8_t addr,
                                            uint8_t endpt, uint8_t interval,
                                            usb_buf_t *buf) {
  device_t *idev = USBHC_METHOD_PROVIDER(dev, interrupt_transfer);
  usbhc_methods(idev->parent)
    ->interrupt_transfer(idev, maxpkt, port, addr, endpt, interval, buf);
}

/*! \brief Schedules a bulk transfer between the host
 * and the specified USB device.
 *
 * This is an asynchronic function.
 *
 * \param dev USB device
 * \param maxpkt max packet size handled by the device
 * \param port port the device is attached to
 * \param addr USB device address on the USB bus
 * \param endpt endpoint address within the device
 * \param buf USB buffer used for the transfer
 */
static inline void usbhc_bulk_transfer(device_t *dev, uint16_t maxpkt,
                                       uint8_t port, uint8_t addr,
                                       uint8_t endpt, usb_buf_t *buf) {
  device_t *idev = USBHC_METHOD_PROVIDER(dev, bulk_transfer);
  usbhc_methods(idev->parent)
    ->bulk_transfer(idev, maxpkt, port, addr, endpt, buf);
}

#endif /* _DEV_USBHC_H_ */
