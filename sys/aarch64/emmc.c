#include <sys/memdev.h>
#include <sys/mimiker.h>
#include <sys/resource.h>
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

/* TO USE eMMC card in qemu, use the following options:
 *  -drive if=sd,index=0,file=IMAGEFILE_NAME,format=raw
 */

#define GPPUD 0x0094

#define EMMC_ARG2        0x0000
#define EMMC_BLKSIZECNT  0x0004
#define EMMC_ARG1        0x0008
#define EMMC_CMDTM       0x000C
#define EMMC_RESP0       0x0010
#define EMMC_RESP1       0x0014
#define EMMC_RESP2       0x0018
#define EMMC_RESP3       0x001C
#define EMMC_DATA        0x0020
#define EMMC_STATUS      0x0024
#define EMMC_CONTROL0    0x0028
#define EMMC_CONTROL1    0x002C
#define EMMC_INTERRUPT   0x0030
#define EMMC_INT_MASK    0x0034
#define EMMC_INT_EN      0x0038
#define EMMC_CONTROL2    0x003C
#define EMMC_SLOTISR_VER 0x00FC

//#define EMMC_STATUS 24

// command flags
#define CMD_APP_SPECIFIC    0x80000000
#define CMD_RSPNS_48        0x00020000
#define CMD_ERRORS_MASK     0xfff9c004
#define CMD_RCA_MASK        0xffff0000

// COMMANDs
#define CMD_GO_IDLE       0x00000000
#define CMD_ALL_SEND_CID  0x02010000
#define CMD_SEND_REL_ADDR 0x03020000
#define CMD_CARD_SELECT   0x07030000
#define CMD_SEND_IF_COND  0x08020000
#define CMD_STOP_TRANS    0x0C030000
#define CMD_READ_SINGLE   0x11220010
#define CMD_READ_MULTI    0x12220032
#define CMD_SET_BLOCKCNT  0x17020000

#define CMD_WRITE_SINGLE  0x18220000
#define CMD_WRITE_MULTI   0x19220022

#define CMD_APP_CMD        0x37000000
#define CMD_SET_BUS_WIDTH (0x06020000 | CMD_APP_SPECIFIC)
#define CMD_SEND_OP_COND  (0x29020000 | CMD_APP_SPECIFIC)
#define CMD_SEND_SCR      (0x33220010 | CMD_APP_SPECIFIC)

// STATUS register settings
#define SR_READ_AVAILABLE 0x00000800
#define SR_DAT_INHIBIT    0x00000002
#define SR_CMD_INHIBIT    0x00000001
#define SR_APP_CMD        0x00000020

// INTERRUPT register settings
#define INT_DATA_TIMEOUT 0x00100000
#define INT_CMD_TIMEOUT  0x00010000
#define INT_READ_RDY     0x00000020
#define INT_WRITE_RDY    0x00000010
#define INT_DATA_DONE    0x00000002
#define INT_CMD_DONE     0x00000001

#define INT_ERROR_MASK   0x017E8000

// CONTROL register settings
#define C0_SPI_MODE_EN 0x00100000
#define C0_HCTL_HS_EN  0x00000004
#define C0_HCTL_DWITDH 0x00000002

#define C1_SRST_DATA  0x04000000
#define C1_SRST_CMD   0x02000000
#define C1_SRST_HC    0x01000000
#define C1_TOUNIT_DIS 0x000f0000
#define C1_TOUNIT_MAX 0x000e0000
#define C1_CLK_GENSEL 0x00000020
#define C1_CLK_EN     0x00000004
#define C1_CLK_STABLE 0x00000002
#define C1_CLK_INTLEN 0x00000001

// SLOTISR_VER values
#define HOST_SPEC_NUM 0x00ff0000
#define HOST_SPEC_NUM_SHIFT 16
#define HOST_SPEC_V3 2
#define HOST_SPEC_V2 1
#define HOST_SPEC_V1 0

// SCR flags
#define SCR_SD_BUS_WIDTH_4  0x00000400
#define SCR_SUPP_SET_BLKCNT 0x02000000
// added by my driver
#define SCR_SUPP_CCS 0x00000001

