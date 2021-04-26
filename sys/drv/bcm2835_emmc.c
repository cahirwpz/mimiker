#define KL_LOG KL_DEV

#include <sys/memdev.h>
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

#define GPPUD 0x0094

#define EMMC_ARG2 0x0000
#define EMMC_BLKSIZECNT 0x0004
#define EMMC_ARG1 0x0008
#define EMMC_CMDTM 0x000C
#define EMMC_RESP0 0x0010
#define EMMC_RESP1 0x0014
#define EMMC_RESP2 0x0018
#define EMMC_RESP3 0x001C
#define EMMC_DATA 0x0020
#define EMMC_STATUS 0x0024
#define EMMC_CONTROL0 0x0028
#define EMMC_CONTROL1 0x002C
#define EMMC_INTERRUPT 0x0030
#define EMMC_INT_MASK 0x0034
#define EMMC_INT_EN 0x0038
#define EMMC_CONTROL2 0x003C
#define EMMC_SLOTISR_VER 0x00FC

/* STATUS register fields */
#define SR_DAT_INHIBIT 0x00000002
#define SR_CMD_INHIBIT 0x00000001

/* INTERRUPT register fields */
#define INT_DATA_TIMEOUT 0x00100000
#define INT_CMD_TIMEOUT 0x00010000
#define INT_READ_RDY 0x00000020
#define INT_WRITE_RDY 0x00000010
#define INT_DATA_DONE 0x00000002
#define INT_CMD_DONE 0x00000001

#define INT_ERROR_MASK 0x017E8000

/* CONTROL register fields */
#define C0_HCTL_DWITDH 0x00000002
#define C0_HCTL_8BIT 0x00000020

#define C1_SRST_HC 0x01000000
#define C1_TOUNIT_MAX 0x000e0000
#define C1_CLK_EN 0x00000004
#define C1_CLK_STABLE 0x00000002
#define C1_CLK_INTLEN 0x00000001

/* SLOTISR_VER values */
#define HOST_SPEC_NUM 0x00ff0000
#define HOST_SPEC_NUM_SHIFT 16
#define HOST_SPEC_V3 2
#define HOST_SPEC_V2 1
#define HOST_SPEC_V1 0

// SCR flags
#define SCR_SD_BUS_WIDTH_4 0x00000400
#define SCR_SUPP_SET_BLKCNT 0x02000000
// added by my driver
#define SCR_SUPP_CCS 0x00000001

#define ACMD41_VOLTAGE 0x00ff8000
#define ACMD41_CMD_COMPLETE 0x80000000
#define ACMD41_CMD_CCS 0x40000000
#define ACMD41_ARG_HC 0x51ff8000

static driver_t bcmemmc_driver;

typedef struct bcmemmc_state {
  resource_t *gpio;
  resource_t *emmc;
  resource_t *irq;
  condvar_t cv_response;
  spin_t slock;
  uint64_t rca;
  uint64_t sd_err;
  uint64_t host_version;
  uint32_t intr_flags;
  int twait;
} bcmemmc_state_t;

#define b_in bus_read_4
#define b_out bus_write_4
#define b_clr(mem, reg, b) b_out((mem), (reg), b_in((mem), (reg)) & ~(b))
#define b_set(mem, reg, b) b_out((mem), (reg), b_in((mem), (reg)) | (b))

/**
 * Delay function.
 * This just loops <delay> times in a way that the compiler
 * won't optimize away.
 * \param delay number of cycles to delay
 */
static void delay(int64_t count) {
  __asm__ volatile("1: subs %[count], %[count], #1; bne 1b"
                   : [ count ] "+r"(count));
}

static inline uint32_t emmc_wait_flags_to_hwflags(emmc_wait_flags_t mask) {
  uint32_t result = ((mask & EMMC_I_DATA_DONE) ? INT_DATA_DONE : 0) |
                    ((mask & EMMC_I_READ_READY) ? INT_READ_RDY : 0) |
                    ((mask & EMMC_I_WRITE_READY) ? INT_WRITE_RDY : 0);
  return result;
}

