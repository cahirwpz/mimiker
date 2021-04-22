#define KL_LOG KL_DEV

#include <sys/memdev.h>
#include <sys/mimiker.h>
#include <sys/rman.h>
#include <sys/devclass.h>
#include <sys/klog.h>
#include <sys/types.h>
#include <aarch64/gpio.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/time.h>
#include <sys/libkern.h>
#include <sys/callout.h>
#include <sys/condvar.h>
#include <sys/bus.h>
#include <dev/emmc.h>
#include <sys/errno.h>

/* TO USE eMMC card in qemu, use the following options:
 *  -drive if=sd,index=0,file=IMAGEFILE_NAME,format=raw
 */

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

//#define EMMC_STATUS 24

// command flags
#define CMD_APP_SPECIFIC 0x80000000
#define CMD_RSPNS_48 0x00020000
#define CMD_ERRORS_MASK 0xfff9c004
#define CMD_RCA_MASK 0xffff0000

// COMMANDs
#define CMD_GO_IDLE 0x00000000
#define CMD_ALL_SEND_CID 0x02010000
#define CMD_SEND_REL_ADDR 0x03020000
#define CMD_CARD_SELECT 0x07020000
#define CMD_SEND_IF_COND 0x08020000
//#define CMD_STOP_TRANS    0x0C030000
/* Turns out the broadcom module has rwo ways of handling responses */
#define CMD_STOP_TRANS 0x0C020000
#define CMD_READ_SINGLE 0x11220010
#define CMD_READ_MULTI 0x12220032
#define CMD_SET_BLOCKCNT 0x17020000

#define CMD_WRITE_SINGLE 0x18220000
#define CMD_WRITE_MULTI 0x19220022

#define CMD_APP_CMD 0x37000000
#define CMD_SET_BUS_WIDTH (0x06020000 | CMD_APP_SPECIFIC)
#define CMD_SEND_OP_COND (0x29020000 | CMD_APP_SPECIFIC)
#define CMD_SEND_SCR (0x33220010 | CMD_APP_SPECIFIC)

// STATUS register settings
#define SR_READ_AVAILABLE 0x00000800
#define SR_WRITE_AVAILABLE 0x00000400
#define SR_DAT_INHIBIT 0x00000002
#define SR_CMD_INHIBIT 0x00000001
#define SR_APP_CMD 0x00000020

// INTERRUPT register settings
#define INT_DATA_TIMEOUT 0x00100000
#define INT_CMD_TIMEOUT 0x00010000
#define INT_READ_RDY 0x00000020
#define INT_WRITE_RDY 0x00000010
#define INT_DATA_DONE 0x00000002
#define INT_CMD_DONE 0x00000001

#define INT_ERROR_MASK 0x017E8000

// CONTROL register settings
#define C0_SPI_MODE_EN 0x00100000
#define C0_HCTL_HS_EN 0x00000004
#define C0_HCTL_DWITDH 0x00000002
#define C0_HCTL_8BIT 0x00000020

#define C1_SRST_DATA 0x04000000
#define C1_SRST_CMD 0x02000000
#define C1_SRST_HC 0x01000000
#define C1_TOUNIT_DIS 0x000f0000
#define C1_TOUNIT_MAX 0x000e0000
#define C1_CLK_GENSEL 0x00000020
#define C1_CLK_EN 0x00000004
#define C1_CLK_STABLE 0x00000002
#define C1_CLK_INTLEN 0x00000001

// SLOTISR_VER values
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

#define SD_OK 0
#define SD_TIMEOUT 1 /* eMMC device timed out */
#define SD_ERROR 2
#define SD_NO_RESPONSE 4 /* eMMC controller is not responding */
#define SD_UNEXPECTED_INTR 8

static driver_t emmc_driver;

