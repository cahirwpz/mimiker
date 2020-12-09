#include <sys/memdev.h>
#include <sys/mimiker.h>
#include <sys/resource.h>
#include <sys/klog.h>
#include <sys/types.h>
#include <aarch64/gpio.h>
#include <sys/device.h>
#include <bus.h>

#define GPPUD ((volatile uint32_t *)(MMIO_BASE + 0x00200094))

/* #define EMMC_ARG2 ((volatile uint32_t *)(MMIO_BASE + 0x00300000))
#define EMMC_BLKSIZECNT ((volatile uint32_t *)(MMIO_BASE + 0x00300004))
#define EMMC_ARG1 ((volatile uint32_t *)(MMIO_BASE + 0x00300008))
#define EMMC_CMDTM ((volatile uint32_t *)(MMIO_BASE + 0x0030000C))
#define EMMC_RESP0 ((volatile uint32_t *)(MMIO_BASE + 0x00300010))
#define EMMC_RESP1 ((volatile uint32_t *)(MMIO_BASE + 0x00300014))
#define EMMC_RESP2 ((volatile uint32_t *)(MMIO_BASE + 0x00300018))
#define EMMC_RESP3 ((volatile uint32_t *)(MMIO_BASE + 0x0030001C))
#define EMMC_DATA ((volatile uint32_t *)(MMIO_BASE + 0x00300020))
#define EMMC_STATUS ((volatile uint32_t *)(MMIO_BASE + 0x00300024))
#define EMMC_CONTROL0 ((volatile uint32_t *)(MMIO_BASE + 0x00300028))
#define EMMC_CONTROL1 ((volatile uint32_t *)(MMIO_BASE + 0x0030002C))
#define EMMC_INTERRUPT ((volatile uint32_t *)(MMIO_BASE + 0x00300030))
#define EMMC_INT_MASK ((volatile uint32_t *)(MMIO_BASE + 0x00300034))
#define EMMC_INT_EN ((volatile uint32_t *)(MMIO_BASE + 0x00300038))
#define EMMC_CONTROL2 ((volatile uint32_t *)(MMIO_BASE + 0x0030003C))
#define EMMC_SLOTISR_VER ((volatile uint32_t *)(MMIO_BASE + 0x003000FC)) */

#define EMMC_STATUS 24

// command flags
#define CMD_NEED_APP    0x80000000
#define CMD_RSPNS_48    0x00020000
#define CMD_ERRORS_MASK 0xfff9c004
#define CMD_RCA_MASK    0xffff0000

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
#define CMD_SET_BUS_WIDTH (0x06020000 | CMD_NEED_APP)
#define CMD_SEND_OP_COND  (0x29020000 | CMD_NEED_APP)
#define CMD_SEND_SCR      (0x33220010 | CMD_NEED_APP)

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

memdev_driver_t emmc_driver;

typedef struct emmc_state {
    resource_t *gpio_mem;
    resource_t *emmc_mem;
    uint64_t sd_scr[2];
    uint64_t sd_ocr;
    uint64_t sd_rca;
    uint64_t sd_err;
    uint64_t sd_hv;
    uint32_t last_lba;
    uint32_t last_code;
    int last_code_cnt;
} emmc_state_t;

//uint64_t sd_scr[2], sd_ocr, sd_rca, sd_err, sd_hv;

/*
 * \brief delay function
 * \param delay number of cycles to delay
 *
 * This just loops <delay> times in a way that the compiler
 * won't optimize away.
 */
static void delay(int64_t count) {
  __asm__ volatile("1: subs %[count], %[count], #1; bne 1b"
                   : [count] "+r"(count));
}

/**
 * Wait for data or command ready
 */
int sd_status(uint32_t mask) {
  int32_t cnt = 500000;
  while (*EMMC_STATUS & mask) && !(*EMMC_INTERRUPT & INT_ERROR_MASK) && cnt--)
    delay(150); /* Random value lol */
  return (cnt <= 0 || (*EMMC_INTERRUPT & INT_ERROR_MASK)) ? SD_ERROR : SD_OK;
}

/**
 * Wait for interrupt
 */
static int32_t sd_int(uint32_t mask) {
  uint32_t r, m = mask | INT_ERROR_MASK;
  int32_t cnt = 1000000;
  while (!(*EMMC_INTERRUPT & m) && cnt--)
    delay(150);                                       /* ! */
  r = *EMMC_INTERRUPT;
  if (cnt <= 0 || (r & INT_CMD_TIMEOUT) || (r & INT_DATA_TIMEOUT)) {
    *EMMC_INTERRUPT = r;
    return SD_TIMEOUT;
  } else if (r & INT_ERROR_MASK) {
    *EMMC_INTERRUPT = r;
    return SD_ERROR;
  }
  *EMMC_INTERRUPT = mask;
  return 0;
}

