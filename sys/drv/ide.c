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
#include <dev/atareg.h>
#include <dev/pciidereg.h>
#include <dev/idereg.h>

typedef struct ide_state {
  resource_t *io_primary;
  resource_t *io_primary_control;
  resource_t *io_secondary;
  resource_t *io_secondary_control;
  resource_t *regs;
  resource_t *irq_res;
} ide_state_t;

struct ide_device {
  unsigned char Reserved;      // 0 (Empty) or 1 (This Drive really exists).
  unsigned char Channel;       // 0 (Primary Channel) or 1 (Secondary Channel).
  unsigned char Drive;         // 0 (Master Drive) or 1 (Slave Drive).
  unsigned short Type;         // 0: ATA, 1:ATAPI.
  unsigned short Signature;    // Drive Signature
  unsigned short Capabilities; // Features.
  unsigned int CommandSets;    // Command Sets Supported.
  unsigned int Size;           // Size in Sectors.
  unsigned char Model[41];     // Model in string.
} ide_devices[4];

#define inb(addr) bus_read_1(ide->regs, (addr))
#define outb(addr, val) bus_write_1(ide->regs, (addr), (val))
#define out4b(addr, val) bus_write_4(ide->regs, (addr), (val))

/*
BAR0: Base address of primary channel (I/O space), if it is 0x0 or 0x1, the port
is 0x1F0. BAR1: Base address of primary channel control port (I/O space), if it
is 0x0 or 0x1, the port is 0x3F6. BAR2: Base address of secondary channel (I/O
space), if it is 0x0 or 0x1, the port is 0x170. BAR3: Base address of secondary
channel control port, if it is 0x0 or 0x1, the port is 0x376. BAR4: Bus Master
IDE; refers to the base of I/O range consisting of 16 ports. Each 8 ports
controls DMA on the primary and secondary channel respectively.
*/

#define STATUS_BSY 0x80
#define STATUS_RDY 0x40
#define STATUS_DRQ 0x08
#define STATUS_DF 0x20
#define STATUS_ERR 0x01

