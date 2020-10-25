#define KL_LOG KL_DEV
#include <sys/klog.h>
#include <sys/bus.h>
#include <sys/devclass.h>
#include <aarch64/armreg.h>
#include <aarch64/bcm2835reg.h>
#include <aarch64/interrupt.h>
#include <sys/kmem.h>
#include <sys/pmap.h>
#include <sys/timer.h>
#include <sys/interrupt.h>

/*
 * TODO(pj): This is only temporary solution. Timer should be initialized by
 * device subsystem.
 */

#define CNTCTL_ENABLE 1

static int timer_start(timer_t *tm, unsigned flags, const bintime_t start,
                       const bintime_t period) {
  uint64_t step = bintime_mul(period, tm->tm_frequency).sec;

  WITH_INTR_DISABLED {
    uint64_t count = READ_SPECIALREG(cntpct_el0);
    WRITE_SPECIALREG(cntp_cval_el0, count + step);
    WRITE_SPECIALREG(cntp_ctl_el0, CNTCTL_ENABLE);

    /*
     * TODO(pj): This should be defined in one header for machine dependent
     * AArch64 code.
     */
    __asm__ volatile("isb" ::: "memory");
  }

  return 0;
}

static int timer_stop(timer_t *tm) {
  WRITE_SPECIALREG(cntp_ctl_el0, 0);
  return 0;
}

static bintime_t timer_gettime(timer_t *tm) {
  uint64_t count = READ_SPECIALREG(cntpct_el0);
  bintime_t val = bintime_mul(tm->tm_min_period, count);
  return val;
}

static timer_t timer = (timer_t){
  .tm_name = "arm-cpu-timer",
  .tm_flags = TMF_PERIODIC,
  .tm_start = timer_start,
  .tm_stop = timer_stop,
  .tm_gettime = timer_gettime,
  .tm_priv = NULL,
};

static vaddr_t va;

void intr_tick(void) {
  tm_trigger(&timer);

  /*
   * https://developer.arm.com/docs/ddi0595/h/aarch64-system-registers/cntp_cval_el0
   */
  uint64_t count = READ_SPECIALREG(cntpct_el0);
  uint64_t freq = READ_SPECIALREG(cntfrq_el0);
  WRITE_SPECIALREG(cntp_cval_el0, count + freq);

  __asm__ volatile("isb" ::: "memory");
}

static void _init_clock(void) {
  /* TODO(pj): This memory should be managed by bus_space_map. */
  va = kva_alloc(PAGESIZE);
  assert((void *)va != NULL);

  paddr_t pa = BCM2836_ARM_LOCAL_BASE;
  pmap_kenter(va, pa, VM_PROT_READ | VM_PROT_WRITE, PMAP_NOCACHE);

  size_t offset = BCM2836_LOCAL_TIMER_IRQ_CONTROLN(0);
  volatile uint32_t *timerp = (uint32_t *)(va + offset);
  *timerp = *timerp | (1 << BCM2836_INT_CNTPNSIRQ);
}

void init_arm_timer(void) {
  uint64_t freq = READ_SPECIALREG(cntfrq_el0);

  timer.tm_frequency = freq;
  timer.tm_min_period = HZ2BT(freq);
  /* TODO(pj): Document what this value means. */
  timer.tm_max_period = bintime_mul(HZ2BT(freq), 1LL << 30);
  tm_register(&timer);
  tm_select(&timer);
}

typedef struct rootdev {
} rootdev_t;

static vaddr_t rootdev_local_handle;
static rman_t rm_mem[2];

/* Use pre mapped memory. */
static int rootdev_bs_map(bus_addr_t addr, bus_size_t size,
                          bus_space_handle_t *handle_p) {
  if (addr >= BCM2836_ARM_LOCAL_BASE &&
      addr < BCM2836_ARM_LOCAL_BASE + BCM2836_ARM_LOCAL_SIZE) {
    *handle_p = rootdev_local_handle;
    return 0;
  }
  return generic_bs_map(addr, size, handle_p);
}