/**
 * Send a command
 */
int32_t emmc_cmd(emmc_state_t *state, uint32_t code, uint32_t arg) {
  if (state->last_code == code)
    state->last_code_cnt++;
  else
    state->last_code_cnt = 0;

  state->last_code = code;

  uint32_t r = 0;
  state->sd_err = SD_OK;
  if (code & CMD_NEED_APP) {
    r = emmc_cmd(state, CMD_APP_CMD | (state->sd_rca ? CMD_RSPNS_48 : 0),
                 state->sd_rca);
    if (state->sd_rca && !r) {
      klog("ERROR: failed to send SD APP command\n");
      state->sd_err = SD_ERROR;
      return 0;
    }
    code &= ~CMD_NEED_APP;
  }
  if (sd_status(SR_CMD_INHIBIT)) {
    klog("ERROR: EMMC busy\n");
    state->sd_err = SD_TIMEOUT;
    return 0;
  }
  // klog("EMMC: Sending command %p, arg %p\n", code, arg);
  *EMMC_INTERRUPT = *EMMC_INTERRUPT;
  *EMMC_ARG1 = arg;
  *EMMC_CMDTM = code;
  if (code == CMD_SEND_OP_COND)
    delay(1500);                                    /* ! */
  else if (code == CMD_SEND_IF_COND || code == CMD_APP_CMD)
    delay(150);                                    /* ! */
  
  if ((r = sd_int(INT_CMD_DONE))) {
    klog("ERROR: failed to send EMMC command\n");
    state->sd_err = r;
    return 0;
  }

  r = *EMMC_RESP0;
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
    r |= *EMMC_RESP3;
    r |= *EMMC_RESP2;
    r |= *EMMC_RESP1;

    if (state->last_code_cnt == 0)
      klog("EMMC: Sending command %s, arg %p\n", "SEND CID", arg);
    return r;
  }

  else if (code == CMD_SEND_REL_ADDR) {
    state->sd_err = (((r & 0x1fff)) | ((r & 0x2000) << 6) | ((r & 0x4000) << 8) |
              ((r & 0x8000) << 8)) &
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
  int32_t r;
  uint32_t c = 0, d;
  if (num < 1)
    num = 1;
  if (lba != state->last_lba)
    klog("sd_readblock lba %p, num %p\n", lba, num);
  state->last_lba = lba;

  if (sd_status(SR_DAT_INHIBIT)) {
    state->sd_err = SD_TIMEOUT;
    return 0;
  }
  uint32_t *buf = (uint32_t *)buffer;
  if (state->sd_scr[0] & SCR_SUPP_CCS) {
    if (num > 1 && (state->sd_scr[0] & SCR_SUPP_SET_BLKCNT)) {
      emmc_cmd(state, CMD_SET_BLOCKCNT, num);
      if (state->sd_err)
        return 0;
    }
    *EMMC_BLKSIZECNT = (num << 16) | 512;
    emmc_cmd(state, num == 1 ? CMD_READ_SINGLE : CMD_READ_MULTI, lba);
    if (state->sd_err)
      return 0;
  } else {
    *EMMC_BLKSIZECNT = (1 << 16) | 512;
  }
  while (c < num) {
    if (!(state->sd_scr[0] & SCR_SUPP_CCS)) {
      emmc_cmd(state, CMD_READ_SINGLE, (lba + c) * 512);
      if (state->sd_err)
        return 0;
    }
    if ((r = sd_int(INT_READ_RDY))) {
      klog("\rERROR: Timeout waiting for ready to read\n");
      state->sd_err = r;
      return 0;
    }
    for (d = 0; d < 128; d++)
      buf[d] = *EMMC_DATA;
    c++;
    buf += 128;
  }
  if (num > 1 && !(state->sd_scr[0] & SCR_SUPP_SET_BLKCNT) &&
      (state->sd_scr[0] & SCR_SUPP_CCS))
    emmc_cmd(state, CMD_STOP_TRANS, 0);
  return state->sd_err != SD_OK || c != num ? 0 : num * 512;
}

/**
 * Write a block to the sd card
 * returns 0 on error
 */
