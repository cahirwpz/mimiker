#define KL_LOG KL_DEV

/* This driver is based on SD Specifications Part 1: Physical Layer Simplified
 * Specification, Version 6.00. This document is a free resource, available
 * here (link split into two lines due to code formatting requirements):
 * https://www.taterli.com/wp-content/uploads/2017/05/
 *   Physical-Layer-Simplified-SpecificationV6.0.pdf */

#include <sys/mimiker.h>
#include <dev/emmc.h>
#include <sys/device.h>
#include <sys/devclass.h>
#include <sys/rman.h>
#include <sys/klog.h>

static driver_t sd_block_device_driver;

/* The custom R7 response is handled just like R1 response, but has different
 * bitfields, same goes for R6 */
#define SDRESP_R7 EMMCRESP_R1
#define SD_R7_CHKPAT(resp) EMMC_FMASK48(resp, 0, 8)

#define SDRESP_R6 EMMCRESP_R1
#define SD_R6_RCA(resp) EMMC_FMASK48(resp, 16, 16)

/* Custom commands */
#define SD_CMD_SET_IF_COND                                                     \
  (emmc_cmd_t) {                                                               \
    .cmd_idx = 8, .flags = 0, .exp_resp = SDRESP_R7,                           \
  }

#define SD_CMD_SEND_OP_COND                                                    \
  (emmc_cmd_t) {                                                               \
    .cmd_idx = 41, .flags = EMMC_F_APP, .exp_resp = EMMCRESP_R1,               \
  }

#define SD_CMD_SEND_SCR                                                        \
  (emmc_cmd_t) {                                                               \
    .cmd_idx = 51, .flags = EMMC_F_APP | EMMC_F_DATA_READ,                     \
    .exp_resp = EMMCRESP_R1,                                                   \
  }

#define SD_CMD_SEND_REL_ADDR                                                   \
  (emmc_cmd_t) {                                                               \
    .cmd_idx = 3, .flags = 0, .exp_resp = SDRESP_R6,                           \
  }

#define SD_CMD_SET_BUS_WIDTH                                                   \
  (emmc_cmd_t) {                                                               \
    .cmd_idx = 6, .flags = EMMC_F_APP, .exp_resp = EMMCRESP_R1,                \
  }

#define SD_ACMD41_SD2_0_POLLRDY_ARG1 0x51ff8000
#define SD_ACMD41_RESP_BUSY_OFFSET 31
#define SD_ACMD41_RESP_BUSY_WIDTH 1
#define SD_ACMD41_RESP_CCS_OFFSET 30
#define SD_ACMD41_RESP_CCS_WIDTH 1
#define SD_ACMD41_RESP_UHSII_OFFSET 29
#define SD_ACMD41_RESP_UHSII_WIDTH 1
#define SD_ACMD41_RESP_SW18A_OFFSET 24
#define SD_ACMD41_RESP_SW18A_WIDTH 1

#define SD_ACMD41_RESP_SET_BUSY(r, b)                                          \
  EMMC_FMASK48_WR((r), SD_ACMD41_RESP_BUSY_OFFSET, SD_ACMD41_RESP_BUSY_WIDTH,  \
                  (b))
#define SD_ACMD41_RESP_READ_BUSY(r)                                            \
  EMMC_FMASK48((r), SD_ACMD41_RESP_BUSY_OFFSET, SD_ACMD41_RESP_BUSY_WIDTH)
#define SD_ACMD41_RESP_READ_CCS(r)                                             \
  EMMC_FMASK48((r), SD_ACMD41_RESP_CCS_OFFSET, SD_ACMD41_RESP_CCS_WIDTH)

// SCR flags
#define SCR_SD_BUS_WIDTH_4 0x00000400
#define SCR_SUPP_SET_BLKCNT 0x02000000

#define SD_BUSWIDTH_1 0x00
#define SD_BUSWIDTH_4 0x02

typedef enum sd_props {
  SD_SUPP_CCS,
  SD_SUPP_BLKCNT,
  SD_SUPP_BUSWIDTH_4,
} sd_props_t;

typedef struct emmcblk_state {
  sd_props_t props;
} emmcblk_state_t;

static int sd_probe(device_t *dev) {
  return dev->unit == 0;
}

static char testbuf[2880];

static const char standard_cap_str[] = "Ver 2.0 or later, Standard Capacity "
                                       "SD Memory Card";
static const char high_cap_str[] = "Ver 2.0 or later, High Capacity or Extended"
                                   " Capacity SD Memory Card";