#define ACMD41_VOLTAGE      0x00ff8000
#define ACMD41_CMD_COMPLETE 0x80000000
#define ACMD41_CMD_CCS      0x40000000
#define ACMD41_ARG_HC       0x51ff8000

#define SD_OK 0
#define SD_TIMEOUT -1
#define SD_ERROR -2

driver_t emmc_driver;

typedef struct emmc_state {
    resource_t *gpio;
    resource_t *emmc;
    resource_t *irq;
    condvar_t cv_response;
    spin_t slock;
    volatile uint32_t intr_mask;
    volatile uint32_t intr_clear;
    volatile uint32_t intr_response;
    uint64_t sd_scr[2];
    uint64_t sd_ocr;
    uint64_t sd_rca;
    uint64_t sd_err;
    uint64_t sd_hv;
    uint32_t last_lba;
    uint32_t last_code;
    int last_code_cnt;
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
                   : [count] "+r"(count));
}

/* Stolen from MichalBlk's mdelay fork.
 * This should be a built-in kernel feature adn it's here only as a placeholder
 * for driver prototyping purposes. */
/* [--------------------------------------------------------------------------*/
static void sleep_ms_timeout(__unused void *arg) {
  /* Nothing to do here. */
}

static void sleep_ms(useconds_t ms) {
  callout_t handle;
  bzero(&handle, sizeof(callout_t));
  callout_setup_relative(&handle, ms, sleep_ms_timeout, NULL);
  callout_drain(&handle);
}
/*--------------------------------------------------------------------------] */

/**
 * \brief Wait for (a set of) interrupt
 * \warning This routine needs to be called with driver's spinlock held!
 * \param dev eMMC device
 * \param mask expected interrupts
 * \param clear additional interrupt bits to be cleared
 * \return 0 on success, SD_ERROR on error, SD_TIMEOUT on timeout
 */
static int32_t emmc_intr_wait(device_t * dev, uint32_t mask, uint32_t clear) {
  emmc_state_t *state = (emmc_state_t *)dev->state;
  uint32_t resp = 0;
    state->intr_mask = mask;
    state->intr_clear = clear | mask;
    state->intr_response = (uint32_t)(-4);
    while (state->intr_response == (uint32_t)(-4)) {
      state->intr_response = (uint32_t)(-4);
      cv_wait(&state->cv_response, &state->slock);
    }
    resp = state->intr_response;
    state->intr_clear = 0x00000000;
  return (int32_t)resp;
}

/**
 * \brief Wait for status
 */
int emmc_status(device_t *dev, uint32_t mask) {
  emmc_state_t *state = (emmc_state_t *)dev->state;
  resource_t *emmc = state->emmc;
  int32_t cnt = 500000;
  do {
    sleep_ms(1);
  } while ((b_in(emmc, EMMC_STATUS) & mask) && cnt--);
  return cnt <= 0;
}

/**
 * \brief Interrupt handler (filter)
 * \param data eMMC device state
 * \details Wakes up a the driver thread if interrupt conditions are met.
 */
static intr_filter_t emmc_intr_filter(void *data) {
  emmc_state_t *state = (emmc_state_t *)data;
  resource_t *emmc = state->emmc;
  WITH_SPIN_LOCK(&state->slock) {
    uint32_t r = 0;
    uint32_t m = state->intr_mask;
    uint32_t c = state->intr_clear | INT_ERROR_MASK;
    
    r = b_in(emmc, EMMC_INTERRUPT);
    uint32_t resp = 0;
    if ((r & INT_CMD_TIMEOUT) || (r & INT_DATA_TIMEOUT)) {
      resp = SD_TIMEOUT;
    } else if (r & INT_ERROR_MASK) {
      resp = SD_ERROR;
    } else {
      resp = 0;
    }
    /* Interrupts need to be cleared manually */
    b_out(emmc, EMMC_INTERRUPT, c);
    /* Wake up the thread if all expected interrupts have been received */
    if ((r & m) == m) {
      state->intr_response = resp;
      cv_signal(&state->cv_response);
    }
    /* Remove already received interrupts from the expected interrupts set */
    state->intr_mask &= ~r;
  }

  return IF_FILTERED;
}