int sd_writeblock(device_t *dev, uint32_t lba, const uint8_t *buffer,
                  uint32_t num) {
  assert(dev->driver == (driver_t *)&emmc_driver);
  emmc_state_t *state = (emmc_state_t *)dev->state;
  if (((uint64_t)buffer & (uint64_t)0x3) != 0)
    return 0;
  uint32_t *buf = (uint32_t *)buffer;
  uint32_t c = 0;
  int32_t d, r;
  if (num < 1)
    return 0;
  if (sd_status(SR_DAT_INHIBIT)) {
    state->sd_err = SD_TIMEOUT;
    return 0;
  }
  if (state->sd_scr[0] & SCR_SUPP_CCS) {
    if (num > 1 && (state->sd_scr[0] & SCR_SUPP_SET_BLKCNT)) {
      emmc_cmd(state, CMD_SET_BLOCKCNT, num);
      if (state->sd_err)
        return 0;
    }
    *EMMC_BLKSIZECNT = (num << 16) | 512;
    emmc_cmd(state, num == 1 ? CMD_WRITE_SINGLE : CMD_WRITE_MULTI, lba);
    if (state->sd_err)
      return 0;
  } else {
    *EMMC_BLKSIZECNT = (1 << 16) | 512;
  }
  while (c < num) {
    if (!(state->sd_scr[0] & SCR_SUPP_CCS)) {
      emmc_cmd(state, CMD_WRITE_SINGLE, (lba + c) * 512);
      if ((r = sd_int(INT_WRITE_RDY))) {
        // uart_puts("\rERROR: Timeout waiting for ready to write\n");
        state->sd_err = r;
        return 0;
      }
      for (d = 0; d < 128; d++)
        *EMMC_DATA = buf[d];
      if (state->sd_err)
        return 0;
    }
    c++;
    buf += 128;
  }

  if (!sd_int(INT_DATA_DONE))
    return 1;

  return 0;
}

/**
 * set SD clock to frequency in Hz
 */
int32_t sd_clk(emmc_state_t *state, uint32_t f) {
  uint32_t d, c = 41666666 / f, x, s = 32, h = 0;
  int32_t cnt = 100000;
  while ((*EMMC_STATUS & (SR_CMD_INHIBIT | SR_DAT_INHIBIT)) && cnt--)
    delay(150);                               /* ! */
  if (cnt <= 0) {
    klog("ERROR: timeout waiting for inhibit flag\n");
    return SD_ERROR;
  }

  *EMMC_CONTROL1 &= ~C1_CLK_EN;
  delay(150);                               /* ! */
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
  *EMMC_CONTROL1 = (*EMMC_CONTROL1 & 0xffff003f) | d;
  delay(150);                                 /* ! */
  *EMMC_CONTROL1 |= C1_CLK_EN;
  delay(150);                                 /* ! */
  cnt = 10000;
  while (!(*EMMC_CONTROL1 & C1_CLK_STABLE) && cnt--)
    delay(150);                               /* ! */
  if (cnt <= 0) {
    klog("ERROR: failed to get stable clock\n");
    return SD_ERROR;
  }
  return SD_OK;
}

#define GPFSEL4 ((volatile uint32_t *)(MMIO_BASE + 0x00200010))
#define GPFSEL5 ((volatile uint32_t *)(MMIO_BASE + 0x00200014))
#define GPPUDCLK0 ((volatile uint32_t *)(MMIO_BASE + 0x00200098))
#define GPPUDCLK1 ((volatile uint32_t *)(MMIO_BASE + 0x0020009C))
#define GPHEN0 ((volatile uint32_t *)(MMIO_BASE + 0x00200064))
#define GPHEN1 ((volatile uint32_t *)(MMIO_BASE + 0x00200068))

/**
 * initialize EMMC to read SDHC card
 */
