/* Heavily inspired by FreeBSD / NetBSD `gt_pci.c` file. */

#define KL_LOG KL_DEV
#include <mips/gt64120.h>
#include <dev/i8259.h>
#include <dev/piixreg.h>
#include <dev/isareg.h>
#include <interrupt.h>
#include <pci.h>
#include <stdc.h>
#include <klog.h>
#include <sync.h>

#define PCI0_CFG_ADDR_R GT_R(GT_PCI0_CFG_ADDR)
#define PCI0_CFG_DATA_R GT_R(GT_PCI0_CFG_DATA)

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

typedef struct gt_pci_state {
  pci_bus_state_t pci_bus;

  intr_chain_t intr_chain[16];

  uint16_t imask;
  uint16_t elcr;
} gt_pci_state_t;

/* Access configuration space through memory mapped GT-64120 registers. Take
 * care of the fact that MIPS processor cannot handle unaligned accesses. */
static uint32_t gt_pci_read_config(device_t *dev, unsigned reg, unsigned size) {
  pci_device_t *pcid = pci_device_of(dev);

  if (pcid->addr.bus > 0)
    return -1;

  PCI0_CFG_ADDR_R = PCI0_CFG_ENABLE | gt_pci_make_addr(pcid->addr, reg >> 2);
  pci_reg_t data = (pci_reg_t)PCI0_CFG_DATA_R;
  reg &= 3;
  switch (size) {
    case 1:
      return data.byte[3 - reg];
    case 2:
      return data.word[1 - (reg >> 1)];
    case 4:
      return data.dword;
    default:
      return -1;
  }
}

static void gt_pci_write_config(device_t *dev, unsigned reg, unsigned size,
                                uint32_t value) {
  pci_device_t *pcid = pci_device_of(dev);

  if (pcid->addr.bus > 0)
    return;

  PCI0_CFG_ADDR_R = PCI0_CFG_ENABLE | gt_pci_make_addr(pcid->addr, reg >> 2);
  pci_reg_t data = (pci_reg_t)PCI0_CFG_DATA_R;
  reg &= 3;
  switch (size) {
    case 1:
      data.byte[3 - reg] = value;
    case 2:
      data.word[1 - (reg >> 1)] = value;
    case 4:
      data.dword = value;
    default:
      break;
  }
  PCI0_CFG_DATA_R = data.dword;
}

static uint8_t gt_pci_read_1(resource_t *handle, unsigned offset) {
  intptr_t addr = handle->r_start + offset;
  return *(volatile uint8_t *)MIPS_PHYS_TO_KSEG1(addr);
}

static void gt_pci_write_1(resource_t *handle, unsigned offset, uint8_t value) {
  intptr_t addr = handle->r_start + offset;
  *(volatile uint8_t *)MIPS_PHYS_TO_KSEG1(addr) = value;
}

static uint16_t gt_pci_read_2(resource_t *handle, unsigned offset) {
  intptr_t addr = handle->r_start + offset;
  return *(volatile uint16_t *)MIPS_PHYS_TO_KSEG1(addr);
}

static void gt_pci_write_2(resource_t *handle, unsigned offset,
                           uint16_t value) {
  intptr_t addr = handle->r_start + offset;
  *(volatile uint16_t *)MIPS_PHYS_TO_KSEG1(addr) = value;
}

static void gt_pci_read_region_1(resource_t *handle, unsigned offset,
                                 uint8_t *dst, size_t count) {
  uint8_t *src = (uint8_t *)MIPS_PHYS_TO_KSEG1(handle->r_start + offset);
  for (size_t i = 0; i < count; i++)
    *dst++ = *src++;
}

static void gt_pci_write_region_1(resource_t *handle, unsigned offset,
                                  const uint8_t *src, size_t count) {
  uint8_t *dst = (uint8_t *)MIPS_PHYS_TO_KSEG1(handle->r_start + offset);
  for (size_t i = 0; i < count; i++)
    *dst++ = *src++;
}

static bus_space_t gt_pci_bus_space = {.read_1 = gt_pci_read_1,
                                       .write_1 = gt_pci_write_1,
                                       .read_2 = gt_pci_read_2,
                                       .write_2 = gt_pci_write_2,
                                       .read_region_1 = gt_pci_read_region_1,
                                       .write_region_1 = gt_pci_write_region_1};

