#define KL_LOG KL_DEV

/* For detailed information on the controller, please refer to:
 * https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf
 */

#include <sys/mimiker.h>
#include <sys/rman.h>
#include <sys/devclass.h>
#include <sys/klog.h>
#include <sys/types.h>
#include <dev/bcm2835_gpio.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/time.h>
#include <sys/libkern.h>
#include <sys/callout.h>
#include <sys/condvar.h>
#include <sys/bus.h>
#include <dev/emmc.h>
#include <sys/errno.h>
#include <sys/bitops.h>
#include <dev/bcm2835_emmcreg.h>

typedef struct bcmemmc_state {
  resource_t *gpio;      /* GPIO resource (needed until we have a decent
                          * way of setting up GPIO) */
  resource_t *emmc;      /* e.MMC controller registers */
  resource_t *irq;       /* e.MMC controller interrupt */
  condvar_t intr_recv;   /* Used to wake up a thread waiting for an interrupt */
  spin_t lock;           /* Covers `pending`, `intr_recv` and `emmc`. */
  uint64_t rca;          /* Relative Card Address */
  uint64_t host_version; /* Host specification version */
  volatile uint32_t pending; /* All interrupts received */
  emmc_error_t errors;
} bcmemmc_state_t;

#define b_in bus_read_4
#define b_out bus_write_4
#define b_clr(mem, reg, b) b_out((mem), (reg), b_in((mem), (reg)) & ~(b))
#define b_set(mem, reg, b) b_out((mem), (reg), b_in((mem), (reg)) | (b))

/**
 * Delay function.
 * This just loops `count` times in a way that the compiler
 * won't optimize away.
 * \param count number of cycles to delay
 */
static void delay(int64_t count) {
  __asm__ volatile("1: subs %[count], %[count], #1; bne 1b"
                   : [count] "+r"(count));
}

static inline uint32_t emmc_wait_flags_to_hwflags(emmc_wait_flags_t mask) {
  return ((mask & EMMC_I_DATA_DONE) ? INT_DATA_DONE : 0) |
         ((mask & EMMC_I_READ_READY) ? INT_READ_RDY : 0) |
         ((mask & EMMC_I_WRITE_READY) ? INT_WRITE_RDY : 0);
}

/* Timeout when awaiting an interrupt */
#define BCMEMMC_TIMEOUT 10000
#define BCMEMMC_BUSY_CYCLES 128

static emmc_error_t bcemmc_decode_errors(uint32_t interrupts) {
  emmc_error_t e = 0;

  if (!(interrupts & INT_ERR))
    return e;
   
  if (interrupts & INT_CTO_ERR)
    e |= EMMC_ERROR_CMD_TIMEOUT;
  
  return e;
}

/* Returns new pending interrupts.
 * All pending interrupts are stored in `bmcemmc_state_t::pending`.
 */
static uint32_t bcmemmc_read_intr(bcmemmc_state_t *state) {
  resource_t *emmc = state->emmc;
  uint32_t newpend;

  newpend = b_in(emmc, BCMEMMC_INTERRUPT);

  /* Pending interrupts need to be cleared manually. */
  if (newpend) {
    state->pending |= newpend;
    b_out(emmc, BCMEMMC_INTERRUPT, newpend);
  }
  return newpend;
}

/**
 * Go to sleep for limited time and wait for an interrupt, then read it in ISR.
 * \return 0 if not interrupt was received, otherwise a mask of all received
 * interrupts (including ones received before issueing this call if they weren't
 * handled, ie. awaited for by using `bcmemmc_wait`/`bcmemmc_intr_wait`).
 * \warning THIS FUNCTION NEEDS TO BE CALLED WITH `state->lock` LOCKED!
 */
static inline uint32_t bcmemmc_try_read_intr_blocking(bcmemmc_state_t *state) {
  if (cv_wait_timed(&state->intr_recv, (lock_t)&state->lock, BCMEMMC_TIMEOUT))
    return 0;
  return state->pending;
}

/* Return 1 if there are any interrupts to be processed, 0 if not */
static inline int bcmemmc_check_intr(bcmemmc_state_t *state) {
  resource_t *emmc = state->emmc;
  return (b_in(emmc, BCMEMMC_INTERRUPT) | state->pending) ? 1 : 0;
}

