#define KL_LOG KL_DEV

/* For detailed information on the controller, please refer to:
 * https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf
 */

#include <sys/mimiker.h>
#include <sys/devclass.h>
#include <sys/klog.h>
#include <sys/types.h>
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
#include <sys/fdt.h>

typedef struct bcmemmc_state {
  resource_t *emmc;      /* e.MMC controller registers */
  resource_t *irq;       /* e.MMC controller interrupt */
  condvar_t intr_recv;   /* Used to wake up a thread waiting for an interrupt */
  mtx_t lock;            /* Covers `pending`, `intr_recv` and `emmc`. */
  uint64_t rca;          /* Relative Card Address */
  uint64_t host_version; /* Host specification version */
  volatile uint32_t pending;   /* All interrupts received */
  emmc_error_t errors;         /* Error flags */
  emmc_error_t ignored_errors; /* Error flags that do not cause invalidation of
                                * current state */
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
static void bcmemmc_delay(int64_t count) {
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
    e |= EMMC_ERROR_TIMEOUT;

  return e;
}

static inline int bcmemmc_invalid_state(bcmemmc_state_t *state) {
  return state->errors & (~state->ignored_errors | EMMC_ERROR_INTERNAL |
                          EMMC_ERROR_INVALID_STATE);
}

static inline emmc_error_t bcmemmc_set_error(bcmemmc_state_t *state,
                                             emmc_error_t error_flags) {
  if (error_flags & ~state->ignored_errors) {
    state->errors |= EMMC_ERROR_INTERNAL;
  }
  state->errors |= error_flags;
  return error_flags;
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
  if (cv_wait_timed(&state->intr_recv, &state->lock, BCMEMMC_TIMEOUT))
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
static emmc_error_t bcmemmc_intr_wait(device_t *dev, uint32_t mask) {
  bcmemmc_state_t *state = (bcmemmc_state_t *)dev->state;

  assert(mask != 0);

  if (bcmemmc_invalid_state(state))
    return bcmemmc_set_error(state, EMMC_ERROR_INVALID_STATE);

  SCOPED_MTX_LOCK(&state->lock);

  for (;;) {
    if (state->pending & INT_ERROR_MASK) {
      emmc_error_t error_flags = bcemmc_decode_errors(state->pending);
      if (error_flags & ~state->ignored_errors)
        klog("e.MMC: An error flag(s) raised for e.MMC controller: 0x%x",
             state->pending & INT_ERROR_MASK);
      state->pending = 0;
      return bcmemmc_set_error(state, error_flags);
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
      return bcmemmc_set_error(state, EMMC_ERROR_TIMEOUT);
    }
  }

  return 0;
}

static emmc_error_t bcmemmc_wait(device_t *cdev, emmc_wait_flags_t wflags) {
  uint32_t mask = emmc_wait_flags_to_hwflags(wflags);
  return bcmemmc_intr_wait(cdev->parent, mask);
}

static intr_filter_t bcmemmc_intr_filter(void *data) {
  bcmemmc_state_t *state = (bcmemmc_state_t *)data;
  WITH_MTX_LOCK (&state->lock) {
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
  int32_t cnt = CLK_STABLE_TRIALS;

  b_clr(emmc, BCMEMMC_CONTROL1, C1_CLK_EN);
  /* host_version <= HOST_SPEC_V2 needs a power-of-two divisor. It would require
   * a different calculation method. */
  assert(state->host_version > HOST_SPEC_V2);
  bcmemmc_clk_set_divisor(state, frq);
  b_set(emmc, BCMEMMC_CONTROL1, C1_CLK_EN);

  /* Wait until the clock becomes stable */
  while (!(b_in(emmc, BCMEMMC_CONTROL1) & C1_CLK_STABLE)) {
    if (cnt-- < 0)
      return bcmemmc_set_error(state, EMMC_ERROR_TIMEOUT);
    bcmemmc_delay(30);
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
  if (cmd.flags & EMMC_F_DATA_MULTI) {
    code |= CMD_DATA_MULTI;
    /* Block counter must be enabled for multi-block transfers in order t
     * generate READ_DONE interrupt when the transfer succeeds. */
    code |= CMD_TM_BLKCNT_EN;
  }
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

/* There are 10 bits available for setting block size */
#define MAXBLKSIZE 512
/* There 16 bits available for setting block count */
#define MAXBLKCNT 65535

static emmc_error_t bcmemmc_get_prop(device_t *cdev, uint32_t id,
                                     uint64_t *var) {
  bcmemmc_state_t *state = (bcmemmc_state_t *)cdev->parent->state;
  resource_t *emmc = state->emmc;

  if (bcmemmc_invalid_state(state) && (id != EMMC_PROP_RW_ERRORS))
    return bcmemmc_set_error(state, EMMC_ERROR_INVALID_STATE);

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
    case EMMC_PROP_RW_ERRORS:
      *var = state->errors;
      break;
    case EMMC_PROP_RW_ALLOW_ERRORS:
      *var = state->ignored_errors;
      break;
    default:
      return bcmemmc_set_error(state, EMMC_ERROR_PROP_NOTSUP);
  }

  return 0;
}

static emmc_error_t bcmemmc_set_prop(device_t *cdev, uint32_t id,
                                     uint64_t var) {
  bcmemmc_state_t *state = (bcmemmc_state_t *)cdev->parent->state;
  resource_t *emmc = state->emmc;

  if (bcmemmc_invalid_state(state))
    return bcmemmc_set_error(state, EMMC_ERROR_INTERNAL);

  uint32_t reg = 0;
  switch (id) {
    case EMMC_PROP_RW_BLKCNT:
      /* BLKCNT can be at most a 16-bit value */
      if (var & ~0xffff)
        return bcmemmc_set_error(state, EMMC_ERROR_PROP_INVALID_ARG);
      reg = b_in(emmc, BCMEMMC_BLKSIZECNT);
      reg = (reg & 0x0000ffff) | (var << 16);
      b_out(emmc, BCMEMMC_BLKSIZECNT, reg);
      break;
    case EMMC_PROP_RW_BLKSIZE:
      /* BLKSIZE can be at most a 10-bit value */
      if ((var == 0) || (var > MAXBLKSIZE))
        return bcmemmc_set_error(state, EMMC_ERROR_PROP_INVALID_ARG);
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
    case EMMC_PROP_RW_ERRORS:
      /* The only way to reset internal error is to reset the entire controller
       * using the `reset` method. */
      state->errors = var | (state->errors & EMMC_ERROR_INTERNAL);
      break;
    case EMMC_PROP_RW_ALLOW_ERRORS:
      state->errors &= ~state->ignored_errors;
      state->ignored_errors = var;
      break;
    default:
      return bcmemmc_set_error(state, EMMC_ERROR_PROP_NOTSUP);
  }

  return 0;
}

/* Send encoded command */
static emmc_error_t bcmemmc_cmd_code(device_t *dev, uint32_t code, uint32_t arg,
                                     emmc_resp_t *resp) {
  bcmemmc_state_t *state = (bcmemmc_state_t *)dev->state;
  resource_t *emmc = state->emmc;

  emmc_error_t error = 0;

  b_out(emmc, BCMEMMC_ARG1, arg);
  b_out(emmc, BCMEMMC_CMDTM, code);
  if ((error = bcmemmc_intr_wait(dev, INT_CMD_DONE))) {
    if (state->errors & ~state->ignored_errors)
      klog("e.MMC: ERROR: failed to send e.MMC command %p (error %d)", code,
           error);
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
static emmc_error_t bcmemmc_send_cmd(device_t *cdev, emmc_cmd_t cmd,
                                     uint32_t arg, emmc_resp_t *resp) {
  bcmemmc_state_t *state = (bcmemmc_state_t *)cdev->parent->state;
  int error = 0;

  if (bcmemmc_invalid_state(state))
    return bcmemmc_set_error(state, EMMC_ERROR_INVALID_STATE);

  /* Application-specific command need to be prefixed with APP_CMD command. */
  if (cmd.flags & EMMC_F_APP)
    error = bcmemmc_send_cmd(cdev, EMMC_CMD(APP_CMD), state->rca << 16, NULL);
  if (error)
    return error;

  uint32_t code = bcmemmc_encode_cmd(cmd);
  return bcmemmc_cmd_code(cdev->parent, code, arg, resp);
}

static emmc_error_t bcmemmc_read(device_t *cdev, void *buf, size_t len,
                                 size_t *read) {
  device_t *emmcdev = cdev->parent;
  bcmemmc_state_t *state = (bcmemmc_state_t *)emmcdev->state;
  resource_t *emmc = state->emmc;
  uint32_t *data = buf;

  assert(is_aligned(len, 4)); /* Assert multiple of 32 bits */

  if (bcmemmc_invalid_state(state))
    return bcmemmc_set_error(state, EMMC_ERROR_INVALID_STATE);

  /* A very simple transfer (should be replaced with DMA in the future) */
  for (size_t i = 0; i < len / sizeof(uint32_t); i++)
    data[i] = b_in(emmc, BCMEMMC_DATA);

  if (read)
    *read = len;
  return 0;
}

static emmc_error_t bcmemmc_write(device_t *cdev, const void *buf, size_t len,
                                  size_t *wrote) {
  device_t *emmcdev = cdev->parent;
  bcmemmc_state_t *state = (bcmemmc_state_t *)emmcdev->state;
  resource_t *emmc = state->emmc;
  const uint32_t *data = buf;

  assert(is_aligned(len, 4)); /* Assert multiple of 32 bits */

  if (bcmemmc_invalid_state(state))
    return bcmemmc_set_error(state, EMMC_ERROR_INVALID_STATE);

  /* A very simple transfer (should be replaced with DMA in the future) */
  for (size_t i = 0; i < len / sizeof(uint32_t); i++)
    b_out(emmc, BCMEMMC_DATA, data[i]);

  if (wrote)
    *wrote = len;
  return 0;
}

#define BCMEMMC_INIT_FREQ 400000

static emmc_error_t bcmemmc_reset_internal(device_t *dev) {
  bcmemmc_state_t *state = (bcmemmc_state_t *)dev->state;
  resource_t *emmc = state->emmc;

  const uint32_t interrupts = INT_CMD_DONE | INT_DATA_DONE | INT_READ_RDY |
                              INT_WRITE_RDY | INT_CTO_ERR | INT_DTO_ERR;

  /* Clear errors */
  state->errors = 0;
  state->ignored_errors = 0;

  /* Disable interrupts. */
  b_out(emmc, BCMEMMC_INT_MASK, 0);
  b_out(emmc, BCMEMMC_INT_EN, 0);

  state->host_version =
    (b_in(emmc, BCMEMMC_SLOTISR_VER) & HOST_SPEC_NUM) >> HOST_SPEC_NUM_SHIFT;

  /* Reset the card. */
  b_out(emmc, BCMEMMC_CONTROL0, 0);
  b_set(emmc, BCMEMMC_CONTROL1, C1_SRST_HC);

  /* Set up clock. */
  b_set(emmc, BCMEMMC_CONTROL1, C1_CLK_INTLEN | C1_TOUNIT_MAX);
  int error = bcmemmc_set_clk_freq(dev, BCMEMMC_INIT_FREQ);
  if (error)
    return error;

  /* Enable interrupts. */
  state->pending = 0;
  b_out(emmc, BCMEMMC_INT_EN, interrupts);
  b_out(emmc, BCMEMMC_INT_MASK, interrupts);

  klog("e.MMC: Reset");

  return 0;
}

static emmc_error_t bcmemmc_reset(device_t *cdev) {
  return bcmemmc_reset_internal(cdev->parent);
}

static int bcmemmc_probe(device_t *dev) {
  return FDT_is_compatible(dev->node, "brcm,bcm2835-emmc");
}

DEVCLASS_DECLARE(emmc);

static int bcmemmc_attach(device_t *dev) {
  bcmemmc_state_t *state = (bcmemmc_state_t *)dev->state;
  int err = 0;

  state->emmc = device_take_memory(dev, 0);
  assert(state->emmc);

  if ((err = bus_map_resource(dev, state->emmc)))
    return err;

  mtx_init(&state->lock, MTX_SPIN);
  cv_init(&state->intr_recv, "e.MMC command response wakeup");

  b_out(state->emmc, BCMEMMC_INTERRUPT, INT_ALL_MASK);

  state->irq = device_take_irq(dev, 0);
  pic_setup_intr(dev, state->irq, bcmemmc_intr_filter, NULL, state,
                 "e.MMC interrupt");

  state->host_version =
    (b_in(state->emmc, BCMEMMC_SLOTISR_VER) & HOST_SPEC_NUM) >>
    HOST_SPEC_NUM_SHIFT;

  int error = bcmemmc_reset_internal(dev);
  if (error) {
    klog("e.MMC: initialzation failed with error flags %d.", error);
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
  .reset = bcmemmc_reset,
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
