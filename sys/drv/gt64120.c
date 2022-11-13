/* GT64120 PCI bus driver
 *
 * Heavily inspired by FreeBSD / NetBSD `gt_pci.c` file.
 */
#define KL_LOG KL_DEV
#include <sys/errno.h>
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <dev/malta.h>
#include <dev/i8259.h>
#include <dev/piixreg.h>
#include <dev/isareg.h>
#include <dev/gt64120reg.h>
#include <dev/pci.h>
#include <sys/mutex.h>
#include <sys/libkern.h>
#include <sys/bus.h>
#include <sys/devclass.h>

#define PCI0_CFG_REG_SHIFT 2
#define PCI0_CFG_FUNCT_SHIFT 8
#define PCI0_CFG_DEV_SHIFT 11
#define PCI0_CFG_BUS_SHIFT 16
#define PCI0_CFG_ENABLE 0x80000000

static unsigned gt_pci_make_addr(pci_addr_t addr, unsigned reg) {
  return (((addr.device) << PCI0_CFG_DEV_SHIFT) |
          ((addr.function) << PCI0_CFG_FUNCT_SHIFT) |
          ((reg) << PCI0_CFG_REG_SHIFT));
}

typedef union {
  uint8_t byte[4];
  uint16_t word[2];
  uint32_t dword;
} pci_reg_t;

#define LO(x) ((x)&0xff)
#define HI(x) (((x) >> 8) & 0xff)
#define ICU_ADDR(x) ((x) + 0)
#define ICU_DATA(x) ((x) + 1)
#define ICU1_ADDR ICU_ADDR(IO_ICU1)
#define ICU1_DATA ICU_DATA(IO_ICU1)
#define ICU2_ADDR ICU_ADDR(IO_ICU2)
#define ICU2_DATA ICU_DATA(IO_ICU2)

typedef struct gt_pci_state {

  /* Resources belonging to this driver. */
  dev_mem_t *corectrl;
  dev_mem_t *pci_io;
  dev_mem_t *pci_mem;
  dev_intr_t *irq_res;

  intr_event_t *intr_event[IO_ICUSIZE];

  uint16_t imask;
  uint16_t elcr;
} gt_pci_state_t;

static driver_t gt_pci_bus;

/* Access configuration space through memory mapped GT-64120 registers. Take
 * care of the fact that MIPS processor cannot handle unaligned accesses.
 * Note that Galileo controller's registers are little endian. */
static uint32_t gt_pci_read_config(device_t *dev, unsigned reg, unsigned size) {
  pci_device_t *pcid = pci_device_of(dev);
  gt_pci_state_t *gtpci = dev->parent->state;
  dev_mem_t *pcicfg = gtpci->corectrl;
  assert(pcid);

  if (pcid->addr.bus > 0)
    return -1;

  bus_write_4(pcicfg, GT_PCI0_CFG_ADDR,
              PCI0_CFG_ENABLE | gt_pci_make_addr(pcid->addr, reg >> 2));
  pci_reg_t data = (pci_reg_t)bus_read_4(pcicfg, GT_PCI0_CFG_DATA);

  reg &= 3;
  switch (size) {
    case 1:
      return data.byte[reg];
    case 2:
      return data.word[reg >> 1];
    case 4:
      return data.dword;
    default:
      return -1;
  }
}

static void gt_pci_write_config(device_t *dev, unsigned reg, unsigned size,
                                uint32_t value) {
  pci_device_t *pcid = pci_device_of(dev);
  gt_pci_state_t *gtpci = dev->parent->state;
  dev_mem_t *pcicfg = gtpci->corectrl;
  assert(pcid);

  if (pcid->addr.bus > 0)
    return;

  bus_write_4(pcicfg, GT_PCI0_CFG_ADDR,
              PCI0_CFG_ENABLE | gt_pci_make_addr(pcid->addr, reg >> 2));
  pci_reg_t data = (pci_reg_t)bus_read_4(pcicfg, GT_PCI0_CFG_DATA);

  reg &= 3;
  switch (size) {
    case 1:
      data.byte[reg] = value;
      break;
    case 2:
      data.word[reg >> 1] = value;
      break;
    case 4:
      data.dword = value;
      break;
    default:
      break;
  }

  bus_write_4(pcicfg, GT_PCI0_CFG_DATA, data.dword);
}

static int gt_pci_route_interrupt(device_t *dev) {
  pci_device_t *pcid = pci_device_of(dev);
  int pin = pcid->pin;

  if (pin == 1 || pin == 2)
    return 10;
  if (pin == 3 || pin == 4)
    return 11;
  return -1;
}

static void gt_pci_enable_busmaster(device_t *dev) {
  uint16_t cmd = pci_read_config_2(dev, PCIR_COMMAND);
  pci_write_config_2(dev, PCIR_COMMAND, cmd | PCIM_CMD_BUSMASTEREN);
}

