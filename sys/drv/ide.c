/* PCI PIIX4 IDE */
#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/pci.h>
#include <sys/interrupt.h>
#include <sys/klog.h>
#include <sys/timer.h>
#include <sys/spinlock.h>
#include <sys/devclass.h>
#include <dev/pciidereg.h>
#include <dev/idereg.h>

typedef struct ide_device {
  uint8_t identification_space[4096];
  uint8_t model[41];
  unsigned int size;
} ide_device_t;

typedef struct ide_channel_resources {
  resource_t *base;
  resource_t *ctrl;
  resource_t *bmide;
  unsigned char nIEN;
} ide_channel_resources_t;

typedef struct ide_state {
  ide_channel_resources_t ide_channels[2];
  ide_device_t ide_devices[4];
} ide_state_t;

static void ide_write(ide_state_t *ide, unsigned char channel,
                      unsigned char reg, unsigned char data) {
  if (reg > 0x07 && reg < 0x0C)
    ide_write(ide, channel, ATA_REG_CONTROL,
              0x80 | ide->ide_channels[channel].nIEN);
  if (reg < 0x08)
    bus_write_1(ide->ide_channels[channel].base, reg - 0x00, data);
  else if (reg < 0x0C)
    bus_write_1(ide->ide_channels[channel].base, reg - 0x06, data);
  else if (reg < 0x0E)
    bus_write_1(ide->ide_channels[channel].ctrl, reg - 0x0A, data);
  else if (reg < 0x16)
    bus_write_1(ide->ide_channels[channel].bmide, reg + channel * 0x08 - 0x0E,
                data);
  if (reg > 0x07 && reg < 0x0C)
    ide_write(ide, channel, ATA_REG_CONTROL, ide->ide_channels[channel].nIEN);
}

static void ide_write2(ide_state_t *ide, unsigned char channel,
                       unsigned char reg, unsigned short data) {
  if (reg > 0x07 && reg < 0x0C)
    ide_write(ide, channel, ATA_REG_CONTROL,
              0x80 | ide->ide_channels[channel].nIEN);
  if (reg < 0x08)
    bus_write_2(ide->ide_channels[channel].base, reg - 0x00, data);
  else if (reg < 0x0C)
    bus_write_2(ide->ide_channels[channel].base, reg - 0x06, data);
  else if (reg < 0x0E)
    bus_write_2(ide->ide_channels[channel].ctrl, reg - 0x0A, data);
  else if (reg < 0x16)
    bus_write_2(ide->ide_channels[channel].bmide, reg + channel * 0x08 - 0x0E,
                data);
  if (reg > 0x07 && reg < 0x0C)
    ide_write(ide, channel, ATA_REG_CONTROL, ide->ide_channels[channel].nIEN);
}

static unsigned char ide_read(ide_state_t *ide, unsigned char channel,
                              unsigned char reg) {
  unsigned char result;
  result = 0xde;
  if (reg > 0x07 && reg < 0x0C)
    ide_write(ide, channel, ATA_REG_CONTROL,
              0x80 | ide->ide_channels[channel].nIEN);
  if (reg < 0x08)
    result = bus_read_1(ide->ide_channels[channel].base, reg - 0x00);
  else if (reg < 0x0C)
    result = bus_read_1(ide->ide_channels[channel].base, reg - 0x06);
  else if (reg < 0x0E)
    result = bus_read_1(ide->ide_channels[channel].ctrl, reg - 0x0A);
  else if (reg < 0x16)
    result =
      bus_read_1(ide->ide_channels[channel].bmide, reg + channel * 0x08 - 0x0E);
  if (reg > 0x07 && reg < 0x0C)
    ide_write(ide, channel, ATA_REG_CONTROL, ide->ide_channels[channel].nIEN);
  return result;
}