/**
 * \brief Wait for the specified interrupts.
 * \param dev eMMC device
 * \param mask expected interrupts
 * \return 0 on success, EIO on internal error, ETIMEDOUT on device timeout.
 * \warning This procedure may put the thread to sleep.
 */
static int32_t bcmemmc_intr_wait(device_t *dev, uint32_t mask) {
  bcmemmc_state_t *state = (bcmemmc_state_t *)dev->state;

  assert(mask != 0);

  SCOPED_SPIN_LOCK(&state->lock);

  for (;;) {
    if (state->pending & INT_ERROR_MASK) {
      klog("An error flag(s) has beem raised for e.MMC controller: 0x%x",
           state->pending & INT_ERROR_MASK);
      state->pending = 0;

      state->errors = bcemmc_decode_errors(state->pending);
      return EIO;
    }

    if ((state->pending & mask) == mask) {
      state->pending &= ~mask;
      state->errors = 0;
      return 0;
    }

    /* Busy-wait for a while. Should be good enough if the card works fine */
    for (int i = 0; i < BCMEMMC_BUSY_CYCLES; i++) {
      if (bcmemmc_check_intr(state)) {
        (void)bcmemmc_read_intr(state);
        continue;
      }
    }

    /* Sleep for a while if no new interrupts have been received so far */
    if (!bcmemmc_try_read_intr_blocking(state)) {
      state->errors = 0;
      return ETIMEDOUT;
    }
  }

  return 0;
}

static int bcmemmc_wait(device_t *cdev, emmc_wait_flags_t wflags) {
  uint32_t mask = emmc_wait_flags_to_hwflags(wflags);
  return bcmemmc_intr_wait(cdev->parent, mask);
}

static intr_filter_t bcmemmc_intr_filter(void *data) {
  bcmemmc_state_t *state = (bcmemmc_state_t *)data;
  WITH_SPIN_LOCK (&state->lock) {
    if (!bcmemmc_read_intr(state))
      return IF_STRAY;
    /* Wake up the thread if all expected interrupts have been received */
    cv_signal(&state->intr_recv);
  }
  return IF_FILTERED;
}

/* Find a divisor that provides the best approximation.
 * That is an integer `c` such as for every integer `c'` != `c`
 * |`frq` - (`clk` / `c`)| <= |`frq` - (`clk` / `c'`)| */
static uint32_t bcmemmc_clk_approx_divisor(uint32_t clk, uint32_t frq) {
  int32_t c1 = clk / frq;
  if (c1 == 0)
    c1++;
  int32_t c2 = c1 + 1;
  int32_t est1 = abs((int32_t)frq - (int32_t)clk / c1);
  int32_t est2 = abs((int32_t)frq - (int32_t)clk / c2);
  return (est1 < est2) ? c1 : c2;
}

/* Set e.MMC clock's divisor to match frequency `frq` */
static void bcmemmc_clk_set_divisor(bcmemmc_state_t *state, uint32_t frq) {
  resource_t *emmc = state->emmc;

  uint32_t clk = GPIO_CLK_EMMC_DEFAULT_FREQ;
  uint32_t divisor = bcmemmc_clk_approx_divisor(clk, frq);

  /* The low and high bytes for clock divisor are swapped and shifted in
   * BCMEMMC_CONTROL1 register, so we need to take that into account */
  uint32_t lo = (divisor & 0x00ff) << 8;
  uint32_t hi = (divisor & 0x0300) >> 2;

  uint32_t ctl1 = b_in(emmc, BCMEMMC_CONTROL1) & BCMEMMC_CLKDIV_INVMASK;
  ctl1 |= lo | hi;
  b_out(emmc, BCMEMMC_CONTROL1, ctl1);

  klog("e.MMC: clock set to %luHz / %lu (requested %luHz)", clk, divisor, frq);
}

#define CLK_STABLE_TRIALS 10000

/**
 * set SD clock to frequency in Hz (approximately), divided mode
 */