typedef struct emmc_state {
  resource_t *gpio;
  resource_t *emmc;
  resource_t *irq;
  condvar_t cv_response;
  spin_t slock;
  volatile uint32_t intr_response;
  uint64_t sd_scr[2];
  uint64_t sd_ocr;
  uint64_t sd_rca;
  uint64_t sd_err;
  uint64_t sd_hv;
  uint32_t last_lba;
  uint32_t last_code;
  int last_code_cnt;
  uint32_t intr_flags;
  int twait;
} emmc_state_t;

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
 * \return 0 on success, SD_ERROR on error, SD_TIMEOUT on eMMC device timeout,
 * SD_NO_RESPONSE on eMMC controller timeout.
 */
static int32_t sdhc_intr_wait(device_t *dev, uint32_t mask) {
  emmc_state_t *state = (emmc_state_t *)dev->state;
  resource_t *emmc = state->emmc;
  uint32_t m = 0;

  WITH_SPIN_LOCK (&state->slock) {
  emmc_restart_intr_wait:
    while (mask) {
      if (state->intr_flags & mask) {
        m = mask;
        mask &= ~state->intr_flags;
        state->intr_flags &= ~m;
        goto emmc_restart_intr_wait;
      }
      /* Busy-wait for a while. Should be good enough if the card works fine */
      for (int i = 0; i < 64; i++) {
        if ((state->intr_flags = b_in(emmc, EMMC_INTERRUPT)))
          goto emmc_restart_intr_wait;
      }
      /* Sleep for a while if no interrupts have been received so far */
      if (!state->intr_flags) {
        state->twait = 1;
        if (cv_wait_timed(&state->cv_response, &state->slock, 10000)) {
          state->twait = 0;
          b_out(emmc, EMMC_INTERRUPT, 0xffffffff);
          return SD_TIMEOUT;
        }
        state->twait = 0;
      }
      if (state->intr_flags & INT_ERROR_MASK) {
        state->intr_flags = 0;
        return SD_ERROR;
      }
      m = state->intr_flags;
      mask &= ~state->intr_flags;
      state->intr_flags &= ~m;
    }
  }
  return SD_OK;
}

static int sdhc_wait(device_t *cdev, emmc_wait_flags_t wflags) {
  assert(cdev->parent && cdev->parent->driver == &emmc_driver);

  uint32_t mask = emmc_wait_flags_to_hwflags(wflags);
  return sdhc_intr_wait(cdev->parent, mask);
}

/**
 * \brief Wait for status
 */
int sdhc_status(device_t *dev, uint32_t mask) {
  emmc_state_t *state = (emmc_state_t *)dev->state;
  resource_t *emmc = state->emmc;
  int32_t cnt = 10000;
  do {
    // sleep_ms(1);
  } while ((b_in(emmc, EMMC_STATUS) & mask) && cnt--);
  return cnt <= 0;
}

static intr_filter_t sdhc_intr_filter(void *data) {
  emmc_state_t *state = (emmc_state_t *)data;
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
int32_t sdhc_clk(device_t *dev, uint32_t f) {
  emmc_state_t *state = (emmc_state_t *)dev->state;
  resource_t *emmc = state->emmc;
  uint32_t d, c = 41666666 / f, x, s = 32, h = 0;
  int32_t cnt = 100000;
  while ((b_in(emmc, EMMC_STATUS) & (SR_CMD_INHIBIT | SR_DAT_INHIBIT)) && cnt--)
    delay(3); /* ! */
  if (cnt <= 0) {
    klog("ERROR: timeout waiting for inhibit flag\n");
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
  if (state->sd_hv > HOST_SPEC_V2)
    d = c;
  else
    d = (1 << s);
  if (d <= 2) {
    d = 2;
    s = 0;
  }
  klog("sdhc_clk divisor %p, shift %p\n", d, s);
  if (state->sd_hv > HOST_SPEC_V2)
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
    klog("ERROR: failed to get stable clock\n");
    return SD_ERROR;
  }
  return SD_OK;
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

static int sdhc_get_prop(device_t *cdev, uint32_t id, uint64_t *var) {
  assert(cdev->parent && cdev->parent->driver == &emmc_driver);
  emmc_state_t *state = (emmc_state_t *)cdev->parent->state;
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
      *var = state->sd_rca;
      return 0;
    default:
      return ENODEV;
  }
}

static int sdhc_set_prop(device_t *cdev, uint32_t id, uint64_t var) {
  assert(cdev->parent && cdev->parent->driver == &emmc_driver);
  emmc_state_t *state = (emmc_state_t *)cdev->parent->state;
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
      return sdhc_clk(cdev->parent, var);
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
      state->sd_rca = var;
      return 0;
    default:
      return ENODEV;
  }
}

