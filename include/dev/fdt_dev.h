#ifndef _DEV_FDT_DEV_H_
#define _DEV_FDT_DEV_H_

#include <sys/devclass.h>
#include <sys/device.h>

/*
 * Add a device that is not a child of the SOC device tree node
 * based on the FDT.
 *
 * Arguments:
 *  - `rootdev`: root bus device
 *  - `path`: absolute FDT device path
 *  - `unit`: unit identifier
 *
 * Returns:
 *  - 0: success
 *  - != 0: errno code identifying the occured error
 */
int FDT_dev_add_device_by_path(device_t *rootdev, const char *path, int unit);

/*
 * The same as `FDT_dev_add_device_by_path` but the device is identified
 * by provided device node `node`.
 */
int FDT_dev_add_device_by_node(device_t *rootdev, phandle_t node, int unit);

int FDT_dev_get_region(device_t *dev, fdt_mem_reg_t *mrs, size_t *cntp);

/*
 * Check whether device node `node` is compatible
 * with device specified by `compatible`.
 *
 * All compatible strings of the device are examined.
 *
 * Returns:
 *  - 0: no match
 *  - 1: match
 */
int FDT_dev_is_compatible(device_t *dev, const char *compatible);

DEVCLASS_DECLARE(simplebus);

#endif /* !_DEV_FDT_DEV_H_ */