static int bcmemmc_set_clk_freq(device_t *dev, uint32_t frq) {
  bcmemmc_state_t *state = (bcmemmc_state_t *)dev->state;
  resource_t *emmc = state->emmc;
  int32_t cnt = 100000;

  b_clr(emmc, BCMEMMC_CONTROL1, C1_CLK_EN);
  /* host_version <= HOST_SPEC_V2 needs a power-of-two divisor. It would require
   * a different calculation method. */
  assert(state->host_version > HOST_SPEC_V2);
  bcmemmc_clk_set_divisor(state, frq);
  b_set(emmc, BCMEMMC_CONTROL1, C1_CLK_EN);

  /* Wait until the clock becomes stable */
  cnt = CLK_STABLE_TRIALS;
  while (!(b_in(emmc, BCMEMMC_CONTROL1) & C1_CLK_STABLE)) {
    if (cnt-- < 0)
      return ETIMEDOUT;
    delay(30);
  }

  return 0;
}

/* Encode `cmd` to command an appropriate command register value */
static uint32_t bcmemmc_encode_cmd(emmc_cmd_t cmd) {
  uint32_t code = (uint32_t)cmd.cmd_idx << 24;

  switch (cmd.exp_resp) {
    case EMMCRESP_NONE:
      break;
    case EMMCRESP_R2:
      code |= CMD_RESP136;
      break;
    case EMMCRESP_R1B:
      code |= CMD_RESP48B;
      break;
    default:
      code |= CMD_RESP48;
  }
  switch (emmc_cmdtype(cmd.flags)) {
    case EMMC_F_SUSPEND:
      code |= CMD_TYPE_SUSPEND;
      break;
    case EMMC_F_RESUME:
      code |= CMD_TYPE_RESUME;
      break;
    case EMMC_F_ABORT:
      code |= CMD_TYPE_ABORT;
      break;
    default:
      break;
  }
  if (cmd.flags & EMMC_F_DATA_READ)
    code |= CMD_DATA_TRANSFER | CMD_DATA_READ;
  if (cmd.flags & EMMC_F_DATA_WRITE)
    code |= CMD_DATA_TRANSFER;
  if (cmd.flags & EMMC_F_DATA_MULTI)
    code |= CMD_DATA_MULTI;
  if (cmd.flags & EMMC_F_CHKIDX)
    code |= CMD_CHECKIDX;
  if (cmd.flags & EMMC_F_CHKCRC)
    code |= CMD_CHECKCRC;

  return code;
}

static int bcmemmc_set_bus_width(bcmemmc_state_t *state, uint32_t bw) {
  switch (bw) {
    case EMMC_BUSWIDTH_1:
      b_clr(state->emmc, BCMEMMC_CONTROL0, C0_HCTL_8BIT);
      b_clr(state->emmc, BCMEMMC_CONTROL0, C0_HCTL_DWITDH);
      return 0;
    case EMMC_BUSWIDTH_4:
      b_clr(state->emmc, BCMEMMC_CONTROL0, C0_HCTL_8BIT);
      b_set(state->emmc, BCMEMMC_CONTROL0, C0_HCTL_DWITDH);
      return 0;
    case EMMC_BUSWIDTH_8:
      b_set(state->emmc, BCMEMMC_CONTROL0, C0_HCTL_8BIT);
      b_clr(state->emmc, BCMEMMC_CONTROL0, C0_HCTL_DWITDH);
      return 0;
    default:
      return EINVAL;
  }
}

static int bcmemmc_get_bus_width(bcmemmc_state_t *state) {
  uint32_t ctl = b_in(state->emmc, BCMEMMC_CONTROL0);
  if (ctl & C0_HCTL_8BIT)
    return EMMC_BUSWIDTH_8;
  if (ctl & C0_HCTL_DWITDH)
    return EMMC_BUSWIDTH_4;
  return EMMC_BUSWIDTH_1;
}

#define MAXBLKSIZE 512 /* Currently only blocks of 512 bytes are supported. */
#define MAXBLKCNT 255  /* Transfers cannot exceed 255 consecutive blocks. */