/**
 * \brief Send a command
 * \param dev eMMC device
 * \param mask expected interrupts
 * \param clear additional interrupt bits to be cleared
 */
uint32_t emmc_cmd(device_t *dev, uint32_t code, uint32_t arg,
                  uint32_t mask, uint32_t clear) {
  emmc_state_t *state = (emmc_state_t *)dev->state;
  resource_t *emmc = state->emmc;
  if (state->last_code == code)
    state->last_code_cnt++;
  else
    state->last_code_cnt = 0;

  state->last_code = code;

  uint32_t r = 0;
  state->sd_err = SD_OK;
  /* Application-specific commands require CMD55 to be sent beforehand */
  if (code & CMD_APP_SPECIFIC) {
    r = emmc_cmd(dev, CMD_APP_CMD | (state->sd_rca ? CMD_RSPNS_48 : 0),
                 state->sd_rca, 0, 0);
    if (state->sd_rca && !r) {
      klog("ERROR: failed to send SD APP command\n");
      state->sd_err = SD_ERROR;
      return 0;
    }
    code &= ~CMD_APP_SPECIFIC;
  }
  if (emmc_status(dev, SR_CMD_INHIBIT)) {
    klog("ERROR: EMMC busy\n");
    state->sd_err = SD_TIMEOUT;
    return 0;
  }
  klog("EMMC: Sending command %p, arg %p\n", code, arg);
  WITH_SPIN_LOCK(&state->slock) {
    b_out(emmc, EMMC_INTERRUPT, INT_CMD_DONE);
    b_out(emmc, EMMC_ARG1, arg);
    b_out(emmc, EMMC_CMDTM, code);
    delay(150);
    
    if ((r = emmc_intr_wait(dev, INT_CMD_DONE | mask, clear))) {
      klog("ERROR: failed to send EMMC command\n");
      state->sd_err = r;
      return 0;
    }
  }

  r = b_in(emmc, EMMC_RESP0);
  if (code == CMD_GO_IDLE || code == CMD_APP_CMD) {
    if (state->last_code_cnt == 0)
      klog("EMMC: Sending command %s, arg %p\n", "GO IDLE", arg);
    return 0;
  }

  else if (code == (CMD_APP_CMD | CMD_RSPNS_48)) {
    if (state->last_code_cnt == 0)
      klog("EMMC: Sending command %s, arg %p\n", "APP", arg);
    return r & SR_APP_CMD;
  }

  else if (code == CMD_SEND_OP_COND) {
    if (state->last_code_cnt == 0)
      klog("EMMC: Sending command %s, arg %p\n", "OP COND", arg);
    return r;
  }

  else if (code == CMD_SEND_IF_COND) {
    if (state->last_code_cnt == 0)
      klog("EMMC: Sending command %s, arg %p\n", "IF COND", arg);
    return r == arg ? SD_OK : SD_ERROR;
  }

  else if (code == CMD_ALL_SEND_CID) {
    r |= b_in(emmc, EMMC_RESP3);
    r |= b_in(emmc, EMMC_RESP2);
    r |= b_in(emmc, EMMC_RESP1);

    if (state->last_code_cnt == 0)
      klog("EMMC: Sending command %s, arg %p\n", "SEND CID", arg);
    return r;
  }

  else if (code == CMD_SEND_REL_ADDR) {
    state->sd_err = (((r & 0x1fff)) | ((r & 0x2000) << 6) |
                    ((r & 0x4000) << 8) | ((r & 0x8000) << 8)) &
             CMD_ERRORS_MASK;

    if (state->last_code_cnt == 0)
      klog("EMMC: Sending command %s, arg %p\n", "ADDRESS", arg);
    return r & CMD_RCA_MASK;
  }
  return r & CMD_ERRORS_MASK;
  // make gcc happy
  return 0;
}

/**
 * read a block from sd card and return the number of bytes read
 * returns 0 on error.
 */
