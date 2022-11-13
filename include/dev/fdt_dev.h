#ifndef _DEV_FDT_DEV_H_
#define _DEV_FDT_DEV_H_

#include <sys/device.h>

typedef uint32_t pcell_t;

/*
 * Add a device represented as an FDT node.
 *
 * Arguments:
 *  - `bus`: parent bus of the device
 *  - `path`: absolute FDT device path
 *  - `dev_bus`: device's bus identifier
 *
 * Returns:
 *  - 0: success
 *  - != 0: errno code identifying the occured error
 */
int FDT_dev_add_child(device_t *bus, const char *path, device_bus_t dev_bus);

int FDT_dev_get_intr_cells(device_t *dev, dev_intr_t *intr, pcell_t *cells,
                           size_t *cntp);

#endif /* !_DEV_FDT_DEV_H_ */