static unsigned short ide_read2(ide_state_t *ide, unsigned char channel,
                                unsigned char reg) {
  unsigned short result;
  result = 0xde;
  if (reg > 0x07 && reg < 0x0C)
    ide_write(ide, channel, ATA_REG_CONTROL,
              0x80 | ide->ide_channels[channel].nIEN);
  if (reg < 0x08)
    result = bus_read_2(ide->ide_channels[channel].base, reg - 0x00);
  else if (reg < 0x0C)
    result = bus_read_2(ide->ide_channels[channel].base, reg - 0x06);
  else if (reg < 0x0E)
    result = bus_read_2(ide->ide_channels[channel].ctrl, reg - 0x0A);
  else if (reg < 0x16)
    result =
      bus_read_2(ide->ide_channels[channel].bmide, reg + channel * 0x08 - 0x0E);
  if (reg > 0x07 && reg < 0x0C)
    ide_write(ide, channel, ATA_REG_CONTROL, ide->ide_channels[channel].nIEN);
  return result;
}

/* TODO: Maybe change this to something else?
 * It should wait an order of n milliseconds */
static void active_wait(int n) {
  for (volatile int i = 0; i < 1000000 * n; i++)
    continue;
}

static void ide_wait_busy(ide_state_t *ide, unsigned char channel) {
  while (ide_read(ide, channel, ATA_REG_STATUS) & ATA_SR_BSY)
    ;
}

static void ide_wait_ready(ide_state_t *ide, unsigned char channel) {
  while (!(ide_read(ide, channel, ATA_REG_STATUS) & ATA_SR_DRDY))
    ;
}

static void __unused ide_hdd_access(ide_state_t *ide, uint8_t drive,
                                    uint16_t *buffer, uint32_t logical_address,
                                    int nbytes, int action) {

  int cylinder;

  uint8_t channel;
  uint8_t sector;
  uint8_t sector_count;
  uint8_t head;
  uint8_t cylinder_low;
  uint8_t cylinder_high;
  uint8_t slave;

  channel = drive / 2;
  slave = drive % 2;
  sector_count = (nbytes + (512 - 1)) / 512;
  sector = (logical_address % 63) + 1;
  cylinder = (logical_address + 1 - sector) / (16 * 63);
  head = (logical_address + 1 - sector) % (16 * 63) / (63);
  cylinder_low = (cylinder >> 0) & 0xFF;
  cylinder_high = (cylinder >> 0) & 0xFF;

  ide_wait_busy(ide, channel);

  ide_write(ide, channel, ATA_REG_HDDEVSEL, 0xA0 | (slave << 4) | head);
  ide_write(ide, channel, ATA_REG_SECCOUNT0, sector_count);
  ide_write(ide, channel, ATA_REG_LBA0, sector);
  ide_write(ide, channel, ATA_REG_LBA1, cylinder_low);
  ide_write(ide, channel, ATA_REG_LBA2, cylinder_high);

  if (action == ATA_READ) {
    ide_write(ide, channel, ATA_REG_COMMAND, ATA_CMD_READ_PIO);

    for (int j = 0; j < sector_count; j++) {

      ide_wait_busy(ide, channel);

      ide_wait_ready(ide, channel);

      for (int i = 0; i < 256; i++)
        buffer[i] = ide_read2(ide, channel, ATA_REG_DATA);
      buffer += 256;
    }
  } else {
    ide_write(ide, channel, ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);

    for (int j = 0; j < sector_count; j++) {

      ide_wait_busy(ide, channel);

      ide_wait_ready(ide, channel);

      for (int i = 0; i < 256; i++)
        ide_write2(ide, channel, ATA_REG_DATA, buffer[i]);
      buffer += 256;
    }
  }
}