int emmc_read_block(device_t *dev, uint32_t lba, unsigned char *buffer,
                    uint32_t num) {
  assert(dev->driver == (driver_t *)&emmc_driver);
  emmc_state_t *state = (emmc_state_t *)dev->state;
  resource_t *emmc = state->emmc;
  uint32_t c = 0, d;
  if (num < 1)
    num = 1;
  if (lba != state->last_lba)
    klog("sd_readblock lba %p, num %p\n", lba, num);
  state->last_lba = lba;

  if (emmc_status(dev, SR_DAT_INHIBIT)) {
    state->sd_err = SD_TIMEOUT;
    return 0;
  }
  uint32_t *buf = (uint32_t *)buffer;
  if (state->sd_scr[0] & SCR_SUPP_CCS) {
    if (num > 1 && (state->sd_scr[0] & SCR_SUPP_SET_BLKCNT)) {
      emmc_cmd(dev, CMD_SET_BLOCKCNT, num, 0, 0);
      if (state->sd_err)
        return 0;
    }
    b_out(emmc, EMMC_BLKSIZECNT, (num << 16) | 512);
    emmc_cmd(dev, num == 1 ? CMD_READ_SINGLE : CMD_READ_MULTI, lba,
             INT_DATA_DONE, 0);
    if (state->sd_err)
      return 0;
  } else {
    b_out(emmc, EMMC_BLKSIZECNT, (1 << 16) | 512);
  }
  while (c < num) {
    if (!(state->sd_scr[0] & SCR_SUPP_CCS)) {
      emmc_cmd(dev, CMD_READ_SINGLE, (lba + c) * 512,
               INT_DATA_DONE | INT_READ_RDY, 0);
      if (state->sd_err)
        return 0;
    }
    for (d = 0; d < 128; d++)
      buf[d] = b_in(emmc, EMMC_DATA);
    c++;
    buf += 128;
  }
  if (num > 1 && !(state->sd_scr[0] & SCR_SUPP_SET_BLKCNT) &&
      (state->sd_scr[0] & SCR_SUPP_CCS))
    emmc_cmd(dev, CMD_STOP_TRANS, 0, INT_DATA_DONE, 0);
  return state->sd_err != SD_OK || c != num ? 0 : num * 512;
}

/**
 * Write a block to the sd card
 * returns 0 on error
 */
int emmc_write_block(device_t *dev, uint32_t lba, const uint8_t *buffer,
                  uint32_t num) {
  assert(dev->driver == (driver_t *)&emmc_driver);
  emmc_state_t *state = (emmc_state_t *)dev->state;
  resource_t *emmc = state->emmc;
  
  if (((uint64_t)buffer & (uint64_t)0x3) != 0)
    return 0;
  uint32_t *buf = (uint32_t *)buffer;
  uint32_t c = 0;
  int32_t d;
  if (num < 1)
    return 0;
  if (emmc_status(dev, SR_DAT_INHIBIT)) {
    state->sd_err = SD_TIMEOUT;
    return 0;
  }
  if (state->sd_scr[0] & SCR_SUPP_CCS) {
    if (num > 1 && (state->sd_scr[0] & SCR_SUPP_SET_BLKCNT)) {
      emmc_cmd(dev, CMD_SET_BLOCKCNT, num, INT_DATA_DONE, 0);
      if (state->sd_err)
        return 0;
    }
    b_out(emmc, EMMC_BLKSIZECNT, (num << 16) | 512);
    emmc_cmd(dev, num == 1 ? CMD_WRITE_SINGLE : CMD_WRITE_MULTI, lba,
             INT_DATA_DONE, 0);
    if (state->sd_err)
      return 0;
  } else {
    b_out(emmc, EMMC_BLKSIZECNT, (1 << 16) | 512);
  }
  while (c < num) {
    if (!(state->sd_scr[0] & SCR_SUPP_CCS)) {
      emmc_cmd(dev, CMD_WRITE_SINGLE, (lba + c) * 512,
               INT_WRITE_RDY, 0);
      if (state->sd_err)
        return 0;
      WITH_SPIN_LOCK(&state->slock) {
        for (d = 0; d < 128; d++)
          b_out(emmc, EMMC_DATA, buf[d]);
        if (!emmc_intr_wait(dev, INT_DATA_DONE, 0))
          return 1;
        if (state->sd_err)
          return 0;
      }
    }
    c++;
    buf += 128;
  }

  return 0;
}

/**
 * set SD clock to frequency in Hz
 */