static int bcmemmc_get_prop(device_t *cdev, uint32_t id, uint64_t *var) {
  bcmemmc_state_t *state = (bcmemmc_state_t *)cdev->parent->state;
  resource_t *emmc = state->emmc;

  uint32_t reg = 0;
  switch (id) {
    case EMMC_PROP_RW_BLKCNT:
      reg = b_in(emmc, BCMEMMC_BLKSIZECNT);
      *var = (reg & BSC_BLKCNT) >> BSC_BLKCNT_SHIFT;
      break;
    case EMMC_PROP_RW_BLKSIZE:
      reg = b_in(emmc, BCMEMMC_BLKSIZECNT);
      *var = reg & BSC_BLKSIZE;
      break;
    case EMMC_PROP_R_MAXBLKSIZE:
      *var = MAXBLKSIZE;
      break;
    case EMMC_PROP_R_MAXBLKCNT:
      *var = MAXBLKCNT;
      break;
    case EMMC_PROP_R_VOLTAGE_SUPPLY:
      *var = EMMC_VOLTAGE_WINDOW_LOW;
      break;
    case EMMC_PROP_RW_RESP_LOW:
      *var = (uint64_t)b_in(emmc, BCMEMMC_RESP0) |
             (uint64_t)b_in(emmc, BCMEMMC_RESP1) << 32;
      break;
    case EMMC_PROP_RW_RESP_HI:
      *var = (uint64_t)b_in(emmc, BCMEMMC_RESP2) |
             (uint64_t)b_in(emmc, BCMEMMC_RESP3) << 32;
      break;
    case EMMC_PROP_RW_BUSWIDTH:
      *var = bcmemmc_get_bus_width(state);
      break;
    case EMMC_PROP_RW_RCA:
      *var = state->rca;
      break;
    case EMMC_PROP_R_ERRORS:
      *var = state->errors;
      break;
    default:
      return ENODEV;
  }

  return 0;
}

static int bcmemmc_set_prop(device_t *cdev, uint32_t id, uint64_t var) {
  bcmemmc_state_t *state = (bcmemmc_state_t *)cdev->parent->state;
  resource_t *emmc = state->emmc;

  uint32_t reg = 0;
  switch (id) {
    case EMMC_PROP_RW_BLKCNT:
      /* BLKCNT can be at most a 16-bit value */
      if (var & ~0xffff)
        return EINVAL;
      reg = b_in(emmc, BCMEMMC_BLKSIZECNT);
      reg = (reg & 0x0000ffff) | (var << 16);
      b_out(emmc, BCMEMMC_BLKSIZECNT, reg);
      break;
    case EMMC_PROP_RW_BLKSIZE:
      /* BLKSIZE can be at most 10-bit value */
      if (var & ~0x03ff)
        return EINVAL;
      reg = b_in(emmc, BCMEMMC_BLKSIZECNT);
      reg = (reg & ~0x03ff) | var;
      b_out(emmc, BCMEMMC_BLKSIZECNT, reg);
      break;
    case EMMC_PROP_RW_RESP_LOW:
      b_out(emmc, BCMEMMC_RESP0, (uint32_t)var);
      b_out(emmc, BCMEMMC_RESP1, (uint32_t)(var >> 32));
      break;
    case EMMC_PROP_RW_RESP_HI:
      b_out(emmc, BCMEMMC_RESP2, (uint32_t)var);
      b_out(emmc, BCMEMMC_RESP3, (uint32_t)(var >> 32));
      break;
    case EMMC_PROP_RW_CLOCK_FREQ:
      return bcmemmc_set_clk_freq(cdev->parent, var);
    case EMMC_PROP_RW_BUSWIDTH:
      return bcmemmc_set_bus_width(state, var);
    case EMMC_PROP_RW_RCA:
      state->rca = var;
      break;
    default:
      return ENODEV;
  }

  return 0;
}

/* Send encoded command */
static int bcmemmc_cmd_code(device_t *dev, uint32_t code, uint32_t arg,
                            emmc_resp_t *resp) {
  bcmemmc_state_t *state = (bcmemmc_state_t *)dev->state;
  resource_t *emmc = state->emmc;

  uint32_t error = 0;

  b_out(emmc, BCMEMMC_ARG1, arg);
  b_out(emmc, BCMEMMC_CMDTM, code);
  if ((error = bcmemmc_intr_wait(dev, INT_CMD_DONE))) {
    klog("ERROR: failed to send EMMC command %p (error %d)", code, error);
    return error;
  }

  if (resp) {
    resp->r[0] = b_in(emmc, BCMEMMC_RESP0);
    resp->r[1] = b_in(emmc, BCMEMMC_RESP1);
    resp->r[2] = b_in(emmc, BCMEMMC_RESP2);
    resp->r[3] = b_in(emmc, BCMEMMC_RESP3);
  }

  return 0;
}

