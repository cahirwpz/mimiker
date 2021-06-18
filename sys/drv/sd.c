#define KL_LOG KL_DEV
#include <sys/mimiker.h>
#include <sys/klog.h>
#include <sys/errno.h>
#include <dev/emmc.h>
#include <sys/device.h>
#include <sys/devclass.h>

typedef struct sd_state {
} sd_state_t;

static int sd_probe(device_t *dev) {
  return 0;
}

static int sd_attach(device_t *dev) {
  return ENXIO;
}

static driver_t sd_card_driver = {
  .desc = "SD(SC/HC) card driver",
  .size = sizeof(sd_state_t),
  .probe = sd_probe,
  .attach = sd_attach,
  .pass = SECOND_PASS,
  .interfaces = {},
};

DEVCLASS_ENTRY(emmc, sd_card_driver);
