/* GT64120 PCI bus driver
 *
 * Heavily inspired by FreeBSD / NetBSD `gt_pci.c` file.
 *
 * How do we handle `resource_start(r)` and `r_bus_handle`
 * of assigned resources?
 *
 * - Interrupts:
 *     In this case, the `r_handler` field is used rather than
 *     `r_bus_handle` and `r_bus_tag`, and `start` identifies an
 *     interrupt number.
 *
 * - Memory:
 *     - `gt_pci_alloc_resource` sets `r_bus_handle` to a physical
 *       address of a resource, while `gt_pci_activate_resource`
 *       upgrades it to a virtual address of the mapped resource.
 *     - `start` is an absolute address of a resource.
 *
 *  - IO ports:
 *     - All IO ports managed by the PCI bus driver are mapped in the
 *       `gt_pci_attach` function, therefore `gt_pci_alloc_resource`
 *       sets `r_bus_handle` to a virtual address of a resource.
 *     - `start` is an offset in the PCI IO space.
 *
 *   Memory BARs must contain absolute addresses, while IO BARs require
 *   relative addresses. The above scheme allows us to unify updating of a
 *   BAR register by using the `start` of a resource.
 */
#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <dev/malta.h>
#include <dev/i8259.h>
#include <dev/piixreg.h>
#include <dev/isareg.h>
#include <dev/gt64120reg.h>
#include <dev/pci.h>
#include <sys/spinlock.h>
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
  resource_t *corectrl;
  resource_t *pci_io;
  resource_t *pci_mem;
  resource_t *irq_res;

  /* Resource managers which manage resources used by child devices. */
  rman_t pci_io_rman;
  rman_t pci_mem_rman;
  rman_t irq_rman;

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
  resource_t *pcicfg = gtpci->corectrl;
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
  resource_t *pcicfg = gtpci->corectrl;
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

  resource_t *io = gtpci->pci_io;
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

static void gt_pci_intr_setup(device_t *dev, resource_t *r, ih_filter_t *filter,
                              ih_service_t *service, void *arg,
                              const char *name) {
  assert(dev->parent->driver == &gt_pci_bus);
  gt_pci_state_t *gtpci = dev->parent->state;
  int irq = resource_start(r);
  assert(irq < IO_ICUSIZE);

  if (gtpci->intr_event[irq] == NULL)
    gtpci->intr_event[irq] = intr_event_create(
      gtpci, irq, gt_pci_mask_irq, gt_pci_unmask_irq, gt_pci_intr_name[irq]);

  r->r_handler =
    intr_event_add_handler(gtpci->intr_event[irq], filter, service, arg, name);
}

static void gt_pci_intr_teardown(device_t *pcib, resource_t *irq) {
  assert(pcib->parent->driver == &gt_pci_bus);

  intr_event_remove_handler(irq->r_handler);
}