static int emmc_blk_test_rw(device_t *dev);

static int sd_attach(device_t *dev) {
  emmcblk_state_t *state = (emmcblk_state_t *)dev->state;

  emmc_resp_t response;
  uint64_t propv;
  uint16_t trial_cnt;
  size_t of = 0;
  uint32_t scr[2];
  uint16_t rca;

  state->props = 0;

  /* The routine below is based on SD Specifications: Part 1 Physical Layer
   * Simplified Specification, Version 6.0. See: page 30 */

  klog("Attachind SD/SDHC block device interface");

  emmc_send_cmd(dev, EMMC_CMD(GO_IDLE), 0, 0, NULL);
  if (emmc_get_prop(dev, EMMC_PROP_R_VOLTAGE_SUPPLY, &propv)) {
    klog("Unable to determine eMMC controller's voltage supply.");
    return -1;
  }
  uint8_t chkpat = ~propv + 1;
  if (emmc_set_prop(dev, EMMC_PROP_RW_RESP_LOW, chkpat - 1)) {
    klog("Unable to determine whether CMD8 responds.");
    return -1;
  }
  emmc_send_cmd(dev, SD_CMD_SET_IF_COND, propv << 8 | chkpat, 0, &response);
  if (SD_R7_CHKPAT(&response) != chkpat) {
    klog("SD 2.0 voltage supply is mismatched, or the card is at Version 1.x");
    return -1;
  }
  trial_cnt = 120;
  /* Counter-intuitively, the busy bit is set ot 0 if the card is not ready */
  SD_ACMD41_RESP_SET_BUSY(&response, 0);
  while (trial_cnt & ~SD_ACMD41_RESP_READ_BUSY(&response)) {
    if (trial_cnt-- == 0) {
      klog("Card timedout on ACMD41 polling.");
      return -1;
    }
    emmc_send_cmd(dev, SD_CMD_SEND_OP_COND, SD_ACMD41_SD2_0_POLLRDY_ARG1, 0,
                  &response);
  }
  if (SD_ACMD41_RESP_READ_CCS(&response))
    state->props |= SD_SUPP_CCS;
  klog("eMMC device detected as %s",
       (state->props & SD_SUPP_CCS) ? high_cap_str : standard_cap_str);

  /* Let's assume there's no voltage switching needed */

  emmc_send_cmd(dev, EMMC_CMD(ALL_SEND_CID), 0, 0, NULL);
  emmc_send_cmd(dev, SD_CMD_SEND_REL_ADDR, 0, 0, &response);
  rca = SD_R6_RCA(&response);
  emmc_set_prop(dev, EMMC_PROP_RW_RCA, rca);

  /* At this point we should have just enetered data transfer mode */

  if (emmc_set_prop(dev, EMMC_PROP_RW_CLOCK_FREQ, 25000000))
    return -1;

  emmc_send_cmd(dev, EMMC_CMD(SELECT_CARD), rca << 16, 0, NULL);
  emmc_set_prop(dev, EMMC_PROP_RW_BLKSIZE, 8);
  emmc_set_prop(dev, EMMC_PROP_RW_BLKCNT, 1);
  emmc_send_cmd(dev, SD_CMD_SEND_SCR, 0, 0, NULL);
  if (emmc_wait(dev, EMMC_I_READ_READY)) {
    klog("SD card timed out when waiting for data (SD_CMD_SEND_SCR)");
    return -1;
  }
  emmc_read(dev, scr, 64, &of);
  if (emmc_wait(dev, EMMC_I_DATA_DONE)) {
    klog("SD card timed out when waiting for end of transmission");
    return -1;
  }
  if (of != 64) {
    klog("Failed to read SD Card's SCR");
    return -1;
  }

  if (scr[0] & SCR_SUPP_SET_BLKCNT)
    state->props |= SD_SUPP_BLKCNT;
  if (scr[0] & SCR_SD_BUS_WIDTH_4) {
    state->props |= SD_SUPP_BUSWIDTH_4;
    emmc_send_cmd(dev, SD_CMD_SET_BUS_WIDTH, SD_BUSWIDTH_4, 0, NULL);
    emmc_set_prop(dev, EMMC_PROP_RW_BUSWIDTH, EMMC_BUSWIDTH_4);
  }

  klog("Card's feature support:\n* CCS: %s\n* SET_BLOCK_COUNT: %s",
       (state->props & SD_SUPP_CCS) ? "YES" : "NO",
       (state->props & SD_SUPP_BLKCNT) ? "YES" : "NO");

  if (emmc_blk_test_rw(dev))
    return -1;

  return 0;
}

