#define KL_LOG KL_DEV
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/fdt.h>
#include <sys/interrupt.h>
#include <sys/klog.h>

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
  pcell_t *intr;
  int err = 0;
  int icells;
  bool extended;

  if ((err = FDT_intr_cells(iparent, &icells)))
    return err;

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

  for (int i = 0; i < nintrs; i += icells) {
    if (extended) {
      /* XXX: FTTB, we assume a single interrupt parent. */
      if (intr[i++] != iparent) {
        err = ENXIO;
        goto end;
      }
      if ((i + icells) > nintrs) {
        err = ERANGE;
        goto end;
      }
    }
    int irqnum = pic_map_intr(dev, &intr[i], icells);
    if (irqnum >= 0)
      device_add_irq(dev, i, irqnum);
  }

end:
  FDT_free(intr);
  return err;
}

device_t *simplebus_add_child(device_t *bus, phandle_t node, int unit,
                              device_t *pic) {
  device_t *dev = device_add_child(bus, unit);
  dev->node = node;
  dev->pic = pic;

  sb_reg_to_rl(dev);
  sb_intr_to_rl(dev);

  int err = 0;
  if (FDT_hasprop(node, "interrupt-controller"))
    if ((err = bus_generic_probe(bus))) {
      device_remove_child(bus, dev);
      dev = NULL;
    }

  return dev;
}