/**
 * \brief Wait for (a set of) interrupt
 * \param dev eMMC device
 * \param mask expected interrupts
 * \param clear additional interrupt bits to be cleared
 * \return 0 on success, EIO on internal error, ETIMEDOUT on device timeout.
 */
static int32_t bcmemmc_intr_wait(device_t *dev, uint32_t mask) {
  bcmemmc_state_t *state = (bcmemmc_state_t *)dev->state;
  resource_t *emmc = state->emmc;
  uint32_t m = 0;

  WITH_SPIN_LOCK (&state->slock) {
  bcmemmc_restart_intr_wait:
    while (mask) {
      if (state->intr_flags & mask) {
        m = mask;
        mask &= ~state->intr_flags;
        state->intr_flags &= ~m;
        goto bcmemmc_restart_intr_wait;
      }
      /* Busy-wait for a while. Should be good enough if the card works fine */
      for (int i = 0; i < 64; i++) {
        if ((state->intr_flags = b_in(emmc, EMMC_INTERRUPT)))
          goto bcmemmc_restart_intr_wait;
      }
      /* Sleep for a while if no interrupts have been received so far */
      if (!state->intr_flags) {
        state->twait = 1;
        if (cv_wait_timed(&state->cv_response, &state->slock, 10000)) {
          state->twait = 0;
          b_out(emmc, EMMC_INTERRUPT, 0xffffffff);
          return ETIMEDOUT;
        }
        state->twait = 0;
      }
      if (state->intr_flags & INT_ERROR_MASK) {
        state->intr_flags = 0;
        return EIO;
      }
      m = state->intr_flags;
      mask &= ~state->intr_flags;
      state->intr_flags &= ~m;
    }
  }
  return 0;
}

static int bcmemmc_wait(device_t *cdev, emmc_wait_flags_t wflags) {
  assert(cdev->parent && cdev->parent->driver == &bcmemmc_driver);

  uint32_t mask = emmc_wait_flags_to_hwflags(wflags);
  return bcmemmc_intr_wait(cdev->parent, mask);
}

static intr_filter_t bcmemmc_intr_filter(void *data) {
  bcmemmc_state_t *state = (bcmemmc_state_t *)data;
  resource_t *emmc = state->emmc;
  WITH_SPIN_LOCK (&state->slock) {
    uint32_t r = b_in(emmc, EMMC_INTERRUPT);
    state->intr_flags = r;
    /* Interrupts need to be cleared manually */
    b_out(emmc, EMMC_INTERRUPT, r);
    /* Wake up the thread if all expected interrupts have been received */
    if (state->twait)
      cv_signal(&state->cv_response);
  }
  return IF_FILTERED;
}

/**
 * set SD clock to frequency in Hz
 */