unsigned char ide_buf[2048] = {0};
unsigned char ide_irq_invoked = 0;
unsigned char atapi_packet[12] = {0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

struct IDEChannelRegisters {
  resource_t *base;   // I/O Base.
  resource_t *ctrl;   // Control Base
  resource_t *bmide;  // Bus Master IDE
  unsigned char nIEN; // nIEN (No Interrupt);
} channels[2];

void ide_write(unsigned char channel, unsigned char reg, unsigned char data) {
  if (reg > 0x07 && reg < 0x0C)
    ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
  if (reg < 0x08)
    bus_write_1(channels[channel].base, reg - 0x00, data);
  else if (reg < 0x0C)
    bus_write_1(channels[channel].base, reg - 0x06, data);
  else if (reg < 0x0E)
    bus_write_1(channels[channel].ctrl, reg - 0x0A, data);
  else if (reg < 0x16)
    bus_write_1(channels[channel].bmide, reg + channel * 0x08 - 0x0E, data);
  if (reg > 0x07 && reg < 0x0C)
    ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
}

unsigned char ide_read(unsigned char channel, unsigned char reg) {
  unsigned char result;
  result = 0xde;
  if (reg > 0x07 && reg < 0x0C)
    ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
  if (reg < 0x08)
    result = bus_read_1(channels[channel].base, reg - 0x00);
  else if (reg < 0x0C)
    result = bus_read_1(channels[channel].base, reg - 0x06);
  else if (reg < 0x0E)
    result = bus_read_1(channels[channel].ctrl, reg - 0x0A);
  else if (reg < 0x16)
    result = bus_read_1(channels[channel].bmide, reg - 0x0E);
  if (reg > 0x07 && reg < 0x0C)
    ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
  return result;
}

unsigned short ide_read2(unsigned char channel, unsigned char reg) {
  unsigned short result;
  result = 0xde;
  if (reg > 0x07 && reg < 0x0C)
    ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
  if (reg < 0x08)
    result = bus_read_2(channels[channel].base, reg - 0x00);
  else if (reg < 0x0C)
    result = bus_read_2(channels[channel].base, reg - 0x06);
  else if (reg < 0x0E)
    result = bus_read_2(channels[channel].ctrl, reg - 0x0A);
  else if (reg < 0x16)
    result = bus_read_2(channels[channel].bmide, reg - 0x0E);
  if (reg > 0x07 && reg < 0x0C)
    ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
  return result;
}

/* TODO: Maybe change this to something else?
 * It should wait an order of n milliseconds */
static void active_wait(int n) {
  for (volatile int i = 0; i < 1000000 * n; i++)
    continue;
}

static int ide_attach(device_t *dev) {
  ide_state_t *ide = dev->state;

  ide->io_primary = device_take_ioports(dev, 0, RF_ACTIVE);
  assert(ide->io_primary != NULL);

  ide->io_primary_control = device_take_ioports(dev, 1, RF_ACTIVE);
  assert(ide->io_primary_control != NULL);

  ide->io_secondary = device_take_ioports(dev, 2, RF_ACTIVE);
  assert(ide->io_secondary != NULL);

  ide->io_secondary_control = device_take_ioports(dev, 3, RF_ACTIVE);
  assert(ide->io_secondary_control != NULL);

  ide->regs = device_take_ioports(dev, 4, RF_ACTIVE);
  assert(ide->regs != NULL);

  channels[0].base = ide->io_primary;
  channels[0].ctrl = ide->io_primary_control;
  channels[0].bmide = ide->regs;
  channels[0].nIEN = 0;

  channels[1].base = ide->io_secondary;
  channels[1].ctrl = ide->io_secondary_control;
  channels[1].bmide = ide->regs;
  channels[1].nIEN = 0;

  int count = 0;

  ide_write(ATA_PRIMARY, ATA_REG_CONTROL, 2);
  ide_write(ATA_SECONDARY, ATA_REG_CONTROL, 2);

  ide_write(0, ATA_REG_CONTROL, 0);
  ide_write(1, ATA_REG_CONTROL, 0);

  for (int i = 0; i < 2; i++)
    for (int j = 0; j < 2; j++) {

      unsigned char err = 0, status;

      /* Select the j-th drive */
      ide_write(i, ATA_REG_HDDEVSEL, 0xA0 | (j << 4));
      active_wait(1);

      /* Send the IDENTIFY command */
      ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
      active_wait(1);

      /* Check if there is a device */
      if (ide_read(i, ATA_REG_STATUS) == 0) {
        klog("Didn't find any device on channel %d", i);
        continue; // If Status = 0, No Device.
      }

      /* XXX: We should probably limit the number of iterations here and do a
       * software reset (or even give up after a few resets) if the loop takes
       * too long */
      while (1) {
        status = ide_read(i, ATA_REG_STATUS);

        /* Device is not an ata device, it might be atapi */
        if ((status & ATA_SR_ERR)) {
          err = 1;
          break;
        }

        /* Device is an ata device */
        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ))
          break;
      }

      short identification_space[2048] = {0};

      if (err) {
        klog("Didn't find any device on channel %d at drive %d", i, j);
      } else {
        ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

        active_wait(1);

        klog("Found a device on channel %d , drive %d", i, j);
        for (int k = 0; k < 128; k++) {
          identification_space[k] = ide_read2(i, ATA_REG_DATA);
        }
        char *ptr = (char *)identification_space;

        for (int k = 0; k < 40; k += 2) {
          ide_devices[count].Model[k] = ptr[ATA_IDENT_MODEL + k + 1];
          ide_devices[count].Model[k + 1] = ptr[ATA_IDENT_MODEL + k];
        }
        ide_devices[count].Model[40] = 0;
        klog("Model %s", ide_devices[count].Model);
      }

      count++;
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