static void gt_pci_set_icus(gt_pci_state_t *gtpci) {
  /* Enable the cascade IRQ (2) if 8-15 is enabled. */
  if ((gtpci->imask & 0xff00) != 0xff00)
    gtpci->imask &= ~(1U << 2);
  else
    gtpci->imask |= (1U << 2);

  dev_mem_t *io = gtpci->pci_io;
  bus_write_1(io, ICU1_DATA, LO(gtpci->imask));
  bus_write_1(io, ICU2_DATA, HI(gtpci->imask));
  bus_write_1(io, PIIX_REG_ELCR + 0, LO(gtpci->elcr));
  bus_write_1(io, PIIX_REG_ELCR + 1, HI(gtpci->elcr));
}

static void gt_pci_mask_irq(intr_event_t *ie) {
  gt_pci_state_t *gtpci = ie->ie_source;
  unsigned irq = ie->ie_irq;

  gtpci->imask |= (1 << irq);
  gtpci->elcr |= (1 << irq);
  gt_pci_set_icus(gtpci);
}

static void gt_pci_unmask_irq(intr_event_t *ie) {
  gt_pci_state_t *gtpci = ie->ie_source;
  unsigned irq = ie->ie_irq;

  gtpci->imask &= ~(1 << irq);
  gtpci->elcr &= ~(1 << irq);
  gt_pci_set_icus(gtpci);
}

/* clang-format off */
static const char *gt_pci_intr_name[IO_ICUSIZE] = {
  [0] = "timer",
  [1] = "kbd",        /* kbd controller (keyboard) */
  [2] = "pic-slave",  /* PIC cascade */
  [3] = "uart(1)",    /* COM 2 */
  [4] = "uart(0)",    /* COM 1 */
  [5] = "unused(0)",
  [6] = "floppy",     /* floppy */
  [7] = "parallel",   /* centronics */
  [8] = "rtc",        /* RTC */
  [9] = "i2c",        /* I2C */
  [10] = "unused(1)",
  [11] = "unused(2)",
  [12] = "mouse",     /* kbd controller (mouse) */
  [13] = "unused(3)",
  [14] = "ide(0)",    /* IDE primary */
  [15] = "ide(1)",    /* IDE secondary */
};
/* clang-format on */

static void gt_pci_setup_intr(device_t *pic, device_t *dev, dev_intr_t *intr,
                              ih_filter_t *filter, ih_service_t *service,
                              void *arg, const char *name) {
  gt_pci_state_t *gtpci = pic->state;
  unsigned irq = intr->irq;
  assert(irq < IO_ICUSIZE);

  if (gtpci->intr_event[irq] == NULL)
    gtpci->intr_event[irq] = intr_event_create(
      gtpci, irq, gt_pci_mask_irq, gt_pci_unmask_irq, gt_pci_intr_name[irq]);

  intr->handler =
    intr_event_add_handler(gtpci->intr_event[irq], filter, service, arg, name);
}

static void gt_pci_teardown_intr(device_t *pic, device_t *dev,
                                 dev_intr_t *intr) {
  intr_event_remove_handler(intr->handler);
}

static void init_8259(dev_mem_t *io, unsigned icu, unsigned imask) {
  /* reset, program device, 4 bytes */
  bus_write_1(io, ICU_ADDR(icu), ICW1_RESET | ICW1_IC4);
  bus_write_1(io, ICU_DATA(icu), 0);
  bus_write_1(io, ICU_DATA(icu), 1 << 2); /* XXX magic value ??? */
  bus_write_1(io, ICU_DATA(icu), ICW4_8086);
  /* mask all interrupts */
  bus_write_1(io, ICU_DATA(icu), imask);
  /* enable special mask mode */
  bus_write_1(io, ICU_ADDR(icu), OCW3_SEL | OCW3_ESMM | OCW3_SMM);
  /* read IRR by default */
  bus_write_1(io, ICU_ADDR(icu), OCW3_SEL | OCW3_RR);
}

static intr_filter_t gt_pci_intr(void *data) {
  gt_pci_state_t *gtpci = data;
  dev_mem_t *io = gtpci->pci_io;
  unsigned irq;

  assert(data != NULL);

  for (;;) {
    /* Handle master PIC, irq 0..7 */
    bus_write_1(io, ICU1_ADDR, OCW3_SEL | OCW3_POLL);
    irq = bus_read_1(io, ICU1_DATA);
    if ((irq & OCW3_POLL_PENDING) == 0)
      return IF_FILTERED;
    irq = OCW3_POLL_IRQ(irq);
    /* Handle slave PIC, irq 8..15 */
    if (irq == 2) {
      bus_write_1(io, ICU2_ADDR, OCW3_SEL | OCW3_POLL);
      irq = bus_read_1(io, ICU2_DATA);
      irq = (irq & OCW3_POLL_PENDING) ? (OCW3_POLL_IRQ(irq) + 8) : 2;
    }

    /* Irq 2 is used for PIC chaining, ignore it. */
    if (irq != 2)
      intr_event_run_handlers(gtpci->intr_event[irq]);

    /* Send a specific EOI to slave PIC... */
    if (irq > 7) {
      bus_write_1(io, ICU2_ADDR,
                  OCW2_SEL | OCW2_EOI | OCW2_SL | OCW2_ILS(irq & 7));
      irq = 2;
    }

    /* ... and finally to master PIC. */
    bus_write_1(io, ICU1_ADDR, OCW2_SEL | OCW2_EOI | OCW2_SL | OCW2_ILS(irq));
  }

  return IF_FILTERED;
}