int32_t bcmemmc_clk(device_t *dev, uint32_t f) {
  bcmemmc_state_t *state = (bcmemmc_state_t *)dev->state;
  resource_t *emmc = state->emmc;
  uint32_t d, c = 41666666 / f, x, s = 32, h = 0;
  int32_t cnt = 100000;
  while ((b_in(emmc, EMMC_STATUS) & (SR_CMD_INHIBIT | SR_DAT_INHIBIT)) && cnt--)
    delay(3); /* ! */
  if (cnt <= 0) {
    klog("ERROR: timeout waiting for inhibit flag");
    return ETIMEDOUT;
  }

  b_clr(emmc, EMMC_CONTROL1, C1_CLK_EN);
  delay(30); /* ! */
  x = c - 1;
  if (!x)
    s = 0;
  else {
    if (!(x & 0xffff0000u)) {
      x <<= 16;
      s -= 16;
    }
    if (!(x & 0xff000000u)) {
      x <<= 8;
      s -= 8;
    }
    if (!(x & 0xf0000000u)) {
      x <<= 4;
      s -= 4;
    }
    if (!(x & 0xc0000000u)) {
      x <<= 2;
      s -= 2;
    }
    if (!(x & 0x80000000u)) {
      x <<= 1;
      s -= 1;
    }
    if (s > 0)
      s--;
    if (s > 7)
      s = 7;
  }
  if (state->host_version > HOST_SPEC_V2)
    d = c;
  else
    d = (1 << s);
  if (d <= 2) {
    d = 2;
    s = 0;
  }
  klog("bcmemmc_clk divisor %p, shift %p", d, s);
  if (state->host_version > HOST_SPEC_V2)
    h = (d & 0x300) >> 2;
  d = (((d & 0x0ff) << 8) | h);
  b_out(emmc, EMMC_CONTROL1, (b_in(emmc, EMMC_CONTROL1) & 0xffff003f) | d);
  delay(30); /* ! */
  b_set(emmc, EMMC_CONTROL1, C1_CLK_EN);
  delay(30); /* ! */
  cnt = 10000;
  while (!(b_in(emmc, EMMC_CONTROL1) & C1_CLK_STABLE) && cnt--)
    delay(30); /* ! */
  if (cnt <= 0) {
    klog("ERROR: failed to get stable clock");
    return ETIMEDOUT;
  }
  return 0;
}

#define CMD_TYPE_SUSPEND 0x00400000
#define CMD_TYPE_RESUME 0x00800000
#define CMD_TYPE_ABORT 0x00c00000
#define CMD_DATA_TRANSFER 0x00200000
#define CMD_DATA_READ 0x00000010
#define CMD_DATA_MULTI 0x00000020
#define CMD_CHECKCRC 0x00080000
#define CMD_CHECKIDX 0x00100000
#define CMD_RESP136 0x00010000
#define CMD_RESP48 0x00020000
#define CMD_RESP48B 0x00030000

/* This function might be (and probably is!) incomplete, but it does enough
 * to handle the current block device scenario */
uint32_t encode_cmd(emmc_cmd_t cmd) {
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
  if (cmd.flags & EMMC_F_DATA_READ) {
    code |= CMD_DATA_TRANSFER | CMD_DATA_READ;
  }
  if (cmd.flags & EMMC_F_DATA_WRITE) {
    code |= CMD_DATA_TRANSFER;
  }
  if (cmd.flags & EMMC_F_DATA_MULTI) {
    code |= CMD_DATA_MULTI;
  }
  if (cmd.flags & EMMC_F_CHKIDX)
    code |= CMD_CHECKIDX;
  if (cmd.flags & EMMC_F_CHKCRC)
    code |= CMD_CHECKCRC;

  return code;
}

static int bcmemmc_get_prop(device_t *cdev, uint32_t id, uint64_t *var) {
  assert(cdev->parent && cdev->parent->driver == &bcmemmc_driver);
  bcmemmc_state_t *state = (bcmemmc_state_t *)cdev->parent->state;
  resource_t *emmc = state->emmc;

  uint32_t reg = 0;
  switch (id) {
    case EMMC_PROP_RW_BLKCNT:
      reg = b_in(emmc, EMMC_BLKSIZECNT);
      *var = (reg & 0xffff0000) >> 16;
      return 0;
    case EMMC_PROP_RW_BLKSIZE:
      reg = b_in(emmc, EMMC_BLKSIZECNT);
      *var = reg & 0x03ff;
      return 0;
    case EMMC_PROP_R_MAXBLKSIZE:
      *var = 512;
      return 0;
    case EMMC_PROP_R_MAXBLKCNT:
      *var = 255;
      return 0;
    case EMMC_PROP_R_VOLTAGE_SUPPLY:
      *var = 1;
      return 0;
    case EMMC_PROP_RW_RESP_LOW:
      *var = (uint64_t)b_in(emmc, EMMC_RESP0) | (uint64_t)b_in(emmc, EMMC_RESP1)
                                                  << 32;
      return 0;
    case EMMC_PROP_RW_RESP_HI:
      *var = (uint64_t)b_in(emmc, EMMC_RESP2) | (uint64_t)b_in(emmc, EMMC_RESP3)
                                                  << 32;
      return 0;
    case EMMC_PROP_RW_RCA:
      *var = state->rca;
      return 0;
    default:
      return ENODEV;
  }
}