static resource_t gt_pci_memory = {.r_bus_space = &gt_pci_bus_space,
                                   .r_type = RT_MEMORY,
                                   .r_start = MALTA_PCI0_MEMORY_BASE,
                                   .r_end = MALTA_PCI0_MEMORY_END};

static resource_t gt_pci_ioports = {.r_bus_space = &gt_pci_bus_space,
                                    .r_type = RT_IOPORTS,
                                    .r_start = MALTA_PCI0_IO_BASE,
                                    .r_end = MALTA_PCI0_IO_END};

static void gt_pci_set_icus(gt_pci_state_t *gtpci) {
  /* Enable the cascade IRQ (2) if 8-15 is enabled. */
  if ((gtpci->imask & 0xff00) != 0xff00)
    gtpci->imask &= ~(1U << 2);
  else
    gtpci->imask |= (1U << 2);

  resource_t *io = gtpci->pci_bus.io_space;
  bus_space_write_1(io, IO_ICU1 + 1, LO(gtpci->imask));
  bus_space_write_1(io, IO_ICU2 + 1, HI(gtpci->imask));
  bus_space_write_1(io, PIIX_REG_ELCR + 0, LO(gtpci->elcr));
  bus_space_write_1(io, PIIX_REG_ELCR + 1, HI(gtpci->elcr));
}

static void gt_pci_mask_irq(gt_pci_state_t *gtpci, unsigned irq) {
  gtpci->imask |= (1 << irq);
  gtpci->elcr |= (1 << irq);
  gt_pci_set_icus(gtpci);
}

static void gt_pci_unmask_irq(gt_pci_state_t *gtpci, unsigned irq) {
  gtpci->imask &= ~(1 << irq);
  gtpci->elcr &= ~(1 << irq);
  gt_pci_set_icus(gtpci);
}

static void gt_pci_intr_setup(device_t *pcib, unsigned irq,
                              intr_handler_t *handler) {
  klog("gt_pci_intr_setup(%p, %d, %p)", pcib, irq, handler);

  gt_pci_state_t *gtpci = pcib->state;
  intr_chain_t *chain = &gtpci->intr_chain[irq];
  CRITICAL_SECTION {
    intr_chain_add_handler(chain, handler);
    if (chain->ic_count == 1)
      gt_pci_unmask_irq(gtpci, irq);
  }
}

static void gt_pci_intr_teardown(device_t *pcib, intr_handler_t *handler) {
  klog("gt_pci_intr_teardown(%p, %p)", pcib, handler);

  gt_pci_state_t *gtpci = pcib->state;
  intr_chain_t *chain = handler->ih_chain;
  CRITICAL_SECTION {
    if (chain->ic_count == 1)
      gt_pci_mask_irq(gtpci, chain->ic_irq);
    intr_chain_remove_handler(handler);
  }
}

static void init_8259(resource_t *io, unsigned icu, unsigned imask) {
  /* reset, program device, 4 bytes */
  bus_space_write_1(io, icu + 0, ICW1_RESET | ICW1_IC4);
  bus_space_write_1(io, icu + 1, 0);
  bus_space_write_1(io, icu + 1, 1 << 2);
  bus_space_write_1(io, icu + 1, ICW4_8086);
  /* mask all interrupts */
  bus_space_write_1(io, icu + 1, imask);
  /* enable special mask mode */
  bus_space_write_1(io, icu + 0, OCW3_SEL | OCW3_ESMM | OCW3_SMM);
  /* read IRR by default */
  bus_space_write_1(io, icu + 0, OCW3_SEL | OCW3_RR);
}

#define ICU1_ADDR_R *(volatile uint8_t *)(MIPS_PHYS_TO_KSEG1(MALTA_ICU1_ADDR))
#define ICU1_DATA_R *(volatile uint8_t *)(MIPS_PHYS_TO_KSEG1(MALTA_ICU1_DATA))

#define ICU2_ADDR_R *(volatile uint8_t *)(MIPS_PHYS_TO_KSEG1(MALTA_ICU2_ADDR))
#define ICU2_DATA_R *(volatile uint8_t *)(MIPS_PHYS_TO_KSEG1(MALTA_ICU2_DATA))

#define OCW3_POLL_IRQ(x) ((x)&0x7f)
#define OCW3_POLL_PENDING (1U << 7)