int32_t sd_clk(device_t *dev, uint32_t f) {
  emmc_state_t *state = (emmc_state_t *)dev->state;
  resource_t *emmc = state->emmc;
  uint32_t d, c = 41666666 / f, x, s = 32, h = 0;
  int32_t cnt = 100000;
  while ((b_in(emmc, EMMC_STATUS) & (SR_CMD_INHIBIT | SR_DAT_INHIBIT)) && cnt--)
    delay(3);                               /* ! */
  if (cnt <= 0) {
    klog("ERROR: timeout waiting for inhibit flag\n");
    return SD_ERROR;
  }

  b_clr(emmc, EMMC_CONTROL1, C1_CLK_EN);
  delay(30);                               /* ! */
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
  klog("sd_clk divisor %p, shift %p\n", d, s);
  if (state->sd_hv > HOST_SPEC_V2)
    h = (d & 0x300) >> 2;
  d = (((d & 0x0ff) << 8) | h);
  b_out(emmc, EMMC_CONTROL1, (b_in(emmc, EMMC_CONTROL1) & 0xffff003f) | d);
  delay(30);                                 /* ! */
  b_set(emmc, EMMC_CONTROL1, C1_CLK_EN);
  delay(30);                                 /* ! */
  cnt = 10000;
  while (!(b_in(emmc, EMMC_CONTROL1) & C1_CLK_STABLE) && cnt--)
    delay(30);                               /* ! */
  if (cnt <= 0) {
    klog("ERROR: failed to get stable clock\n");
    return SD_ERROR;
  }
  return SD_OK;
}

#define GPFSEL4    0x0010
#define GPFSEL5    0x0014
#define GPPUDCLK0  0x0098
#define GPPUDCLK1  0x009C
#define GPHEN0     0x0064
#define GPHEN1     0x0068

