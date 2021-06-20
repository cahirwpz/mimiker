#define KL_LOG KL_DEV
#include <sys/mimiker.h>
#include <sys/klog.h>
#include <sys/errno.h>
#include <dev/emmc.h>
#include <sys/device.h>
#include <sys/devclass.h>

/* Dummy driver that will never attach. Used to satisfy linker set
 * non-empty requirement */

static int sd_probe(device_t *dev) {
  return 0;
}

static int sd_attach(device_t *dev) {
  return ENXIO;
}

static driver_t emmc_dummy_driver = {
  .desc = "SD(SC/HC) card driver",
  .size = 0,
  .probe = sd_probe,
  .attach = sd_attach,
  .pass = SECOND_PASS,
  .interfaces = {},
};

DEVCLASS_ENTRY(emmc, sd_card_driver);