DEVCLASS_DECLARE(pci);

static int gt_pci_attach(device_t *pcib) {
  gt_pci_state_t *gtpci = pcib->state;
  int err = 0;

  pcib->devclass = &DEVCLASS(pci);
  pcib->node = 1;

  gtpci->pci_mem = device_take_mem(pcib, 0);
  gtpci->pci_io = device_take_mem(pcib, 1);
  gtpci->corectrl = device_take_mem(pcib, 2);

  if (gtpci->corectrl == NULL || gtpci->pci_mem == NULL ||
      gtpci->pci_io == NULL) {
    panic("gt64120 resource allocation fail");
  }

  if ((err = bus_map_mem(pcib, gtpci->pci_io)))
    return err;
  if ((err = bus_map_mem(pcib, gtpci->corectrl)))
    return err;

  /* All interrupts default to "masked off" and edge-triggered. */
  gtpci->imask = 0xffff;
  gtpci->elcr = 0;

  /* Initialize the 8259s. */
  dev_mem_t *io = gtpci->pci_io;
  init_8259(io, IO_ICU1, LO(gtpci->imask));
  init_8259(io, IO_ICU2, HI(gtpci->imask));

  /* Default all interrupts to edge-triggered. */
  bus_write_1(io, PIIX_REG_ELCR + 0, LO(gtpci->elcr));
  bus_write_1(io, PIIX_REG_ELCR + 1, HI(gtpci->elcr));

  gtpci->irq_res = device_take_intr(pcib, 0);
  assert(gtpci->irq_res);

  if ((err = pic_setup_intr(pcib, gtpci->irq_res, gt_pci_intr, NULL, gtpci,
                            "GT64120 main irq"))) {
    /* TODO: handle the `ENODEV` case when resource unmapping is implemented. */
    panic("gt64120 interrupt setup fail");
  }

  pci_bus_enumerate(pcib);

  device_t *dev;

  /* ISA Bridge */
  dev = device_add_child(pcib, 0);
  dev->bus = DEV_BUS_PCI;
  device_add_mem(dev, 0, IO_ISABEGIN, IO_ISAEND + 1, 0);
  device_add_pending(dev);

  intr_pic_register(pcib, 1);

  return 0;
}

static int gt_pci_map_mem(device_t *dev, dev_mem_t *mem) {
  gt_pci_state_t *gtpci = dev->parent->state;
  bus_addr_t start = mem->start;

  mem->bus_tag = generic_bus_space;

  pci_device_t *pcid = pci_device_of(dev);
  if (!pcid)
    goto ioports;

  if (mem->id >= PCI_BAR_MAX)
    return EINVAL;

  pci_bar_t *bar = &pcid->bar[mem->id];
  if (!bar->size)
    return EINVAL;

  pci_res_type_t type = bar->type;

  uint16_t command = pci_read_config_2(dev, PCIR_COMMAND);
  if (type == PCI_RT_MEMORY)
    command |= PCIM_CMD_MEMEN;
  else
    command |= PCIM_CMD_PORTEN;
  pci_write_config_2(dev, PCIR_COMMAND, command);

  if (type == PCI_RT_MEMORY)
    start += MALTA_PCI0_MEMORY_BASE;

  pci_write_config_4(dev, PCIR_BAR(mem->id), start);

  if (type == PCI_RT_MEMORY)
    return bus_space_map(mem->bus_tag, start, dev_mem_size(mem),
                         &mem->bus_handle);

ioports:
  mem->bus_handle = gtpci->pci_io->bus_handle + start;
  return 0;
}

static void gt_pci_unmap_mem(device_t *dev, dev_mem_t *mem) {
  /* TODO: unmap mapped memory. */
}

static int gt_pci_probe(device_t *d) {
  return d->unit == 1;
}

static bus_methods_t gt_pci_bus_if = {
  .map_mem = gt_pci_map_mem,
  .unmap_mem = gt_pci_unmap_mem,
};

static pic_methods_t gt_pic_if = {
  .setup_intr = gt_pci_setup_intr,
  .teardown_intr = gt_pci_teardown_intr,
};

static pci_bus_methods_t gt_pci_pci_bus_if = {
  .read_config = gt_pci_read_config,
  .write_config = gt_pci_write_config,
  .route_interrupt = gt_pci_route_interrupt,
  .enable_busmaster = gt_pci_enable_busmaster,
};

static driver_t gt_pci_bus = {
  .desc = "GT-64120 PCI bus driver",
  .size = sizeof(gt_pci_state_t),
  .pass = FIRST_PASS,
  .attach = gt_pci_attach,
  .probe = gt_pci_probe,
  .interfaces =
    {
      [DIF_BUS] = &gt_pci_bus_if,
      [DIF_PIC] = &gt_pic_if,
      [DIF_PCI_BUS] = &gt_pci_pci_bus_if,
    },
};

DEVCLASS_ENTRY(root, gt_pci_bus);