static void init_8259(resource_t *io, unsigned icu, unsigned imask) {
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
  resource_t *io = gtpci->pci_io;
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

DEVCLASS_DECLARE(isa);

static int gt_pci_attach(device_t *pcib) {
  gt_pci_state_t *gtpci = pcib->state;

  gtpci->pci_mem = device_take_memory(pcib, 0, 0);
  gtpci->pci_io = device_take_memory(pcib, 1, RF_ACTIVE);
  gtpci->corectrl = device_take_memory(pcib, 2, RF_ACTIVE);

  if (gtpci->corectrl == NULL || gtpci->pci_mem == NULL ||
      gtpci->pci_io == NULL) {
    panic("gt64120 resource allocation fail");
  }

  rman_init(&gtpci->pci_io_rman, "GT64120 PCI I/O ports");
  rman_manage_region(&gtpci->pci_io_rman, 0, 0x10000);

  /* This will ensure absolute addresses which is essential
   * in order to satisfy memory alignment. */
  rman_init_from_resource(&gtpci->pci_mem_rman, "GT64120 PCI memory",
                          gtpci->pci_mem);

  rman_init(&gtpci->irq_rman, "GT64120 PCI & ISA interrupts");
  rman_manage_region(&gtpci->irq_rman, 0, IO_ICUSIZE);

  /* All interrupts default to "masked off" and edge-triggered. */
  gtpci->imask = 0xffff;
  gtpci->elcr = 0;

  /* Initialize the 8259s. */
  resource_t *io = gtpci->pci_io;
  init_8259(io, IO_ICU1, LO(gtpci->imask));
  init_8259(io, IO_ICU2, HI(gtpci->imask));

  /* Default all interrupts to edge-triggered. */
  bus_write_1(io, PIIX_REG_ELCR + 0, LO(gtpci->elcr));
  bus_write_1(io, PIIX_REG_ELCR + 1, HI(gtpci->elcr));

  gtpci->irq_res = device_take_irq(pcib, 0, RF_ACTIVE);
  bus_intr_setup(pcib, gtpci->irq_res, gt_pci_intr, NULL, gtpci,
                 "GT64120 main irq");

  pci_bus_enumerate(pcib);

  device_t *dev;

  /* ISA Bridge */
  dev = device_add_child(pcib, 0);
  dev->bus = DEV_BUS_PCI;
  dev->devclass = &DEVCLASS(isa);
  device_add_ioports(dev, 0, IO_ISABEGIN, IO_ISAEND + 1);

  /* TODO: replace raw resource assignments by parsing FDT file. */

  return bus_generic_probe(pcib);
}

static bool gt_pci_bar(device_t *dev, res_type_t type, int rid,
                       rman_addr_t start) {
  if ((type == RT_IOPORTS && start <= IO_ISAEND) || type == RT_IRQ)
    return false;
  pci_device_t *pcid = pci_device_of(dev);
  return rid < PCI_BAR_MAX && pcid->bar[rid].size != 0;
}

static resource_t *gt_pci_alloc_resource(device_t *dev, res_type_t type,
                                         int rid, rman_addr_t start,
                                         rman_addr_t end, size_t size,
                                         rman_flags_t flags) {
  assert(dev->bus == DEV_BUS_PCI);

  device_t *pcib = dev->parent;
  gt_pci_state_t *gtpci = pcib->state;
  bus_space_handle_t bh = 0;
  size_t alignment = 0;
  rman_t *rman = NULL;

  if (type == RT_IOPORTS) {
    rman = &gtpci->pci_io_rman;
    bh = gtpci->pci_io->r_bus_handle;
  } else if (type == RT_IRQ) {
    rman = &gtpci->irq_rman;
  } else if (type == RT_MEMORY) {
    alignment = PAGESIZE;
    rman = &gtpci->pci_mem_rman;
  } else {
    panic("Unknown PCI device type: %d", type);
  }

  if (gt_pci_bar(dev, type, rid, start))
    alignment = max(alignment, size);

  resource_t *r =
    rman_reserve_resource(rman, type, rid, start, end, size, alignment, flags);
  if (!r)
    return NULL;

  if (type != RT_IRQ) {
    r->r_bus_tag = generic_bus_space;
    r->r_bus_handle = bh + resource_start(r);
  }

  if (type == RT_IOPORTS || flags & RF_ACTIVE) {
    if (bus_activate_resource(dev, r)) {
      resource_release(r);
      return NULL;
    }
  }

  return r;
}

static int gt_pci_activate_resource(device_t *dev, resource_t *r) {
  rman_addr_t start = resource_start(r);

  if (r->r_type == RT_MEMORY ||
      (r->r_type == RT_IOPORTS && start > IO_ISAEND)) {
    uint16_t command = pci_read_config_2(dev, PCIR_COMMAND);
    if (r->r_type == RT_MEMORY)
      command |= PCIM_CMD_MEMEN;
    else if (r->r_type == RT_IOPORTS)
      command |= PCIM_CMD_PORTEN;
    pci_write_config_2(dev, PCIR_COMMAND, command);
  }

  if (gt_pci_bar(dev, r->r_type, r->r_rid, start))
    pci_write_config_4(dev, PCIR_BAR(r->r_rid), start);

  if (r->r_type == RT_MEMORY)
    return bus_space_map(r->r_bus_tag, start, resource_size(r),
                         &r->r_bus_handle);

  return 0;
}

static int gt_pci_probe(device_t *d) {
  /* TODO(cahir) match device with driver on FDT basis */
  return d->unit == 1;
}

static bus_methods_t gt_pci_bus_if = {
  .intr_setup = gt_pci_intr_setup,
  .intr_teardown = gt_pci_intr_teardown,
  .alloc_resource = gt_pci_alloc_resource,
  .activate_resource = gt_pci_activate_resource,
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
      [DIF_PCI_BUS] = &gt_pci_pci_bus_if,
    },
};

DEVCLASS_ENTRY(root, gt_pci_bus);