static int bcmemmc_set_prop(device_t *cdev, uint32_t id, uint64_t var) {
  assert(cdev->parent && cdev->parent->driver == &bcmemmc_driver);
  bcmemmc_state_t *state = (bcmemmc_state_t *)cdev->parent->state;
  resource_t *emmc = state->emmc;

  uint32_t reg = 0;
  switch (id) {
    case EMMC_PROP_RW_BLKCNT:
      if (var & ~0xffff)
        return EINVAL;
      reg = b_in(emmc, EMMC_BLKSIZECNT);
      reg = (reg & 0x0000ffff) | (var << 16);
      b_out(emmc, EMMC_BLKSIZECNT, reg);
      return 0;
    case EMMC_PROP_RW_BLKSIZE:
      if (var & ~0x03ff)
        return 0;
      reg = b_in(emmc, EMMC_BLKSIZECNT);
      reg = (reg & ~0x03ff) | var;
      b_out(emmc, EMMC_BLKSIZECNT, reg);
      return 0;
    case EMMC_PROP_RW_RESP_LOW:
      b_out(emmc, EMMC_RESP0, (uint32_t)var);
      b_out(emmc, EMMC_RESP1, (uint32_t)(var >> 32));
      return 0;
    case EMMC_PROP_RW_RESP_HI:
      b_out(emmc, EMMC_RESP2, (uint32_t)var);
      b_out(emmc, EMMC_RESP3, (uint32_t)(var >> 32));
      return 0;
    case EMMC_PROP_RW_CLOCK_FREQ:
      return bcmemmc_clk(cdev->parent, var);
    case EMMC_PROP_RW_BUSWIDTH:
      switch (var) {
        case EMMC_BUSWIDTH_1:
          b_clr(emmc, EMMC_CONTROL0, C0_HCTL_8BIT);
          b_clr(emmc, EMMC_CONTROL0, C0_HCTL_DWITDH);
          return 0;
        case EMMC_BUSWIDTH_4:
          b_clr(emmc, EMMC_CONTROL0, C0_HCTL_8BIT);
          b_set(emmc, EMMC_CONTROL0, C0_HCTL_DWITDH);
          return 0;
        case EMMC_BUSWIDTH_8:
          b_set(emmc, EMMC_CONTROL0, C0_HCTL_8BIT);
          b_clr(emmc, EMMC_CONTROL0, C0_HCTL_DWITDH);
          return 0;
        default:
          return EINVAL;
      }
    case EMMC_PROP_RW_RCA:
      state->rca = var;
      return 0;
    default:
      return ENODEV;
  }
}

static int bcmemmc_cmd_code(device_t *dev, uint32_t code, uint32_t arg1,
                            uint32_t arg2, emmc_resp_t *resp) {
  bcmemmc_state_t *state = (bcmemmc_state_t *)dev->state;
  resource_t *emmc = state->emmc;

  uint32_t r = 0;
  state->sd_err = 0;

  b_out(emmc, EMMC_ARG1, arg1);
  b_out(emmc, EMMC_ARG2, arg2);
  b_out(emmc, EMMC_CMDTM, code);
  if ((r = bcmemmc_intr_wait(dev, INT_CMD_DONE))) {
    klog("ERROR: failed to send EMMC command %p", code);
    state->sd_err = r;
    return 0;
  }

  if (resp) {
    resp->r[0] = b_in(emmc, EMMC_RESP0);
    resp->r[1] = b_in(emmc, EMMC_RESP1);
    resp->r[2] = b_in(emmc, EMMC_RESP2);
    resp->r[3] = b_in(emmc, EMMC_RESP3);
  }

  return 0;
}