/* clang-format off */
static bus_space_t *rootdev_bus_space = &(bus_space_t){
  .bs_map = rootdev_bs_map,
  .bs_read_1 = generic_bs_read_1,
  .bs_read_2 = generic_bs_read_2,
  .bs_read_4 = generic_bs_read_4,
  .bs_write_1 = generic_bs_write_1,
  .bs_write_2 = generic_bs_write_2,
  .bs_write_4 = generic_bs_write_4,
  .bs_read_region_1 = generic_bs_read_region_1,
  .bs_read_region_2 = generic_bs_read_region_2,
  .bs_read_region_4 = generic_bs_read_region_4,
  .bs_write_region_1 = generic_bs_write_region_1,
  .bs_write_region_2 = generic_bs_write_region_2,
  .bs_write_region_4 = generic_bs_write_region_4,
};
/* clang-format on */

static void rootdev_intr_setup(device_t *dev, unsigned num,
                               intr_handler_t *handler) {
  aarch64_intr_setup(handler, num);
}

static void rootdev_intr_teardown(device_t *dev, intr_handler_t *handler) {
  aarch64_intr_teardown(handler);
}

static int rootdev_attach(device_t *bus) {
  rman_init(&rm_mem[0], "BCM2835 peripherials", BCM2835_PERIPHERALS_BASE,
            BCM2835_PERIPHERALS_BASE + BCM2835_PERIPHERALS_SIZE - 1, RT_MEMORY);
  rman_init(&rm_mem[1], "ARM local", BCM2836_ARM_LOCAL_BASE,
            BCM2836_ARM_LOCAL_BASE + BCM2836_ARM_LOCAL_SIZE - 1, RT_MEMORY);

  /* Map BCM2836 shared processo only once. */
  rootdev_local_handle = kva_alloc(BCM2836_ARM_LOCAL_SIZE);
  pmap_kenter(rootdev_local_handle, BCM2836_ARM_LOCAL_BASE,
              VM_PROT_READ | VM_PROT_WRITE, PMAP_NOCACHE);
  return 0;
}

static resource_t *rootdev_alloc_resource(device_t *bus, device_t *child,
                                          res_type_t type, int rid,
                                          rman_addr_t start, rman_addr_t end,
                                          size_t size, res_flags_t flags) {
  resource_t *r;

  r = rman_alloc_resource(&rm_mem[0], start, end, size, 1, RF_NONE, child);
  if (r == NULL)
    r = rman_alloc_resource(&rm_mem[1], start, end, size, 1, RF_NONE, child);

  if (r) {
    r->r_bus_tag = rootdev_bus_space;
    if (flags & RF_ACTIVE)
      bus_space_map(r->r_bus_tag, r->r_start, r->r_end - r->r_start + 1,
                    &r->r_bus_handle);
    device_add_resource(child, r, rid);
  }

  return r;
}

static bus_driver_t rootdev_driver = {
  .driver =
    {
      .size = sizeof(rootdev_t),
      .desc = "RPI3 platform root bus driver",
      .attach = rootdev_attach,
    },
  .bus =
    {
      .intr_setup = rootdev_intr_setup,
      .intr_teardown = rootdev_intr_teardown,
      .alloc_resource = rootdev_alloc_resource,
    },
};

DEVCLASS_CREATE(root);

static device_t rootdev = (device_t){
  .children = TAILQ_HEAD_INITIALIZER(rootdev.children),
  .driver = (driver_t *)&rootdev_driver,
  .instance = &(rootdev_t){},
  .state = NULL,
  .devclass = &DEVCLASS(root),
};

DEVCLASS_ENTRY(root, rootdev);

void init_devices(void) {
  device_attach(&rootdev);
  /* TODO(pj) move it to other place. */
  _init_clock();
}