/* Send a command */
static int bcmemmc_send_cmd(device_t *cdev, emmc_cmd_t cmd, uint32_t arg,
                            emmc_resp_t *resp) {
  bcmemmc_state_t *state = (bcmemmc_state_t *)cdev->parent->state;
  int error = 0;

  /* Application-specific command need to be prefixed with APP_CMD command. */
  if (cmd.flags & EMMC_F_APP)
    error = bcmemmc_send_cmd(cdev, EMMC_CMD(APP_CMD), state->rca << 16, NULL);
  if (error)
    return error;

  uint32_t code = bcmemmc_encode_cmd(cmd);
  return bcmemmc_cmd_code(cdev->parent, code, arg, resp);
}

static int bcmemmc_read(device_t *cdev, void *buf, size_t len, size_t *read) {
  device_t *emmcdev = cdev->parent;
  bcmemmc_state_t *state = (bcmemmc_state_t *)emmcdev->state;
  resource_t *emmc = state->emmc;
  uint32_t *data = buf;

  assert(is_aligned(len, 4)); /* Assert multiple of 32 bits */

  /* A very simple transfer (should be replaced with DMA in the future) */
  for (size_t i = 0; i < len / sizeof(uint32_t); i++)
    data[i] = b_in(emmc, BCMEMMC_DATA);

  /* TODO (mohrcore): check whether the transfer fully succeeded! */
  if (read)
    *read = len;
  return 0;
}

static int bcmemmc_write(device_t *cdev, const void *buf, size_t len,
                         size_t *wrote) {
  device_t *emmcdev = cdev->parent;
  bcmemmc_state_t *state = (bcmemmc_state_t *)emmcdev->state;
  resource_t *emmc = state->emmc;
  const uint32_t *data = buf;

  assert(is_aligned(len, 4)); /* Assert multiple of 32 bits */

  /* A very simple transfer (should be replaced with DMA in the future) */
  for (size_t i = 0; i < len / sizeof(uint32_t); i++)
    b_out(emmc, BCMEMMC_DATA, data[i]);

  /* TODO (mohr): check wether the transfer fully succeeded! */
  if (wrote)
    *wrote = len;
  return 0;
}

/* e.MMC requires some GPIO setup to work properly. This however is different
 * than what is described in BCM2835 Peripherals datasheet.
 * See: https://www.raspberrypi.org/app/uploads/2012/04/
 *      Raspberry-Pi-Schematics-R1.0.pdf */
static void emmc_gpio_init(device_t *dev) {
  bcmemmc_state_t *state = (bcmemmc_state_t *)dev->state;
  resource_t *gpio = state->gpio;

  /* GPIO_CD: interrupt pin */
  bcm2835_gpio_function_select(gpio, 47, BCM2835_GPIO_ALT3);
  bcm2835_gpio_set_pull(gpio, 47, BCM2838_GPIO_GPPUD_PULLDOWN);
  bcm2835_gpio_set_high_detect(gpio, 47, true);

  /* GPIO_CLK, GPIO_CMD */
  bcm2835_gpio_function_select(gpio, 48, BCM2835_GPIO_ALT3);
  bcm2835_gpio_function_select(gpio, 49, BCM2835_GPIO_ALT3);
  bcm2835_gpio_set_pull(gpio, 48, BCM2838_GPIO_GPPUD_PULLDOWN);
  bcm2835_gpio_set_pull(gpio, 49, BCM2838_GPIO_GPPUD_PULLDOWN);

  /* GPIO_DAT0, GPIO_DAT1, GPIO_DAT2, GPIO_DAT3 */
  bcm2835_gpio_function_select(gpio, 50, BCM2835_GPIO_ALT3);
  bcm2835_gpio_function_select(gpio, 51, BCM2835_GPIO_ALT3);
  bcm2835_gpio_function_select(gpio, 52, BCM2835_GPIO_ALT3);
  bcm2835_gpio_function_select(gpio, 53, BCM2835_GPIO_ALT3);
  bcm2835_gpio_set_pull(gpio, 50, BCM2838_GPIO_GPPUD_PULLDOWN);
  bcm2835_gpio_set_pull(gpio, 51, BCM2838_GPIO_GPPUD_PULLDOWN);
  bcm2835_gpio_set_pull(gpio, 52, BCM2838_GPIO_GPPUD_PULLDOWN);
  bcm2835_gpio_set_pull(gpio, 53, BCM2838_GPIO_GPPUD_PULLDOWN);
}

