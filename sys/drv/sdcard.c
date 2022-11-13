#define KL_LOG KL_DEV
#include <sys/mimiker.h>
#include <sys/klog.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/devclass.h>
#include <sys/fdt.h>
#include <dev/emmc.h>

/* Dummy driver that will never attach. Used to satisfy linker set
 * non-empty requirement */

static int sdcard_probe(device_t *dev) {
  return FDT_is_compatible(dev->node, "brcm,bcm2835-sdcard");
}

static int sdcard_attach(device_t *dev) {
  return 0;
}

static driver_t sdcard_driver = {
  .desc = "SD(SC/HC) card driver",
  .size = 0,
  .probe = sdcard_probe,
  .attach = sdcard_attach,
  .pass = SECOND_PASS,
  .interfaces = {},
};

DEVCLASS_ENTRY(emmc, sdcard_driver);