static int bcmemmc_cmd(device_t *cdev, emmc_cmd_t cmd, uint32_t arg1,
                       uint32_t arg2, emmc_resp_t *resp) {
  assert(cdev->parent && cdev->parent->driver == &bcmemmc_driver);
  bcmemmc_state_t *state = (bcmemmc_state_t *)cdev->parent->state;

  if (cmd.flags & EMMC_F_APP)
    bcmemmc_cmd(cdev, EMMC_CMD(APP_CMD), state->rca << 16, 0, NULL);

  uint32_t code = encode_cmd(cmd);
  return bcmemmc_cmd_code(cdev->parent, code, arg1, arg2, resp);
}

static int bcmemmc_read(device_t *cdev, void *buf, size_t len, size_t *read) {
  assert(cdev->parent && cdev->parent->driver == &bcmemmc_driver);
  device_t *emmcdev = cdev->parent;
  bcmemmc_state_t *state = (bcmemmc_state_t *)emmcdev->state;
  resource_t *emmc = state->emmc;

  assert((len & 0x03) == 0); /* Assert multiple of 32 bits */

  /* A very simple transfer */
  for (size_t i = 0; i < (len >> 2); i++)
    ((uint32_t *)buf)[i] = b_in(emmc, EMMC_DATA);

  /* TODO (mohr): check wether the transfer fully succeeded! */
  if (read)
    *read = len;
  return 0;
}

static int bcmemmc_write(device_t *cdev, const void *buf, size_t len,
                         size_t *wrote) {
  assert(cdev->parent && cdev->parent->driver == &bcmemmc_driver);
  device_t *emmcdev = cdev->parent;
  bcmemmc_state_t *state = (bcmemmc_state_t *)emmcdev->state;
  resource_t *emmc = state->emmc;

  assert((len & 0x03) == 0); /* Assert multiple of 32 bits */

  /* A very simple transfer */
  for (size_t i = 0; i < (len >> 2); i++)
    b_out(emmc, EMMC_DATA, ((uint32_t *)buf)[i]);

  /* TODO (mohr): check wether the transfer fully succeeded! */
  if (wrote)
    *wrote = len;
  return 0;
}

#define GPFSEL4 0x0010
#define GPFSEL5 0x0014
#define GPPUDCLK1 0x009C
#define GPHEN1 0x0068

static void emmc_gpio_init(device_t *dev) {
  bcmemmc_state_t *state = (bcmemmc_state_t *)dev->state;
  resource_t *gpio = state->gpio;
  int64_t r = 0;
  /* GPIO_CD */
  r = b_in(gpio, GPFSEL4);
  r &= ~(7 << (7 * 3));
  b_out(gpio, GPFSEL4, r);
  b_out(gpio, GPPUD, 2);
  delay(150); /* ! */
  b_out(gpio, GPPUDCLK1, 1 << 15);
  delay(150); /* ! */
  b_out(gpio, GPPUD, 0);
  b_out(gpio, GPPUDCLK1, 0);
  r = b_in(gpio, GPHEN1);
  r |= 1 << 15;
  b_out(gpio, GPHEN1, r);

  /* GPIO_CLK, GPIO_CMD */
  r = b_in(gpio, GPFSEL4);
  r |= (7 << (8 * 3)) | (7 << (9 * 3));
  b_out(gpio, GPFSEL4, r);
  b_out(gpio, GPPUD, 2);
  delay(150); /* ! */
  b_out(gpio, GPPUDCLK1, (1 << 16) | (1 << 17));
  delay(150); /* ! */
  b_out(gpio, GPPUD, 0);
  b_out(gpio, GPPUDCLK1, 0);

  /* GPIO_DAT0, GPIO_DAT1, GPIO_DAT2, GPIO_DAT3 */
  r = b_in(gpio, GPFSEL5);
  r |= (7 << (0 * 3)) | (7 << (1 * 3)) | (7 << (2 * 3)) | (7 << (3 * 3));
  b_out(gpio, GPFSEL5, r);
  b_out(gpio, GPPUD, 2);
  delay(150); /* ! */
  b_out(gpio, GPPUDCLK1, (1 << 18) | (1 << 19) | (1 << 20) | (1 << 21));
  delay(150); /* ! */
  b_out(gpio, GPPUD, 0);
  b_out(gpio, GPPUDCLK1, 0);
}