static int ide_attach(device_t *dev) {
  ide_state_t *ide = dev->state;

  ide->ide_channels[0].base = device_take_ioports(dev, 0, RF_ACTIVE);
  assert(ide->ide_channels[0].base != NULL);

  ide->ide_channels[0].ctrl = device_take_ioports(dev, 1, RF_ACTIVE);
  assert(ide->ide_channels[0].ctrl != NULL);

  ide->ide_channels[0].bmide = device_take_ioports(dev, 4, RF_ACTIVE);
  assert(ide->ide_channels[0].bmide != NULL);

  ide->ide_channels[0].nIEN = ATA_DISABLE_INTS;

  ide->ide_channels[1].base = device_take_ioports(dev, 2, RF_ACTIVE);
  assert(ide->ide_channels[1].base != NULL);

  ide->ide_channels[1].ctrl = device_take_ioports(dev, 3, RF_ACTIVE);
  assert(ide->ide_channels[1].ctrl != NULL);

  /* bmide for both channels is the same resource */
  ide->ide_channels[1].bmide = ide->ide_channels[0].bmide;

  ide->ide_channels[1].nIEN = ATA_DISABLE_INTS;

  /* We have to zero the control registers first */
  ide_write(ide, ATA_PRIMARY, ATA_REG_CONTROL, 0);
  ide_write(ide, ATA_SECONDARY, ATA_REG_CONTROL, 0);

  ide_write(ide, ATA_PRIMARY, ATA_REG_CONTROL, ATA_DISABLE_INTS);
  ide_write(ide, ATA_SECONDARY, ATA_REG_CONTROL, ATA_DISABLE_INTS);

  for (int channel = 0; channel < 2; channel++)
    for (int slave = 0; slave < 2; slave++) {

      int drive = channel * 2 + slave;

      unsigned char err = 0, status;

      /* Select the drive */
      ide_write(ide, channel, ATA_REG_HDDEVSEL, 0xA0 | (slave << 4));
      active_wait(1);

      ide_write(ide, channel, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
      active_wait(1);

      /* Check if there is a device */
      if (ide_read(ide, channel, ATA_REG_STATUS) == 0) {
        klog("Didn't find any device on channel %d", channel);
        continue;
      }

      /* XXX: We should probably limit the number of iterations here and do a
       * software reset (or even give up after a few resets) if the loop takes
       * too long */
      while (1) {
        status = ide_read(ide, channel, ATA_REG_STATUS);

        /* Device is not an ata device, it might be atapi */
        if ((status & ATA_SR_ERR)) {
          err = 1;
          break;
        }

        /* Device is an ata device */
        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ))
          break;
      }

      uint8_t *is = ide->ide_devices[drive].identification_space;

      for (int i = 0; i < 2048; i++) {
        is[i] = 0;
      }

      if (err) {
        klog("Didn't find any device on channel %d at position %d", channel,
             slave);
      } else {
        ide_write(ide, channel, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

        active_wait(1);

        klog("Found a device on channel %d , position %d", channel, slave);
        for (int i = 0; i < 128; i++) {
          *((short *)(is) + i) = ide_read2(ide, channel, ATA_REG_DATA);
        }
        ide->ide_devices[drive].size =
          512 * *((unsigned int *)(is + ATA_IDENT_MAX_LBA));

        /* big-endian little-endian conversion */
        for (int i = 0; i < 40; i += 2) {
          ide->ide_devices[drive].model[i] = is[ATA_IDENT_MODEL + i + 1];
          ide->ide_devices[drive].model[i + 1] = is[ATA_IDENT_MODEL + i];
        }
        ide->ide_devices[drive].model[40] = 0;
        klog("Model %s of size %u bytes", ide->ide_devices[drive].model,
             ide->ide_devices[drive].size);
      }
    }

  return 0;
}

static int ide_probe(device_t *dev) {
  pci_device_t *pcid = pci_device_of(dev);

  if (!pci_device_match(pcid, 0x8086, 0x7111))
    return 0;

  return 1;
}

/* clang-format off */
static driver_t ide_driver = {
  .desc = "IDE driver",
  .size = sizeof(ide_state_t),
  .attach = ide_attach,
  .probe = ide_probe
};
/* clang-format on */

DEVCLASS_ENTRY(pci, ide_driver);