#define BCMEMMC_INIT_FREQ 400000

static int bcmemmc_init(device_t *dev) {
  bcmemmc_state_t *state = (bcmemmc_state_t *)dev->state;
  resource_t *emmc = state->emmc;
  int cnt;

  state->host_version =
    (b_in(emmc, BCMEMMC_SLOTISR_VER) & HOST_SPEC_NUM) >> HOST_SPEC_NUM_SHIFT;
  
  /* Reset the card. */
  b_out(emmc, BCMEMMC_CONTROL0, 0);
  b_set(emmc, BCMEMMC_CONTROL1, C1_SRST_HC);
  cnt = 10000;
  do {
    delay(30); /* TODO: Test it on hardware */
  } while ((b_in(emmc, BCMEMMC_CONTROL1) & C1_SRST_HC) && cnt--);
  if (cnt < 0) {
    klog("ERROR: failed to reset EMMC");
    return ETIMEDOUT;
  }

  /* Set up clock. */
  b_set(emmc, BCMEMMC_CONTROL1, C1_CLK_INTLEN | C1_TOUNIT_MAX);
  int error = bcmemmc_set_clk_freq(dev, BCMEMMC_INIT_FREQ);
  if (error)
    return error;

  /* Enable interrupts. */
  state->pending = 0;
  const uint32_t interrupts = INT_CMD_DONE | INT_DATA_DONE | INT_READ_RDY |
                              INT_WRITE_RDY | INT_CTO_ERR | INT_DTO_ERR;
  b_out(emmc, BCMEMMC_INT_EN, interrupts);
  b_out(emmc, BCMEMMC_INT_MASK, interrupts);

  return 0;
}

static int bcmemmc_probe(device_t *dev) {
  return dev->unit == 3;
}

DEVCLASS_DECLARE(emmc);

static int bcmemmc_attach(device_t *dev) {
  bcmemmc_state_t *state = (bcmemmc_state_t *)dev->state;

  state->gpio = device_take_memory(dev, 0, RF_ACTIVE);
  state->emmc = device_take_memory(dev, 1, RF_ACTIVE);

  spin_init(&state->lock, 0);
  cv_init(&state->intr_recv, "e.MMC command response wakeup");

  b_out(state->emmc, BCMEMMC_INTERRUPT, INT_ALL_MASK);
  emmc_gpio_init(dev);
  klog("e.MMC: GPIO set up");

  state->irq = device_take_irq(dev, 0, RF_ACTIVE);
  bus_intr_setup(dev, state->irq, bcmemmc_intr_filter, NULL, state,
                 "e.MMC interrupt");

  int error = bcmemmc_init(dev);
  if (error) {
    klog("e.MMC initialzation failed with code %d.", error);
    return ENXIO;
  }

  /* This is not a legitimate bus in a sense that it implements `DIF_BUS`, but
   * it should work nevertheless */
  device_t *child = device_add_child(dev, 0);
  child->bus = DEV_BUS_EMMC;
  child->devclass = &DEVCLASS(emmc);
  child->unit = 0;

  return bus_generic_probe(dev);
}

static emmc_methods_t bcmemmc_emmc_if = {
  .send_cmd = bcmemmc_send_cmd,
  .wait = bcmemmc_wait,
  .read = bcmemmc_read,
  .write = bcmemmc_write,
  .get_prop = bcmemmc_get_prop,
  .set_prop = bcmemmc_set_prop,
};

static driver_t bcmemmc_driver = {
  .desc = "e.MMC controller driver",
  .size = sizeof(bcmemmc_state_t),
  .probe = bcmemmc_probe,
  .attach = bcmemmc_attach,
  .pass = SECOND_PASS,
  .interfaces =
    {
      [DIF_EMMC] = &bcmemmc_emmc_if,
    },
};

DEVCLASS_ENTRY(root, bcmemmc_driver);