static int bcmemmc_init(device_t *dev) {
  assert(dev->driver == (driver_t *)&bcmemmc_driver);
  bcmemmc_state_t *state = (bcmemmc_state_t *)dev->state;
  resource_t *emmc = state->emmc;
  int64_t r, cnt;

  state->host_version =
    (b_in(emmc, EMMC_SLOTISR_VER) & HOST_SPEC_NUM) >> HOST_SPEC_NUM_SHIFT;
  klog("EMMC: GPIO set up");
  /* Reset the card. */
  b_out(emmc, EMMC_CONTROL0, 0);
  b_set(emmc, EMMC_CONTROL1, C1_SRST_HC);
  cnt = 10000;
  do {
    delay(30); /* ! */
  } while ((b_in(emmc, EMMC_CONTROL1) & C1_SRST_HC) && cnt--);
  if (cnt <= 0) {
    klog("ERROR: failed to reset EMMC");
    return ETIMEDOUT;
  }
  klog("EMMC: reset OK");
  b_set(emmc, EMMC_CONTROL1, C1_CLK_INTLEN | C1_TOUNIT_MAX);
  delay(30); /* ! */
  ;
  // Set clock to setup frequency.
  if ((r = bcmemmc_clk(dev, 400000)))
    return r;
  b_out(emmc, EMMC_INT_EN,
        INT_CMD_DONE | INT_DATA_DONE | INT_READ_RDY | INT_WRITE_RDY |
          INT_CMD_TIMEOUT | INT_DATA_TIMEOUT);
  b_out(emmc, EMMC_INT_MASK,
        INT_CMD_DONE | INT_DATA_DONE | INT_READ_RDY | INT_WRITE_RDY |
          INT_CMD_TIMEOUT | INT_DATA_TIMEOUT);
  state->rca = state->sd_err = 0;

  return 0;
}

static int bcmemmc_probe(device_t *dev) {
  return dev->unit == 3;
}

DEVCLASS_DECLARE(emmc);

static int bcmemmc_attach(device_t *dev) {
  assert(dev->driver == (driver_t *)&bcmemmc_driver);
  bcmemmc_state_t *state = (bcmemmc_state_t *)dev->state;

  state->gpio = device_take_memory(dev, 0, RF_ACTIVE);
  state->emmc = device_take_memory(dev, 1, RF_ACTIVE);

  spin_init(&state->slock, 0);
  cv_init(&state->cv_response, "SD card response conditional variable");

  b_out(state->emmc, EMMC_INTERRUPT, 0xff);
  emmc_gpio_init(dev);

  state->irq = device_take_irq(dev, 2, RF_ACTIVE);
  bus_intr_setup(dev, state->irq, bcmemmc_intr_filter, NULL, state,
                 "e.MMMC interrupt");

  int init_res = bcmemmc_init(dev);
  if (init_res) {
    klog("e.MMC initialzation failed with code %d.", init_res);
    return -1;
  }

  /* This is not a legitimate bus in a sense that it implements `DIF_BUS`, but
   * it should work nevertheless */
  device_t *child = device_add_child(dev, 0);
  child->bus = DEV_BUS_EMMC;
  child->devclass = &DEVCLASS(emmc);
  child->unit = 0;

  return bus_generic_probe(dev);
}

emmc_methods_t bcmemmc_emmc_if = {
  .send_cmd = bcmemmc_cmd,
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