static int emmc_init(device_t *dev) {
  assert(dev->driver == (driver_t *)&emmc_driver);
  emmc_state_t *state = (emmc_state_t *)dev->state;
  int64_t r, cnt, ccs = 0;
  // GPIO_CD
  r = *GPFSEL4;
  r &= ~(7 << (7 * 3));
  *GPFSEL4 = r;
  *GPPUD = 2;
  delay(150);                               /* ! */
  *GPPUDCLK1 = (1 << 15);
  delay(150);                               /* ! */
  *GPPUD = 0;
  *GPPUDCLK1 = 0;
  r = *GPHEN1;
  r |= 1 << 15;
  *GPHEN1 = r;

  // GPIO_CLK, GPIO_CMD
  r = *GPFSEL4;
  r |= (7 << (8 * 3)) | (7 << (9 * 3));
  *GPFSEL4 = r;
  *GPPUD = 2;
  delay(150);                               /* ! */
  *GPPUDCLK1 = (1 << 16) | (1 << 17);
  delay(150);                               /* ! */
  *GPPUD = 0;
  *GPPUDCLK1 = 0;

  // GPIO_DAT0, GPIO_DAT1, GPIO_DAT2, GPIO_DAT3
  r = *GPFSEL5;
  r |= (7 << (0 * 3)) | (7 << (1 * 3)) | (7 << (2 * 3)) | (7 << (3 * 3));
  *GPFSEL5 = r;
  *GPPUD = 2;
  delay(150);                               /* ! */
  *GPPUDCLK1 = (1 << 18) | (1 << 19) | (1 << 20) | (1 << 21);
  delay(150);                               /* ! */
  *GPPUD = 0;
  *GPPUDCLK1 = 0;

  state->sd_hv = (*EMMC_SLOTISR_VER & HOST_SPEC_NUM) >> HOST_SPEC_NUM_SHIFT;
  klog("EMMC: GPIO set up\n");
  // Reset the card.
  *EMMC_CONTROL0 = 0;
  *EMMC_CONTROL1 |= C1_SRST_HC;
  cnt = 10000;
  do {
    delay(150);                               /* ! */
  } while ((*EMMC_CONTROL1 & C1_SRST_HC) && cnt--);
  if (cnt <= 0) {
    klog("ERROR: failed to reset EMMC\n");
    return SD_ERROR;
  }
  klog("EMMC: reset OK\n");
  *EMMC_CONTROL1 |= C1_CLK_INTLEN | C1_TOUNIT_MAX;
  delay(150);                               /* ! */;
  // Set clock to setup frequency.
  if ((r = sd_clk(state, 400000)))
    return r;
  *EMMC_INT_EN = 0xffffffff;
  *EMMC_INT_MASK = 0xffffffff;
  state->sd_scr[0] = state->sd_scr[1] = state->sd_rca = state->sd_err = 0;
  emmc_cmd(state, CMD_GO_IDLE, 0);
  if (state->sd_err)
    return state->sd_err;

  emmc_cmd(state, CMD_SEND_IF_COND, 0x000001AA);
  if (state->sd_err)
    return state->sd_err;
  cnt = 6;
  r = 0;
  while (!(r & ACMD41_CMD_COMPLETE) && cnt--) {
    delay(400);                               /* ! */
    r = emmc_cmd(state, CMD_SEND_OP_COND, ACMD41_ARG_HC);
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

  emmc_cmd(state, CMD_ALL_SEND_CID, 0);

  state->sd_rca = emmc_cmd(state, CMD_SEND_REL_ADDR, 0);
  klog("EMMC: CMD_SEND_REL_ADDR returned %p \n", state->sd_rca);
  if (state->sd_err)
    return state->sd_err;

  if ((r = sd_clk(state, 25000000)))
    return r;

  emmc_cmd(state, CMD_CARD_SELECT, state->sd_rca);
  if (state->sd_err)
    return state->sd_err;

  if (sd_status(SR_DAT_INHIBIT))
    return SD_TIMEOUT;
  *EMMC_BLKSIZECNT = (1 << 16) | 8;
  emmc_cmd(state, CMD_SEND_SCR, 0);
  if (state->sd_err)
    return state->sd_err;
  if (sd_int(INT_READ_RDY))
    return SD_TIMEOUT;

  r = 0;
  cnt = 100000;
  while (r < 2 && cnt) {
    if (*EMMC_STATUS & SR_READ_AVAILABLE)
      state->sd_scr[r++] = *EMMC_DATA;
    else
      delay(150);                               /* ! */
  }
  if (r != 2)
    return SD_TIMEOUT;
  if (state->sd_scr[0] & SCR_SD_BUS_WIDTH_4) {
    emmc_cmd(state, CMD_SET_BUS_WIDTH, state->sd_rca | 2);
    if (state->sd_err)
      return state->sd_err;
    *EMMC_CONTROL0 |= C0_HCTL_DWITDH;
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
  /* unimplemented! */
  panic("EMMC probe is not implemented");
  return 0;
}

static int emmc_attach(device_t *dev) {
  assert(dev->driver == (driver_t *)&emmc_driver);
  emmc_state_t *state = (emmc_state_t *)dev->state;
  state->last_lba = 0;
  state->last_code = 0;
  state->last_code_cnt = 0;
  state->gpio_mem = device_take_memory(dev, 0, RF_ACTIVE);
  state->emmc_mem = device_take_memory(dev, 1, RF_ACTIVE);
  emmc_init(dev);
  return 0;
}

/*
static int emmc_detach(device_t *dev) {
}
*/

memdev_driver_t emmc_driver = {
  .driver = {
    .desc = "EMMC memory card driver",
    .size = sizeof(emmc_state_t),
    .probe = emmc_probe,
    .attach = emmc_attach,
    /* .detach = emmc_detach */
  },
  .memdev = {
    .read_block = emmc_read_block,
    .write_block = sd_writeblock
  }
};