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

typedef struct ide_state {
  resource_t *regs;
  resource_t *irq_res;
} ide_state_t;

#define inb(addr) bus_read_1(ide->regs, (addr))
#define outb(addr, val) bus_write_1(ide->regs, (addr), (val))
#define out4b(addr, val) bus_write_4(ide->regs, (addr), (val))

/*
static intr_filter_t pit_intr(void *data) {
  pit_state_t *pit = data;

  WITH_SPIN_LOCK (&pit->lock) {
    bintime_add_frac(&pit->time, pit->period_frac);
    pit->overflow = false;
    pit->last_cntr = pit_get_counter(pit);
  }
  tm_trigger(&pit->timer);
  return IF_FILTERED;
}
*/

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

static void ATA_wait_BSY(ide_state_t *ide) // Wait for bsy to be 0
{
  while (inb(7) & STATUS_BSY)
    ;
}
static void ATA_wait_DRQ(ide_state_t *ide) // Wait fot drq to be 1
{
  while (!(inb(7) & STATUS_RDY))
    ;
}
void read_sectors(ide_state_t *ide) {

  // reg cheat sheet https://www.bswd.com/idems100.pdf

  /*
    int __unused r0 = 0xf0, __unused r1 = 0xf0, __unused r2 = 0xf0,
                 __unused r3 = 0xf0, __unused r4 = 0xf0, r5 = 0xf0,
                 __unused r6 = 0xf0, __unused r7 = 0xf0;

    for (int i = 0; i < 16; i++)
      if (i != 10)
        outb(i, 0xff);

    for (int i = 0; i < 16; i++)
      klog("%d  %d\n", i, inb(i));
  */
  unsigned short a[256];
  for (int i = 0; i < 256; i++)
    a[i] = 1;

  uint64_t prd[2];

  prd[0] = (uint32_t)a;

  prd[1] = 128;

  outb(0xc, (uint32_t)prd);

  outb(0x8, 0);
  outb(0xa, 6);

  outb(0x8, 1);

  ATA_wait_BSY(ide);

  for (int i = 0; i < 16; i++)
    klog("%d  %d\n", i, inb(i));
  outb(5, 1);
  outb(2, 1);
  outb(3, 0);
  outb(4, 0);
  outb(5, 0);
  outb(7, 0x20); // Send the read command

  uint16_t *target = (uint16_t *)a;

  ATA_wait_BSY(ide);
  ATA_wait_DRQ(ide);
  for (int i = 0; i < 256; i++)
    target[i] = inb(0);
  target += 256;
}

static int ide_attach(device_t *dev) {
  ide_state_t *ide = dev->state;

  ide->regs = device_take_ioports(dev, 0, RF_ACTIVE);
  assert(ide->regs != NULL);

  // ide->irq_res = device_take_irq(dev, 0, RF_ACTIVE);

  read_sectors(ide);

  return 0;
}

static int ide_probe(device_t *dev) {
  return dev->unit == 4; /* XXX: unit 4 assigned by gt_pci */
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