static void emmc_gpio_init(device_t *dev) {
  emmc_state_t *state = (emmc_state_t *)dev->state;
  resource_t *gpio = state->gpio;
  int64_t r = 0;
  // GPIO_CD
  r = b_in(gpio, GPFSEL4);
  r &= ~(7 << (7 * 3));
  b_out(gpio, GPFSEL4, r);
  b_out(gpio, GPPUD, 2);
  delay(150);                               /* ! */
  b_out(gpio, GPPUDCLK1, 1 << 15);
  delay(150);                               /* ! */
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
  delay(150);                               /* ! */
  b_out(gpio, GPPUDCLK1, (1 << 16) | (1 << 17));
  delay(150);                               /* ! */
  b_out(gpio, GPPUD, 0);
  b_out(gpio, GPPUDCLK1, 0);

  // GPIO_DAT0, GPIO_DAT1, GPIO_DAT2, GPIO_DAT3
  r = b_in(gpio, GPFSEL5);
  r |= (7 << (0 * 3)) | (7 << (1 * 3)) | (7 << (2 * 3)) | (7 << (3 * 3));
  b_out(gpio, GPFSEL5, r);
  b_out(gpio, GPPUD, 2);
  delay(150);                               /* ! */
  b_out(gpio, GPPUDCLK1, (1 << 18) | (1 << 19) | (1 << 20) | (1 << 21));
  delay(150);                               /* ! */
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
    delay(30);                               /* ! */
  } while ((b_in(emmc, EMMC_CONTROL1) & C1_SRST_HC) && cnt--);
  if (cnt <= 0) {
    klog("ERROR: failed to reset EMMC\n");
    return SD_ERROR;
  }
  klog("EMMC: reset OK\n");
  b_set(emmc, EMMC_CONTROL1, C1_CLK_INTLEN | C1_TOUNIT_MAX);
  delay(30);                               /* ! */;
  // Set clock to setup frequency.
  if ((r = sd_clk(dev, 400000)))
    return r;
  b_out(emmc, EMMC_INT_EN,
        INT_CMD_DONE | INT_DATA_DONE | INT_READ_RDY | INT_WRITE_RDY |
        INT_CMD_TIMEOUT | INT_DATA_TIMEOUT);
  b_out(emmc, EMMC_INT_MASK,
        INT_CMD_DONE | INT_DATA_DONE | INT_READ_RDY | INT_WRITE_RDY |
        INT_CMD_TIMEOUT | INT_DATA_TIMEOUT);
  state->sd_scr[0] = state->sd_scr[1] = state->sd_rca = state->sd_err = 0;
  emmc_cmd(dev, CMD_GO_IDLE, 0, 0, 0);
  if (state->sd_err)
    return state->sd_err;

  emmc_cmd(dev, CMD_SEND_IF_COND, 0x000001AA, 0, 0);
  if (state->sd_err)
    return state->sd_err;
  cnt = 6;
  r = 0;
  while (!(r & ACMD41_CMD_COMPLETE) && cnt--) {
    delay(400);                               /* ! */
    r = emmc_cmd(dev, CMD_SEND_OP_COND, ACMD41_ARG_HC, 0, 0);
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

  emmc_cmd(dev, CMD_ALL_SEND_CID, 0, 0, 0);

  state->sd_rca = emmc_cmd(dev, CMD_SEND_REL_ADDR, 0, 0, 0);
  klog("EMMC: CMD_SEND_REL_ADDR returned %p \n", state->sd_rca);
  if (state->sd_err)
    return state->sd_err;

  if ((r = sd_clk(dev, 25000000)))
    return r;

  emmc_cmd(dev, CMD_CARD_SELECT, state->sd_rca, INT_DATA_DONE, 0);
  if (state->sd_err)
    return state->sd_err;
  if (emmc_status(dev, SR_DAT_INHIBIT))
    return SD_TIMEOUT;
  b_out(emmc, EMMC_BLKSIZECNT, (1 << 16) | 8);
  emmc_cmd(dev, CMD_SEND_SCR, 0, INT_READ_RDY, 0);
  if (state->sd_err)
    return state->sd_err;

  r = 0;
  cnt = 100000;
  WITH_SPIN_LOCK(&state->slock) {
    while (r < 2 && cnt) {
      if (b_in(emmc, EMMC_STATUS) & SR_READ_AVAILABLE)
        state->sd_scr[r++] = b_in(emmc, EMMC_DATA);
      else
        cnt--;
    }
    if (emmc_intr_wait(dev, INT_DATA_DONE, 0))
      return state->sd_err;
  }
  if (r != 2)
    return SD_TIMEOUT;
  if (state->sd_scr[0] & SCR_SD_BUS_WIDTH_4) {
    emmc_cmd(dev, CMD_SET_BUS_WIDTH, state->sd_rca | 2, 0, 0);
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
  klog("\r\n");
  state->sd_scr[0] &= ~SCR_SUPP_CCS;
  state->sd_scr[0] |= ccs;
  return SD_OK;
}

/*----------------------------------------------------------------------------*/

static int emmc_probe(device_t *dev) {
  /* return 0; */
  return dev->unit == 3;
}

static int test_read(device_t *dev) {
  unsigned char *testblock = kmalloc(M_DEV, 512, 0);
  int read_res = emmc_read_block(dev, 0, testblock, 512);
  if (read_res == 0)
    klog("eMMC test read failed!");
  kfree(M_DEV, testblock);
  return read_res;
}

static int emmc_attach(device_t *dev) {
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
  bus_intr_setup(dev, state->irq, emmc_intr_filter, NULL, state,
                 "e.MMMC interrupt");

  int init_res = emmc_init(dev);
  if (init_res != SD_OK) {
    klog("e.MMC initialzation failed with code %d.", init_res);
    return -1;
  }

  klog("e.MMC communication initialized successfully!");
  
  (void)test_read(dev);

  return 0;
}

#undef b_in
#undef b_out
#undef b_set
#undef b_clr

/*
static int emmc_detach(device_t *dev) {
}
*/

blockdev_methods_t emmc_block_if = {
  .read = NULL,
  .write = NULL,
};

driver_t emmc_driver = {
  .desc = "EMMC memory card driver",
  .size = sizeof(emmc_state_t),
  .probe = emmc_probe,
  .attach = emmc_attach,
  .pass = SECOND_PASS,
  .interfaces =
    {
      [DIF_BLOCK_MEM] = &emmc_block_if,
    },
};

DEVCLASS_ENTRY(root, emmc_driver);