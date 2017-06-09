#define KL_LOG KL_DEV
#include <stdc.h>
#include <vnode.h>
#include <devfs.h>
#include <klog.h>
#include <condvar.h>
#include <malloc.h>
#include <ringbuf.h>
#include <pci.h>
#include <dev/isareg.h>
#include <dev/atkbdcreg.h>
#include <interrupt.h>
#include <sysinit.h>

#define KBD_DATA (IO_KBD + KBD_DATA_PORT)
#define KBD_STATUS (IO_KBD + KBD_STATUS_PORT)
#define KBD_COMMAND (IO_KBD + KBD_COMMAND_PORT)

#define KBD_BUFSIZE 128

typedef struct atkbdc_state {
  mtx_t mtx;
  condvar_t nonempty;
  ringbuf_t scancodes;
  intr_handler_t intr_handler;
  resource_t *regs;
} atkbdc_state_t;

/* For now, this is the only keyboard driver we'll want to have, so the
   interface is not very flexible. */

/* NOTE: These blocking wait helper functions can't use an interrupt, as the
   PS/2 controller does not generate interrupts for these events. However, this
   is not a major problem, since pretty much always the controller is
   immediately ready to proceed, so the we don't loop in practice. */
static inline void wait_before_read(resource_t *regs) {
  while (!(bus_space_read_1(regs, KBD_STATUS) & KBDS_KBD_BUFFER_FULL))
    ;
}

static inline void wait_before_write(resource_t *regs) {
  while (bus_space_read_1(regs, KBD_STATUS) & KBDS_INPUT_BUFFER_FULL)
    ;
}

static void write_command(resource_t *regs, uint8_t command) {
  wait_before_write(regs);
  bus_space_write_1(regs, KBD_COMMAND, command);
}

static uint8_t read_data(resource_t *regs) {
  wait_before_read(regs);
  return bus_space_read_1(regs, KBD_DATA);
}

static void write_data(resource_t *regs, uint8_t byte) {
  wait_before_write(regs);
  bus_space_write_1(regs, KBD_DATA, byte);
}

static int scancode_read(vnode_t *v, uio_t *uio) {
  atkbdc_state_t *atkbdc = v->v_data;
  int error;

  uio->uio_offset = 0; /* This device does not support offsets. */

  WITH_MTX_LOCK (&atkbdc->mtx) {
    uint8_t data;
    /* For simplicity, copy to the user space one byte at a time. */
    while (!ringbuf_getb(&atkbdc->scancodes, &data))
      cv_wait(&atkbdc->nonempty, &atkbdc->mtx);
    if ((error = uiomove_frombuf(&data, 1, uio)))
      return error;
  }

  return 0;
}

static vnodeops_t dev_scancode_ops = {
  .v_open = vnode_open_generic, .v_read = scancode_read,
};

/* Reset keyboard and perform a self-test. */
static bool kbd_reset(resource_t *regs) {
  write_data(regs, KBDC_RESET_KBD);
  uint8_t response = read_data(regs);
  if (response != KBD_ACK)
    return false;
  return (read_data(regs) == KBD_RESET_DONE);
}

static intr_filter_t atkbdc_intr(void *data) {
  atkbdc_state_t *atkbdc = data;

  if (!(bus_space_read_1(atkbdc->regs, KBD_STATUS) & KBDS_KBD_BUFFER_FULL))
    return IF_STRAY;

  /* TODO: Some locking mechanism will be necessary if we want to sent extra
     commands to the ps/2 controller and/or other ps/2 devices, because this
     thread would interfere with commands/responses. */
  uint8_t code = read_data(atkbdc->regs);
  uint8_t code2 = 0;
  bool extended = (code == 0xe0);

  if (extended)
    code2 = read_data(atkbdc->regs);

  WITH_MTX_LOCK (&atkbdc->mtx) {
    /* TODO: There's no logic for processing scancodes. */
    ringbuf_putb(&atkbdc->scancodes, code);
    if (extended)
      ringbuf_putb(&atkbdc->scancodes, code2);
    cv_signal(&atkbdc->nonempty);
  }

  return IF_FILTERED;
}

static int atkbdc_probe(device_t *dev) {
  assert(dev->parent->bus == DEV_BUS_PCI);
  pci_bus_state_t *pcib = dev->parent->state;
  resource_t *regs = pcib->io_space;

  if (!kbd_reset(regs)) {
    klog("Keyboard self-test failed.");
    return 0;
  }

  /* Enable scancode table #1 */
  write_data(regs, KBDC_SET_SCANCODE_SET);
  write_data(regs, 1);
  if (read_data(regs) != KBD_ACK)
    return 0;

  /* Start scanning. */
  write_data(regs, KBDC_ENABLE_KBD);
  if (read_data(regs) != KBD_ACK)
    return 0;

  return 1;
}

static int atkbdc_attach(device_t *dev) {
  assert(dev->parent->bus == DEV_BUS_PCI);

  pci_bus_state_t *pcib = dev->parent->state;
  atkbdc_state_t *atkbdc = dev->state;

  atkbdc->scancodes.data = kmalloc(M_DEV, KBD_BUFSIZE, M_ZERO);
  atkbdc->scancodes.size = KBD_BUFSIZE;

  mtx_init(&atkbdc->mtx, MTX_DEF);
  cv_init(&atkbdc->nonempty, "AT keyboard buffer non-empty");
  atkbdc->regs = pcib->io_space;

  atkbdc->intr_handler =
    INTR_HANDLER_INIT(atkbdc_intr, NULL, atkbdc, "AT keyboard controller", 0);
  bus_intr_setup(dev, 1, &atkbdc->intr_handler);

  /* Enable interrupt */
  write_command(atkbdc->regs, KBDC_SET_COMMAND_BYTE);
  write_data(atkbdc->regs, KBD_ENABLE_KBD_INT);

  /* Prepare /dev/scancode interface. */
  devfs_install("scancode", V_DEV, &dev_scancode_ops, atkbdc);

  return 0;
}

static driver_t atkbdc_driver = {
  .desc = "AT keyboard controller driver",
  .size = sizeof(atkbdc_state_t),
  .probe = atkbdc_probe,
  .attach = atkbdc_attach,
};

extern device_t *gt_pci;

static void atkbdc_init(void) {
  vnodeops_init(&dev_scancode_ops);
  (void)make_device(gt_pci, &atkbdc_driver);
}

SYSINIT_ADD(atkbdc, atkbdc_init, DEPS("rootdev"));
