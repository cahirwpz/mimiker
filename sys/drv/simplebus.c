#define KL_LOG KL_DEV
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/fdt.h>
#include <sys/interrupt.h>
#include <sys/klog.h>

/* TODO: handle ranges property. */
static int sb_reg_to_rl(device_t *dev) {
  fdt_mem_reg_t mrs[FDT_MAX_REG_TUPLES];
  size_t cnt;

  int err = FDT_get_reg(dev->node, mrs, &cnt);
  if (err)
    return err;

  for (size_t i = 0; i < cnt; i++)
    device_add_memory(dev, i, mrs[i].addr, mrs[i].size);

  return 0;
}

static int sb_intr_to_rl(device_t *dev) {
  device_t *pic = dev->pic;
  assert(pic);
  assert(pic->driver);

  phandle_t node = dev->node;
  phandle_t iparent = pic->node;
  int err = 0;
  int icells;

  if ((err = FDT_intr_cells(iparent, &icells)))
    return err;

  bool extended;
  pcell_t *intr;
  int nintrs = FDT_getencprop_alloc_multi(node, "interrupts", sizeof(pcell_t),
                                          (void **)&intr);
  if (nintrs > 0) {
    if (icells > nintrs) {
      err = ERANGE;
      goto end;
    }
    extended = false;
  } else {
    nintrs = FDT_getencprop_alloc_multi(node, "interrupts-extended",
                                        sizeof(pcell_t), (void **)&intr);
    if (nintrs <= 0)
      return 0;
    extended = true;
  }

  int rid = 0;
  for (int i = 0; i < nintrs; i += icells) {
    if (extended) {
      /* XXX: FTTB, we assume a single interrupt parent. */
      i++;
      if ((i + icells) > nintrs) {
        err = ERANGE;
        goto end;
      }
    }
    int irqnum = pic_map_intr(dev, &intr[i], icells);
    if (irqnum >= 0)
      device_add_irq(dev, rid++, irqnum);
  }

end:
  FDT_free(intr);
  return err;
}

int simplebus_add_child(device_t *bus, phandle_t node, int unit, device_t *pic,
                        device_t **devp) {
  device_t *dev = device_add_child(bus, unit);
  dev->node = node;
  dev->pic = pic;

  int err;

  if ((err = sb_reg_to_rl(dev)))
    goto end;
  if ((err = sb_intr_to_rl(dev)))
    goto end;

  if (FDT_hasprop(node, "interrupt-controller")) {
    if ((err = bus_generic_probe(bus)))
      goto end;
  }

  if (devp)
    *devp = dev;
end:
  if (err)
    device_remove_child(bus, dev);
  return err;
}