static int sdhc_cmd_code(device_t *dev, uint32_t code, uint32_t arg1,
                         uint32_t arg2, emmc_resp_t *resp) {
  emmc_state_t *state = (emmc_state_t *)dev->state;
  resource_t *emmc = state->emmc;

  uint32_t r = 0;
  state->sd_err = SD_OK;

  if (sdhc_status(dev, SR_CMD_INHIBIT)) {
    klog("ERROR: EMMC busy");
    return EBUSY;
  }

  /* klog("EMMC: Sending command %p, arg1 %p, arg2 %p", code, arg1, arg2); */

  b_out(emmc, EMMC_ARG1, arg1);
  b_out(emmc, EMMC_ARG2, arg2);
  b_out(emmc, EMMC_CMDTM, code);
  if ((r = sdhc_intr_wait(dev, INT_CMD_DONE))) {
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

static int sdhc_cmd(device_t *cdev, emmc_cmd_t cmd, uint32_t arg1,
                    uint32_t arg2, emmc_resp_t *resp) {
  assert(cdev->parent && cdev->parent->driver == &emmc_driver);
  emmc_state_t *state = (emmc_state_t *)cdev->parent->state;

  if (cmd.flags & EMMC_F_APP)
    sdhc_cmd(cdev, EMMC_CMD(APP_CMD), state->sd_rca << 16, 0, NULL);

  uint32_t code = encode_cmd(cmd);
  return sdhc_cmd_code(cdev->parent, code, arg1, arg2, resp);
}

/**
 * \brief Send a command
 * \param dev eMMC device
 * \param mask expected interrupts
 * \param clear additional interrupt bits to be cleared
 */
static int sdhc_cmd_old(device_t *dev, uint32_t code, uint32_t arg,
                        uint32_t mask) {
  emmc_state_t *state = (emmc_state_t *)dev->state;
  resource_t *emmc = state->emmc;

  uint32_t r = 0;

  state->sd_err = SD_OK;
  /* Application-specific commands require CMD55 to be sent beforehand */
  if (code & CMD_APP_SPECIFIC) {
    r = sdhc_cmd_old(dev, CMD_APP_CMD | (state->sd_rca ? CMD_RSPNS_48 : 0),
                     state->sd_rca, 0);
    if (state->sd_rca && !r) {
      klog("ERROR: failed to send SD APP command");
      state->sd_err = SD_ERROR;
      return 0;
    }
    code &= ~CMD_APP_SPECIFIC;
  }
  if (sdhc_status(dev, SR_CMD_INHIBIT)) {
    klog("ERROR: EMMC busy\n");
    state->sd_err = SD_TIMEOUT;
    return 0;
  }
  klog("EMMC: Sending command %p, arg %p", code, arg);
  WITH_SPIN_LOCK (&state->slock) {
    b_out(emmc, EMMC_ARG1, arg);
    b_out(emmc, EMMC_CMDTM, code);
    delay(150);

    if ((r = sdhc_intr_wait(dev, INT_CMD_DONE | mask))) {
      klog("ERROR: failed to send EMMC command");
      state->sd_err = r;
      return 0;
    }
  }

  r = b_in(emmc, EMMC_RESP0);
  if (code == CMD_GO_IDLE || code == CMD_APP_CMD) {
    if (state->last_code_cnt == 0)
      klog("EMMC: Sending command %s, arg %p", "GO IDLE", arg);
    return 0;
  }

  else if (code == (CMD_APP_CMD | CMD_RSPNS_48)) {
    if (state->last_code_cnt == 0)
      klog("EMMC: Sending command %s, arg %p", "APP", arg);
    return r & SR_APP_CMD;
  }

  else if (code == CMD_SEND_OP_COND) {
    if (state->last_code_cnt == 0)
      klog("EMMC: Sending command %s, arg %p", "OP COND", arg);
    return r;
  }

  else if (code == CMD_SEND_IF_COND) {
    if (state->last_code_cnt == 0)
      klog("EMMC: Sending command %s, arg %p", "IF COND", arg);
    return r == arg ? SD_OK : SD_ERROR;
  }

  else if (code == CMD_ALL_SEND_CID) {
    r |= b_in(emmc, EMMC_RESP3);
    r |= b_in(emmc, EMMC_RESP2);
    r |= b_in(emmc, EMMC_RESP1);

    if (state->last_code_cnt == 0)
      klog("EMMC: Sending command %s, arg %p", "SEND CID", arg);
    return r;
  }

  else if (code == CMD_SEND_REL_ADDR) {
    state->sd_err = (((r & 0x1fff)) | ((r & 0x2000) << 6) |
                     ((r & 0x4000) << 8) | ((r & 0x8000) << 8)) &
                    CMD_ERRORS_MASK;

    if (state->last_code_cnt == 0)
      klog("EMMC: Sending command %s, arg %p", "ADDRESS", arg);
    return r & CMD_RCA_MASK;
  }
  return r & CMD_ERRORS_MASK;
  // make gcc happy
  return 0;
}

static int sdhc_read(device_t *cdev, void *buf, size_t len, size_t *read) {
  assert(cdev->parent && cdev->parent->driver == &emmc_driver);
  device_t *emmcdev = cdev->parent;
  emmc_state_t *state = (emmc_state_t *)emmcdev->state;
  resource_t *emmc = state->emmc;

  assert((len & 0x03) == 0); /* Assert multiple of 32 bits */
  /* uint64_t blksz = 0;
  emmc_get_prop(cdev, EMMC_PROP_RW_BLKSIZE, &blksz);
  if (len & (blksz - 1))
    return EINVAL; */ /* Length is not a multiple of block size */

  /* A very simple transfer */
  for (size_t i = 0; i < (len >> 2); i++)
    ((uint32_t *)buf)[i] = b_in(emmc, EMMC_DATA);

  /* TODO (mohr): check wether the transfer fully succeeded! */
  *read = len;
  return 0;
}

static int sdhc_write(device_t *cdev, const void *buf, size_t len,
                      size_t *wrote) {
  assert(cdev->parent && cdev->parent->driver == &emmc_driver);
  device_t *emmcdev = cdev->parent;
  emmc_state_t *state = (emmc_state_t *)emmcdev->state;
  resource_t *emmc = state->emmc;

  assert((len & 0x03) == 0); /* Assert multiple of 32 bits */
  uint64_t blksz = 0;
  emmc_get_prop(cdev, EMMC_PROP_RW_BLKSIZE, &blksz);
  if (len & (blksz - 1))
    return EINVAL; /* Length is not a multiple of block size */

  /* A very simple transfer */
  for (size_t i = 0; i < (len >> 2); i++)
    b_out(emmc, EMMC_DATA, ((uint32_t *)buf)[i]);

  /* TODO (mohr): check wether the transfer fully succeeded! */
  return len;
}

#define GPFSEL4 0x0010
#define GPFSEL5 0x0014
#define GPPUDCLK0 0x0098
#define GPPUDCLK1 0x009C
#define GPHEN0 0x0064
#define GPHEN1 0x0068

static void emmc_gpio_init(device_t *dev) {
  emmc_state_t *state = (emmc_state_t *)dev->state;
  resource_t *gpio = state->gpio;
  int64_t r = 0;
  // GPIO_CD
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

  // GPIO_CLK, GPIO_CMD
  r = b_in(gpio, GPFSEL4);
  r |= (7 << (8 * 3)) | (7 << (9 * 3));
  b_out(gpio, GPFSEL4, r);
  b_out(gpio, GPPUD, 2);
  delay(150); /* ! */
  b_out(gpio, GPPUDCLK1, (1 << 16) | (1 << 17));
  delay(150); /* ! */
  b_out(gpio, GPPUD, 0);
  b_out(gpio, GPPUDCLK1, 0);

  // GPIO_DAT0, GPIO_DAT1, GPIO_DAT2, GPIO_DAT3
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

/**
 * initialize EMMC to read SDHC card
 */
static int emmc_init(device_t *dev) {
  assert(dev->driver == (driver_t *)&emmc_driver);
  emmc_state_t *state = (emmc_state_t *)dev->state;
  resource_t *emmc = state->emmc;
  int64_t r, cnt, ccs = 0;

  state->sd_hv =
    (b_in(emmc, EMMC_SLOTISR_VER) & HOST_SPEC_NUM) >> HOST_SPEC_NUM_SHIFT;
  klog("EMMC: GPIO set up\n");
  // Reset the card.
  b_out(emmc, EMMC_CONTROL0, 0);
  b_set(emmc, EMMC_CONTROL1, C1_SRST_HC);
  cnt = 10000;
  do {
    delay(30); /* ! */
  } while ((b_in(emmc, EMMC_CONTROL1) & C1_SRST_HC) && cnt--);
  if (cnt <= 0) {
    klog("ERROR: failed to reset EMMC\n");
    return SD_ERROR;
  }
  klog("EMMC: reset OK\n");
  b_set(emmc, EMMC_CONTROL1, C1_CLK_INTLEN | C1_TOUNIT_MAX);
  delay(30); /* ! */
  ;
  // Set clock to setup frequency.
  if ((r = sdhc_clk(dev, 400000)))
    return r;
  b_out(emmc, EMMC_INT_EN,
        INT_CMD_DONE | INT_DATA_DONE | INT_READ_RDY | INT_WRITE_RDY |
          INT_CMD_TIMEOUT | INT_DATA_TIMEOUT);
  b_out(emmc, EMMC_INT_MASK,
        INT_CMD_DONE | INT_DATA_DONE | INT_READ_RDY | INT_WRITE_RDY |
          INT_CMD_TIMEOUT | INT_DATA_TIMEOUT);
  state->sd_scr[0] = state->sd_scr[1] = state->sd_rca = state->sd_err = 0;

  return SD_OK;

  sdhc_cmd_old(dev, CMD_GO_IDLE, 0, 0);
  if (state->sd_err)
    return state->sd_err;

  sdhc_cmd_old(dev, CMD_SEND_IF_COND, 0x000001AA, 0);
  if (state->sd_err)
    return state->sd_err;
  cnt = 6;
  r = 0;
  while (!(r & ACMD41_CMD_COMPLETE) && cnt--) {
    delay(400); /* ! */
    r = sdhc_cmd_old(dev, CMD_SEND_OP_COND, ACMD41_ARG_HC, 0);
    klog("EMMC: CMD_SEND_OP_COND returned ");
    if (r & ACMD41_CMD_COMPLETE)
      klog("COMPLETE ");
    if (r & ACMD41_VOLTAGE)
      klog("VOLTAGE ");
    if (r & ACMD41_CMD_CCS)
      klog("CCS ");
    klog(" %p \n", r);
    if (state->sd_err != (uint64_t)SD_TIMEOUT && state->sd_err != SD_OK) {
      klog("ERROR: EMMC ACMD41 returned error\n");
      return state->sd_err;
    }
  }
  if (!(r & ACMD41_CMD_COMPLETE) || !cnt)
    return SD_TIMEOUT;
  if (!(r & ACMD41_VOLTAGE))
    return SD_ERROR;
  if (r & ACMD41_CMD_CCS)
    ccs = SCR_SUPP_CCS;

  sdhc_cmd_old(dev, CMD_ALL_SEND_CID, 0, 0);

  state->sd_rca = sdhc_cmd_old(dev, CMD_SEND_REL_ADDR, 0, 0);
  klog("EMMC: CMD_SEND_REL_ADDR returned %p \n", state->sd_rca);
  if (state->sd_err)
    return state->sd_err;

  if ((r = sdhc_clk(dev, 25000000)))
    return r;

  sdhc_cmd_old(dev, CMD_CARD_SELECT, state->sd_rca, 0);
  if (state->sd_err)
    return state->sd_err;
  if (sdhc_status(dev, SR_DAT_INHIBIT))
    return SD_TIMEOUT;
  b_out(emmc, EMMC_BLKSIZECNT, (1 << 16) | 8);
  sdhc_cmd_old(dev, CMD_SEND_SCR, 0, INT_READ_RDY);
  if (state->sd_err)
    return state->sd_err;

  r = 0;
  cnt = 100000;
  WITH_SPIN_LOCK (&state->slock) {
    while (r < 2 && cnt) {
      if (b_in(emmc, EMMC_STATUS) & SR_READ_AVAILABLE)
        state->sd_scr[r++] = b_in(emmc, EMMC_DATA);
      else
        cnt--;
    }
    if (sdhc_intr_wait(dev, INT_DATA_DONE))
      return state->sd_err;
  }
  if (r != 2)
    return SD_TIMEOUT;
  if (state->sd_scr[0] & SCR_SD_BUS_WIDTH_4) {
    sdhc_cmd_old(dev, CMD_SET_BUS_WIDTH, state->sd_rca | 2, 0);
    if (state->sd_err)
      return state->sd_err;
    b_set(emmc, EMMC_CONTROL0, C0_HCTL_DWITDH);
  }
  // add software flag
  klog("EMMC: supports");
  if (state->sd_scr[0] & SCR_SUPP_SET_BLKCNT)
    klog(" SET_BLKCNT");
  if (ccs)
    klog(" CCS");
  state->sd_scr[0] &= ~SCR_SUPP_CCS;
  state->sd_scr[0] |= ccs;

  state->sd_rca = 0;

  return SD_OK;
}

/*----------------------------------------------------------------------------*/

static int sdhc_probe(device_t *dev) {
  /* return 0; */
  return dev->unit == 3;
}

DEVCLASS_DECLARE(emmc);

static int sdhc_attach(device_t *dev) {
  assert(dev->driver == (driver_t *)&emmc_driver);
  emmc_state_t *state = (emmc_state_t *)dev->state;
  state->last_lba = 0;
  state->last_code = 0;
  state->last_code_cnt = 0;

  state->gpio = device_take_memory(dev, 0, RF_ACTIVE);
  state->emmc = device_take_memory(dev, 1, RF_ACTIVE);

  spin_init(&state->slock, 0);
  cv_init(&state->cv_response, "SD card response conditional variable");

  b_out(state->emmc, EMMC_INTERRUPT, 0xff);
  emmc_gpio_init(dev);

  state->irq = device_take_irq(dev, 2, RF_ACTIVE);
  bus_intr_setup(dev, state->irq, sdhc_intr_filter, NULL, state,
                 "e.MMMC interrupt");

  int init_res = emmc_init(dev);
  if (init_res != SD_OK) {
    klog("e.MMC initialzation failed with code %d.", init_res);
    return -1;
  }

  /* This is not a bus in a sensse that it implements `DIF_BUS`, but this should
   * work nevertheless */
  device_t *child = device_add_child(dev, 0);
  child->bus = DEV_BUS_EMMC;
  child->devclass = &DEVCLASS(emmc);
  child->unit = 0;

  return bus_generic_probe(dev);
}

#undef b_in
#undef b_out
#undef b_set
#undef b_clr

emmc_methods_t emmc_emmc_if = {
  .send_cmd = sdhc_cmd,
  .wait = sdhc_wait,
  .read = sdhc_read,
  .write = sdhc_write,
  .get_prop = sdhc_get_prop,
  .set_prop = sdhc_set_prop,
};

static driver_t emmc_driver = {
  .desc = "e.MMC memory card driver",
  .size = sizeof(emmc_state_t),
  .probe = sdhc_probe,
  .attach = sdhc_attach,
  .pass = SECOND_PASS,
  .interfaces =
    {
      [DIF_EMMC] = &emmc_emmc_if,
    },
};

DEVCLASS_ENTRY(root, emmc_driver);