static intr_filter_t gt_pci_intr(void *data) {
  unsigned irq;

  for (;;) {
    ICU1_ADDR_R = OCW3_SEL | OCW3_P;
    irq = ICU1_DATA_R;
    if ((irq & OCW3_POLL_PENDING) == 0)
      return IF_HANDLED;
    irq = OCW3_POLL_IRQ(irq);
    /* slave PIC ? */
    if (irq == 2) {
      ICU2_ADDR_R = OCW3_SEL | OCW3_P;
      irq = ICU2_DATA_R;
      if (irq & OCW3_POLL_PENDING)
        irq = OCW3_POLL_IRQ(irq) + 8;
      else
        irq = 2;
    }

    /* Send a specific EOI to the 8259. */
    if (irq > 7) {
      ICU2_ADDR_R = OCW2_SEL | OCW2_EOI | OCW2_SL | OCW2_ILS(irq & 7);
      irq = 2;
    }

    ICU1_ADDR_R = OCW2_SEL | OCW2_EOI | OCW2_SL | OCW2_ILS(irq);
  }

  return IF_HANDLED;
}

INTR_HANDLER_DEFINE(gt_pci_intr_handler, gt_pci_intr, NULL, NULL,
                    "GT64120 interrupt", 0);

static int gt_pci_attach(device_t *pcib) {
  gt_pci_state_t *gtpci = pcib->state;
  gtpci->pci_bus.mem_space = &gt_pci_memory;
  gtpci->pci_bus.io_space = &gt_pci_ioports;

  /* All interrupts default to "masked off" and edge-triggered. */
  gtpci->imask = 0xffff;
  gtpci->elcr = 0;

  /* Initialize the 8259s. */
  resource_t *io = gtpci->pci_bus.io_space;
  init_8259(io, IO_ICU1, LO(gtpci->imask));
  init_8259(io, IO_ICU2, HI(gtpci->imask));

  /* Default all interrupts to edge-triggered. */
  bus_space_write_1(io, PIIX_REG_ELCR + 0, LO(gtpci->elcr));
  bus_space_write_1(io, PIIX_REG_ELCR + 1, HI(gtpci->elcr));

  intr_chain_t *chain = gtpci->intr_chain;
  intr_chain_init(&chain[0], 0, "timer");     /* timer */
  intr_chain_init(&chain[1], 1, "kbd");       /* kbd controller (keyboard) */
  intr_chain_init(&chain[2], 2, "pic-slave"); /* PIC cascade */
  intr_chain_init(&chain[3], 3, "uart(1)");   /* COM 2 */
  intr_chain_init(&chain[4], 4, "uart(0)");   /* COM 1 */
  intr_chain_init(&chain[5], 5, "unused(0)");
  intr_chain_init(&chain[6], 6, "floppy");   /* floppy */
  intr_chain_init(&chain[7], 7, "parallel"); /* centronics */
  intr_chain_init(&chain[8], 8, "rtc");      /* RTC */
  intr_chain_init(&chain[9], 9, "i2c");      /* I2C */
  intr_chain_init(&chain[10], 10, "unused(1)");
  intr_chain_init(&chain[11], 11, "unused(2)");
  intr_chain_init(&chain[12], 12, "mouse"); /* kbd controller (mouse) */
  intr_chain_init(&chain[13], 13, "unused(3)");
  intr_chain_init(&chain[14], 14, "ide(0)"); /* IDE primary */
  intr_chain_init(&chain[15], 15, "ide(1)"); /* IDE secondary */

  pci_bus_enumerate(pcib);
  pci_bus_assign_space(pcib);
  pci_bus_dump(pcib);

  bus_intr_setup(pcib, 2, gt_pci_intr_handler);

  gt_pci_unmask_irq(gtpci, 8);

  return bus_generic_probe(pcib);
}

pci_bus_driver_t gt_pci = {
  .driver =
    {
      .desc = "GT-64120 PCI bus driver",
      .size = sizeof(gt_pci_state_t),
      .attach = gt_pci_attach,
    },
  .bus =
    {
      .intr_setup = gt_pci_intr_setup, .intr_teardown = gt_pci_intr_teardown,
    },
  .pci_bus =
    {
      .read_config = gt_pci_read_config, .write_config = gt_pci_write_config,
    }};