static char testbuf[] =
  "Man is a rope stretched between the animal and the Superman--a rope over an "
  "abyss.\n\nA dangerous crossing, a dangerous wayfaring, a dangerous "
  "looking-back, a dangerous trembling and halting.\n\nWhat is great in man is "
  "that he is a bridge and not a goal: what is lovable in man is that he is an "
  "OVER-GOING and a DOWN-GOING.\n\nI love those that know not how to live "
  "except as down-goers, for they are the over-goers.\n\nI love the great "
  "despisers, because they are the great adorers, and arrows of longing for "
  "the other shore.\n\nI love those who do not first seek a reason beyond the "
  "stars for going down and being sacrifices, but sacrifice themselves to the "
  "earth, that the earth of the Superman may hereafter arrive.\n\nI love him "
  "who lives in order to know, and seeks to know in order that the Superman "
  "may hereafter live. Thus seeks he his own down-going.\n\nI love him who "
  "labors and invents, that he may build the house for the Superman, and "
  "prepare for him earth, animal, and plant: for thus seeks he his own "
  "down-going.\n\nI love him who loves his virtue: for virtue is the will to "
  "down-going, and an arrow of longing.\n\nI love him who reserves no share of "
  "spirit for himself, but wants to be wholly the spirit of his virtue: thus "
  "walks he as spirit over the bridge.\n\nI love him who makes his virtue his "
  "inclination and destiny: thus, for the sake of his virtue, he is willing to "
  "live on, or live no more.\n\nI love him who desires not too many virtues. "
  "One virtue is more of a virtue than two, because it is more of a knot for "
  "one's destiny to cling to.\n\nI love him whose soul is lavish, who wants no "
  "thanks and does not give back: for he always bestows, and desires not to "
  "keep for himself.\n\nI love him who is ashamed when the dice fall in his "
  "favor, and who then asks: \"Am I a dishonest player?\"--for he is willing "
  "to succumb.\n\nI love him who scatters golden words in advance of his "
  "deeds, and always does more than he promises: for he seeks his own "
  "down-going.\n\nI love him who justifies the future ones, and redeems the "
  "past ones: for he is willing to succumb through the present ones.\n\nI love "
  "him who chastens his God, because he loves his God: for he must succumb "
  "through the wrath of his God.\n\nI love him whose soul is deep even in the "
  "wounding, and may succumb through a small matter: thus goes he willingly "
  "over the bridge.\n\nI love him whose soul is so overfull that he forgets "
  "himself, and all things that are in him: thus all things become his "
  "down-going.\n\nI love him who is of a free spirit and a free heart: thus is "
  "his head only the bowels of his heart; his heart, however, causes his "
  "down-going.\n\nI love all who are like heavy drops falling one by one out "
  "of the dark cloud that lowers over man: they herald the coming of the "
  "lightning, and succumb as heralds.\n\nLo, I am a herald of the lightning, "
  "and a heavy drop out of the cloud: the lightning, however, is the "
  "SUPERMAN.\n";

/**
 * read a block from sd card and return the number of bytes read
 * returns 0 on error.
 */
int emmcblk_read_blk(device_t *dev, uint32_t lba, void *buffer, uint32_t num) {
  assert(dev->driver == (driver_t *)&sd_block_device_driver);
  emmcblk_state_t *state = (emmcblk_state_t *)dev->state;

  if (num < 1)
    return 0;

  uint32_t *buf = (uint32_t *)buffer;
  int r;

  uint64_t sup_blkcnt = 0;

  if (state->props & SD_SUPP_CCS) {
    /* Multiple block transfers either be terminated after a set amount of
     * block is transferred, or by sending CMD_STOP_TRANS command */
    if (num > 1 && (state->props & SD_SUPP_BLKCNT)) {
      r = emmc_send_cmd(dev, EMMC_CMD(SET_BLOCK_COUNT), num, 0, NULL);
      if (r)
        return 0;
    }
    emmc_set_prop(dev, EMMC_PROP_RW_BLKCNT, num);
    emmc_set_prop(dev, EMMC_PROP_RW_BLKSIZE, 512);
    if (num > 1) {
      emmc_send_cmd(dev, EMMC_CMD(READ_MULTIPLE_BLOCKS), lba, 0, NULL);
    } else {
      emmc_send_cmd(dev, EMMC_CMD(READ_BLOCK), lba, 0, NULL);
    }
    if (emmc_wait(dev, EMMC_I_READ_READY))
      return 0;
  } else {
    emmc_set_prop(dev, EMMC_PROP_RW_BLKCNT, num);
    emmc_set_prop(dev, EMMC_PROP_RW_BLKSIZE, 512);
  }
  if (state->props & SD_SUPP_CCS) {
    r = emmc_read(dev, buf, num * 512, NULL);
    if (r)
      return 0;
    if (emmc_wait(dev, EMMC_I_DATA_DONE))
      return 0;
  } else {
    for (uint32_t c = 0; c < num; c++) {
      emmc_send_cmd(dev, EMMC_CMD(READ_BLOCK), lba + c, 0, NULL);
      r = emmc_read(dev, buf, 512, NULL);
      if (r)
        return 0;
      if (emmc_wait(dev, EMMC_I_DATA_DONE))
        return 0;
      buf += 128;
    }
  }
  if (num > 1 && !sup_blkcnt && (state->props & SD_SUPP_CCS))
    emmc_send_cmd(dev, EMMC_CMD(STOP_TRANSMISSION), 0, 0, NULL);
  return num * 512;
}

