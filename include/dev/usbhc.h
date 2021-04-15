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
typedef void (*usbhc_control_transfer_t)(device_t *dev, uint16_t size,
                                         uint16_t mps, uint8_t addr,
                                         usb_buf_t *usbb,
                                         usb_device_request_t *req);
typedef void (*usbhc_intr_transfer_t)(device_t *dev, uint16_t size,
                                      uint16_t mps, uint8_t endp, uint8_t addr,
                                      uint8_t interval, usb_buf_t *usbb);
typedef void (*usbhc_bulk_transfer_t)(device_t *dev, uint16_t size,
                                      uint16_t mps, uint8_t endp, uint8_t addr,
                                      usb_buf_t *usbb);

typedef struct usbhc_methods {
  usbhc_number_of_ports_t number_of_ports;
  usbhc_device_present_t device_present;
  usbhc_reset_port_t reset_port;
  usbhc_control_transfer_t control_in;
  usbhc_control_transfer_t control_out;
  usbhc_intr_transfer_t intr_in;
  usbhc_intr_transfer_t intr_out;
  usbhc_bulk_transfer_t bulk_in;
  usbhc_bulk_transfer_t bulk_out;
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

/*! \brief Schedules a control input transfer between the host and
 * the specified USB device.
 *
 * This is an asynchronic function.
 *
 * \param dev USB bus device
 * \param size transfer size
 * \param mps max packet size handled by the device
 * \param addr USB device address on the USB bus
 * \param usbb USB buffer used for the transfer
 * \param req USB device request
 */
static void usbhc_control_in(device_t *dev, uint16_t size, uint16_t mps,
                             uint8_t addr, usb_buf_t *usbb,
                             usb_device_request_t *req) {
  device_t *idev = USBHC_METHOD_PROVIDER(dev, read);
  usbhc_methods(idev->parent)->control_in(dev, size, mps, addr, usbb, req);
}

/*! \brief Schedules a control output transfer between the host and
 * the specified USB device.
 *
 * This is an asynchronic function.
 *
 * \param dev USB bus device
 * \param size transfer size
 * \param mps max packet size handled by the device
 * \param addr USB device address on the USB bus
 * \param usbb USB buffer used for the transfer
 * \param req USB device request
 */
static void usbhc_control_out(device_t *dev, uint16_t size, uint16_t mps,
                              uint8_t addr, usb_buf_t *usbb,
                              usb_device_request_t *req) {
  device_t *idev = USBHC_METHOD_PROVIDER(dev, read);
  usbhc_methods(idev->parent)->control_out(dev, size, mps, addr, usbb, req);
}

/*! \brief Schedules an input interrupt transfer between the host
 * and the specified USB device.
 *
 * This is an asynchronic function.
 *
 * \param dev USB bus device
 * \param size transfer size
 * \param mps max packet size handled by the device
 * \param endp endpoint address within the device
 * \param addr USB device address on the USB bus
 * \param interval when to perform the transfer
 * \param usbb USB buffer used for the transfer
 */
static void usbhc_intr_in(device_t *dev, uint16_t size, uint16_t mps,
                          uint8_t endp, uint8_t addr, uint8_t interval,
                          usb_buf_t *usbb) {
  device_t *idev = USBHC_METHOD_PROVIDER(dev, read);
  usbhc_methods(idev->parent)
    ->intr_in(dev, size, mps, endp, addr, interval, usbb);
}

/*! \brief Schedules an output interrupt transfer between the host
 * and the specified USB device.
 *
 * This is an asynchronic function.
 *
 * \param dev USB bus device
 * \param size transfer size
 * \param mps max packet size handled by the device
 * \param endp endpoint address within the device
 * \param addr USB device address on the USB bus
 * \param interval when to perform the transfer
 * \param usbb USB buffer used for the transfer
 */
static void usbhc_intr_out(device_t *dev, uint16_t size, uint16_t mps,
                           uint8_t endp, uint8_t addr, uint8_t interval,
                           usb_buf_t *usbb) {
  device_t *idev = USBHC_METHOD_PROVIDER(dev, read);
  usbhc_methods(idev->parent)
    ->intr_out(dev, size, mps, endp, addr, interval, usbb);
}

/*! \brief Schedules an input bulk transfer between the host
 * and the specified USB device.
 *
 * This is an asynchronic function.
 *
 * \param dev USB bus device
 * \param size transfer size
 * \param mps max packet size handled by the device
 * \param endp endpoint address within the device
 * \param addr USB device address on the USB bus
 * \param usbb USB buffer used for the transfer
 */
static void usbhc_bulk_in(device_t *dev, uint16_t size, uint16_t mps,
                          uint8_t endp, uint8_t addr, usb_buf_t *usbb) {
  device_t *idev = USBHC_METHOD_PROVIDER(dev, read);
  usbhc_methods(idev->parent)->bulk_in(dev, size, mps, endp, addr, usbb);
}

/*! \brief Schedules an output bulk transfer between the host
 * and the specified USB device.
 *
 * This is an asynchronic function.
 *
 * \param dev USB bus device
 * \param size transfer size
 * \param mps max packet size handled by the device
 * \param endp endpoint address within the device
 * \param addr USB device address on the USB bus
 * \param usbb USB buffer used for the transfer
 */
static void usbhc_bulk_out(device_t *dev, uint16_t size, uint16_t mps,
                           uint8_t endp, uint8_t addr, usb_buf_t *usbb) {
  device_t *idev = USBHC_METHOD_PROVIDER(dev, read);
  usbhc_methods(idev->parent)->bulk_out(dev, size, mps, endp, addr, usbb);
}

#endif /* _DEV_USBHC_H_ */