/**
 * Write a block to the sd card
 * returns 0 on error
 */
int emmcblk_write_blk(device_t *dev, uint32_t lba, const void *buffer,
                      uint32_t num) {
  assert(dev->driver == (driver_t *)&sd_block_device_driver);
  emmcblk_state_t *state = (emmcblk_state_t *)dev->state;

  /* DMA requirements */
  /* if (((uint64_t)buffer & (uint64_t)0x3) != 0)
    return 0; */

  uint32_t *buf = (uint32_t *)buffer;
  int r;
  uint32_t c = 0;

  if (num < 1)
    return 0;

  if (state->props & SD_SUPP_BLKCNT) {
    if (num > 1 && (state->props & SD_SUPP_BLKCNT)) {
      r = emmc_send_cmd(dev, EMMC_CMD(SET_BLOCK_COUNT), num, 0, NULL);
      if (r)
        return 0;
    }
    emmc_set_prop(dev, EMMC_PROP_RW_BLKCNT, num);
    emmc_set_prop(dev, EMMC_PROP_RW_BLKSIZE, 512);

    emmc_send_cmd(
      dev, num == 1 ? EMMC_CMD(WRITE_BLOCK) : EMMC_CMD(WRITE_MULTIPLE_BLOCKS),
      lba, 0, NULL);
    if (emmc_wait(dev, EMMC_I_WRITE_READY))
      return 0;
  } else {
    emmc_set_prop(dev, EMMC_PROP_RW_BLKCNT, num);
    emmc_set_prop(dev, EMMC_PROP_RW_BLKSIZE, 512);
  }
  while (c < num) {
    if (!(state->props & SD_SUPP_BLKCNT)) {
      emmc_send_cmd(dev, EMMC_CMD(WRITE_BLOCK), lba + c, 0, NULL);
      if (emmc_wait(dev, EMMC_I_WRITE_READY))
        return 0;
    }
    if (emmc_write(dev, buf, 512, NULL)) {
      klog("eMMC write failed");
      return 0;
    }
    if (emmc_wait(dev, EMMC_I_DATA_DONE))
      return 0;

    c++;
    buf += 128;
  }

  return num * 512;
}

static int emmc_blk_test_rw(device_t *dev) {
  int r = 0;
  char *buf = kmalloc(M_DEV, 4 * 512, M_ZERO);
  r = emmcblk_write_blk(dev, 0, testbuf, 4);
  if (!r)
    goto sd_test_failed;
  r = emmcblk_read_blk(dev, 0, buf, 4);
  if (!r)
    goto sd_test_failed;
  for (size_t i = 0; i < 4 * 512; i++) {
    if (buf[i] != testbuf[i]) {
      klog("SD test bytes don't match at 0x%x", i);
      goto sd_test_failed;
    }
  }
  klog("SD test passed");
  kfree(M_DEV, buf);
  return 0;
sd_test_failed:
  klog("SD test failed");
  kfree(M_DEV, buf);
  return -1;
}

static driver_t sd_block_device_driver = {.desc = "e.MMC block device driver",
                                          .size = sizeof(emmcblk_state_t),
                                          .probe = sd_probe,
                                          .attach = sd_attach,
                                          .pass = SECOND_PASS,
                                          .interfaces = {}};

DEVCLASS_ENTRY(emmc, sd_block_device_driver);