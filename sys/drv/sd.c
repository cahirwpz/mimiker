#define KL_LOG KL_DEV

/* This driver is based on "SD Specifications Part 1: Physical Layer Simplified
 * Specification, Version 6.00". This document is a free resource, available
 * here (link split into two lines due to code formatting requirements):
 * https://www.taterli.com/wp-content/uploads/2017/05/
 *   Physical-Layer-Simplified-SpecificationV6.0.pdf
 * Whenever a comment in this very file mentions "the specs", it refers to this
 * document, unless explicitly said otherwise.
 */

#include <sys/mimiker.h>
#include <dev/emmc.h>
#include <sys/device.h>
#include <sys/devclass.h>
#include <sys/klog.h>
#include <sys/vnode.h>
#include <sys/devfs.h>
#include <dev/sd.h>
#include <sys/fdt.h>

typedef struct sd_state {
  sd_props_t props; /* SD Card's flags */
  void *block_buf;  /* A buffer for reading data into */
  uint64_t csd[2];  /* Card-Specific Data register's content */
  uint16_t rca;     /* Relative Card Address */
} sd_state_t;

static int sd_probe(device_t *dev) {
  /* return FDT_is_compatible(dev->node, "mimiker,sd"); */
  return dev->unit == 0;
}

static const char standard_cap_str[] = "Ver 2.0 or later, Standard Capacity "
                                       "SD Memory Card";
static const char high_cap_str[] = "Ver 2.0 or later, High Capacity or Extended"
                                   " Capacity SD Memory Card";

static int sd_sanity_check(device_t *dev) {
  uint64_t propv;
  emmc_get_prop(dev, EMMC_PROP_RW_ERRORS, &propv);
  if (propv & EMMC_ERROR_INTERNAL) {
    klog("A failure in communication with an SD device occured: %llx", propv);
    return ENXIO;
  }
  return 0;
}

/* Returns SD card's capacity in bytes */
static uint64_t sd_capacity(sd_state_t *state) {
  /* XXX: The specs seems to imply that C_SIZE entry should be read from a
   * freshly generated response of CMD9 (SEND_CSD). I don't know when the value
   * of C_SIZE could change, but we don't have such scenario yet. */
  /* XXX Note that the response doesn't contain the CRC checksum and the end
   * bit, which are presented as art of CSD register structure in the specs.
   * Thus all fields are shifted in the response by 8 bits right */
  if (state->props & SD_SUPP_CCS) {
    /* SDHC/SDSC cards have different CSD structure than SD cards.
     * Refer to page 171 and 172 of the SD specs */
    uint64_t c_size = (state->csd[0] & 0x3fffff0000000000) >> 40;
    assert(c_size >= 0x1010); /* Minimum size for SDHC */
    assert(c_size <= 0xff5f); /* Maximum size for SDHC */
    return (c_size + 1) * 512 * 1024;
  }
  /* Refer to page 164 and 167 of the SD specs */
  uint64_t c_size_low = (state->csd[0] & 0xffc0000000000000) >> 62;
  uint64_t c_size_high = state->csd[1] & 0x03;
  uint64_t c_size_mult = (state->csd[0] & 0x0000038000000000) >> 47;
  uint64_t read_bl_len = (state->csd[1]) & 0x00000f00 >> 16;
  uint64_t c_size = (c_size_high << 2) | c_size_low;
  return (c_size + 1) * c_size_mult * (1 << read_bl_len);
}

static int sd_init(device_t *dev) {
  int err = 0;
  sd_state_t *state = (sd_state_t *)dev->state;

  emmc_resp_t response;
  uint64_t propv;                /* Property's Value */
  size_t of = 0;                 /* Offset (Used only for checking the number of
                                  * bytes read */
  uint32_t scr[SD_SCR_WORD_CNT]; /* SD Configuration Register */

  state->props = 0;

  emmc_reset(dev);

  /* The card initialization routine below is based on the diagram present on
   * the page 30 of the specs. */

  klog("Attaching SD/SDHC block device interface...");

  emmc_set_prop(dev, EMMC_PROP_RW_RCA, 0);
  emmc_send_cmd(dev, EMMC_CMD(GO_IDLE), 0, NULL);

  if ((err = sd_sanity_check(dev)))
    return err;

  if (emmc_get_prop(dev, EMMC_PROP_R_VOLTAGE_SUPPLY, &propv)) {
    klog("Unable to determine e.MMC controller's voltage supply.");
    return ENXIO;
  }
  uint8_t chkpat = ~propv + 1;
  emmc_set_prop(dev, EMMC_PROP_RW_RESP_LOW, chkpat - 1);
  emmc_send_cmd(dev, SD_CMD_SET_IF_COND, propv << 8 | chkpat, &response);

  assert(sd_sanity_check(dev) == 0);
  if ((err = sd_sanity_check(dev)))
    return err;

  if (SD_R7_CHKPAT(&response) != chkpat) {
    klog("SD 2.0 voltage supply is mismatched, or the card is at Version 1.x");
    return ENXIO;
  }

  /* Counter-intuitively, the busy bit is set ot 0 if the card is not ready */
  SD_ACMD41_RESP_SET_BUSY(&response, 0);
  int trial_cnt = 120; /* TODO: test it on a real hardware */

  /* During this phase the controller may time out on some commands and
   * it shouldn't be treated as a fatal error. It just means that the card
   * is busy. Refer to page 29 of the specs  for more information. */
  emmc_set_prop(dev, EMMC_PROP_RW_ALLOW_ERRORS, EMMC_ERROR_TIMEOUT);

  if ((err = sd_sanity_check(dev)))
    return err;

  while (trial_cnt & ~SD_ACMD41_RESP_READ_BUSY(&response)) {
    if (trial_cnt-- < 0) {
      klog("Card timedout on ACMD41 polling.");
      emmc_set_prop(dev, EMMC_PROP_RW_ALLOW_ERRORS, 0);
      return ETIMEDOUT;
    }
    emmc_send_cmd(dev, SD_CMD_SEND_OP_COND, SD_ACMD41_SD2_0_POLLRDY_ARG1,
                  &response);
    emmc_get_prop(dev, EMMC_PROP_RW_ERRORS, &propv);
    if (propv == EMMC_ERROR_INTERNAL) {
      emmc_set_prop(dev, EMMC_PROP_RW_ALLOW_ERRORS, 0);
      klog("SD/HC: An internal error has occured.");
      return ENXIO;
    }
  }

  if ((err = sd_sanity_check(dev)))
    return err;

  emmc_set_prop(dev, EMMC_PROP_RW_ERRORS, 0);
  emmc_set_prop(dev, EMMC_PROP_RW_ALLOW_ERRORS, 0);

  if (SD_ACMD41_RESP_READ_CCS(&response))
    state->props |= SD_SUPP_CCS;

  klog("e.MMC device detected as %s",
       (state->props & SD_SUPP_CCS) ? high_cap_str : standard_cap_str);

  /* Let's assume there's no voltage switching needed */

  emmc_send_cmd(dev, EMMC_CMD(ALL_SEND_CID), 0, NULL);
  emmc_send_cmd(dev, SD_CMD_SEND_REL_ADDR, 0, &response);
  state->rca = SD_R6_RCA(&response);
  emmc_set_prop(dev, EMMC_PROP_RW_RCA, state->rca);

  if ((err = sd_sanity_check(dev)))
    return err;

  if (emmc_set_prop(dev, EMMC_PROP_RW_CLOCK_FREQ, SD_CLOCK_FREQ))
    klog("Failed to set e.MMC clock for SD card. Transfers might be slow.");

  emmc_send_cmd(dev, EMMC_CMD(SEND_CSD), state->rca << 16, &response);

  state->csd[0] = (uint64_t)response.r[0] | ((uint64_t)response.r[1] << 32);
  state->csd[1] = (uint64_t)response.r[2] | ((uint64_t)response.r[3] << 32);

  /* Go from stand-by state into transfer state */
  emmc_send_cmd(dev, EMMC_CMD(SELECT_CARD), state->rca << 16, NULL);

  emmc_set_prop(dev, EMMC_PROP_RW_BLKSIZE, 8);
  emmc_set_prop(dev, EMMC_PROP_RW_BLKCNT, 1);

  emmc_send_cmd(dev, SD_CMD_SEND_SCR, 0, NULL);
  emmc_wait(dev, EMMC_I_READ_READY);
  emmc_read(dev, scr, SD_SCR_WORD_CNT * sizeof(uint32_t), &of);
  emmc_wait(dev, EMMC_I_DATA_DONE);

  if (of != SD_SCR_WORD_CNT * sizeof(uint32_t)) {
    klog("Failed to read SD Card's SCR");
    return EIO;
  }

  if (scr[0] & SCR_SUPP_SET_BLKCNT)
    state->props |= SD_SUPP_BLKCNT;
  if (scr[0] & SCR_SD_BUS_WIDTH_4) {
    state->props |= SD_SUPP_BUSWIDTH_4;
    emmc_send_cmd(dev, SD_CMD_SET_BUS_WIDTH, SD_BUSWIDTH_4, NULL);
    emmc_set_prop(dev, EMMC_PROP_RW_BUSWIDTH, EMMC_BUSWIDTH_4);
  }

  if ((err = sd_sanity_check(dev)))
    return err;

  klog("Card's feature support:\n* CCS: %s\n* SET_BLOCK_COUNT: %s\n"
       "Capacity: %lluB",
       (state->props & SD_SUPP_CCS) ? "YES" : "NO",
       (state->props & SD_SUPP_BLKCNT) ? "YES" : "NO",
       sd_capacity(state));
  
  if ((err = sd_sanity_check(dev)))
    return err;
  
  return err;
}

/* Data read routine for CCS-enabled cards (SDHC/SDSC) */
static int sd_read_block_ccs(device_t *dev, uint32_t lba, void *buffer,
                             uint32_t num, size_t *read) {
  int err = 0;
  sd_state_t *state = (sd_state_t *)dev->state;
  uint32_t *buf = (uint32_t *)buffer;

  emmc_set_prop(dev, EMMC_PROP_RW_BLKCNT, num);
  emmc_set_prop(dev, EMMC_PROP_RW_BLKSIZE, DEFAULT_BLKSIZE);

  /* Multiple block transfers either be terminated after a set amount of
   * block is transferred, or by sending CMD_STOP_TRANS command */
  if (num > 1 && (state->props & SD_SUPP_BLKCNT))
    emmc_send_cmd(dev, EMMC_CMD(SET_BLOCK_COUNT), num, NULL);

  emmc_cmd_t read_blocks_cmd =
    (num > 1) ? EMMC_CMD(READ_MULTIPLE_BLOCKS) : EMMC_CMD(READ_BLOCK);
  emmc_send_cmd(dev, read_blocks_cmd, lba, NULL);
  emmc_wait(dev, EMMC_I_READ_READY);

  if ((err = sd_sanity_check(dev)))
    return err;

  if ((err = emmc_read(dev, buf, num * DEFAULT_BLKSIZE, NULL)))
    return err;
  if ((num == 1) || (state->props & SD_SUPP_BLKCNT)) {
    emmc_wait(dev, EMMC_I_DATA_DONE);
  }

  if (read)
    *read = num * DEFAULT_BLKSIZE;

  if ((num > 1) && !(state->props & SD_SUPP_BLKCNT)) {
    emmc_send_cmd(dev, EMMC_CMD(STOP_TRANSMISSION), 0, NULL);
    emmc_wait(dev, EMMC_I_DATA_DONE);
  }

  return sd_sanity_check(dev);
}

/* Data read routine for CCS-disabled cards (SDSC) */
static int sd_read_block_noccs(device_t *dev, uint32_t lba, void *buffer,
                               uint32_t num, size_t *read) {
  int err = 0;
  uint32_t *buf = (uint32_t *)buffer;

  for (uint32_t c = 0; c < num; c++) {
    /* See note no. 10 at page 222 of the specs. */
    emmc_send_cmd(dev, EMMC_CMD(READ_BLOCK), (lba + c) * DEFAULT_BLKSIZE, NULL);

    if ((err = sd_sanity_check(dev)))
      return err;

    if ((err = emmc_read(dev, buf, DEFAULT_BLKSIZE, NULL)))
      return err;
    emmc_wait(dev, EMMC_I_DATA_DONE);
    buf += DEFAULT_BLKSIZE / sizeof(uint32_t);
    if (read)
      *read = *read + DEFAULT_BLKSIZE;
  }

  return sd_sanity_check(dev);
}

/* Read blocks from sd card. Returns 0 on success. */
static int sd_read_blk(device_t *dev, uint32_t lba, void *buffer, uint32_t num,
                       size_t *read) {
  sd_state_t *state = (sd_state_t *)dev->state;

  if (num < 1)
    return EINVAL;

  int err = 0;

  if (read)
    *read = 0;

  emmc_set_prop(dev, EMMC_PROP_RW_BLKCNT, num);
  emmc_set_prop(dev, EMMC_PROP_RW_BLKSIZE, DEFAULT_BLKSIZE);

  if (state->props & SD_SUPP_CCS) {
    if ((err = sd_read_block_ccs(dev, lba, buffer, num, read)))
      return err;
  } else {
    if ((err = sd_read_block_noccs(dev, lba, buffer, num, read)))
      return err;
  }

  return err;
}

static int sd_write_blk(device_t *dev, uint32_t lba, void *buffer, uint32_t num,
                        size_t *wrote) {
  sd_state_t *state = (sd_state_t *)dev->state;

  uint32_t *buf = (uint32_t *)buffer;
  int err = 0;

  if (num < 1)
    return EINVAL;

  if (wrote)
    *wrote = 0;

  emmc_set_prop(dev, EMMC_PROP_RW_BLKCNT, num);
  emmc_set_prop(dev, EMMC_PROP_RW_BLKSIZE, DEFAULT_BLKSIZE);

  if (num > 1 && (state->props & SD_SUPP_BLKCNT))
    emmc_send_cmd(dev, EMMC_CMD(SET_BLOCK_COUNT), num, NULL);
  
  emmc_cmd_t write_blocks_cmd =
    (num > 1) ? EMMC_CMD(WRITE_MULTIPLE_BLOCKS) : EMMC_CMD(WRITE_BLOCK);

  uint32_t addr = lba;
  /* See note no. 10 at page 222 of
   * SD Physical Layer Simplified Specification V6.0 */
  if (!(state->props & SD_SUPP_CCS))
    addr *= DEFAULT_BLKSIZE;
  emmc_send_cmd(dev, write_blocks_cmd, addr, NULL);
  emmc_wait(dev, EMMC_I_WRITE_READY);

  for (uint32_t c = 0; c < num; c++) {
    if ((err = sd_sanity_check(dev)))
      return err;

    if ((err = emmc_write(dev, buf, DEFAULT_BLKSIZE, NULL)))
      return err;
    buf += DEFAULT_BLKSIZE / sizeof(uint32_t);
    if (wrote)
      *wrote = *wrote + DEFAULT_BLKSIZE;
  }

  emmc_wait(dev, EMMC_I_DATA_DONE);
  if (num > 1) {
    emmc_send_cmd(dev, EMMC_CMD(STOP_TRANSMISSION), 0, NULL);
  }

  err = sd_sanity_check(dev);
  return err;
}


static inline int sd_check_uio(device_t *dev, uio_t *uio) {
  sd_state_t *state = (sd_state_t *)dev->state;
  if ((uint64_t)uio->uio_offset >= sd_capacity(state))
    return EINVAL;
  return 0;
}

static int sd_dop_uio(devnode_t *d, uio_t *uio) {
  device_t *dev = d->data;
  sd_state_t *state = (sd_state_t *)dev->state;
  int err;

  if ((err = sd_check_uio(dev, uio)))
    return err;

  uint32_t lba = uio->uio_offset / DEFAULT_BLKSIZE;
  uint32_t read_offset = lba * DEFAULT_BLKSIZE;
  uint32_t copy_offset =  uio->uio_offset - read_offset;
  uint32_t blk_cnt = (uio->uio_resid) / DEFAULT_BLKSIZE;
  /* XXX: If there's a offset that causes the transfer to overlap with next
   * block, we need to read that extra block as well. */
  if (blk_cnt * DEFAULT_BLKSIZE < copy_offset + uio->uio_resid)
    blk_cnt++;

  uint32_t blocks_left = blk_cnt;
  while (uio->uio_resid) {
    /* We move from internal buffer, soe we need to change the offset to reflect
    * that from our internal buffer */
    uio->uio_offset = copy_offset;
    uint32_t num = min(blocks_left, (uint32_t)SD_KERNEL_BLOCKS);
    uint32_t bytes_to_transfer =
      min(uio->uio_resid, num * DEFAULT_BLKSIZE - copy_offset);

    if (uio->uio_op == UIO_READ) {
      if ((err = sd_read_blk(dev, lba, state->block_buf, num, NULL)))
        return err;
      if ((err = uiomove_frombuf(state->block_buf,
          SD_KERNEL_BLOCKS * DEFAULT_BLKSIZE, uio)))
        return err;
    } else {
      if (bytes_to_transfer % DEFAULT_BLKSIZE != 0) {
        if ((err = sd_read_blk(dev, lba, state->block_buf, num, NULL)))
          return err;
      }
      if ((err = uiomove_frombuf(state->block_buf,
          SD_KERNEL_BLOCKS * DEFAULT_BLKSIZE, uio)))
        return err;
      if ((err = sd_write_blk(dev, lba, state->block_buf, num, NULL)))
        return err;
    }
    copy_offset = 0;
    blocks_left -= num;
    lba += num;
  }

  return 0;
}

static int sd_open(devnode_t *d, file_t *fp, int oflags) {
  device_t *dev = d->data;
  sd_state_t *state = (sd_state_t *)dev->state;
  
  d->size = sd_capacity(state);

  return 0;
}

static devops_t sd_devops = {
  .d_type = DT_SEEKABLE,
  .d_open = sd_open,
  .d_read = sd_dop_uio,
  .d_write = sd_dop_uio,
};

static int sd_attach(device_t *dev) {
  int err = 0;
  sd_state_t *state = (sd_state_t *)dev->state;

  state->block_buf = kmalloc(M_DEV, SD_KERNEL_BLOCKS * DEFAULT_BLKSIZE, 0);

  if ((err = sd_init(dev)))
    goto sd_attach_fail;

  if ((err = devfs_makedev_new(NULL, "sd_card", &sd_devops, dev, NULL)))
    goto sd_attach_fail;

  return err;

sd_attach_fail:
  kfree(M_DEV, state->block_buf);

  return err;
}

static driver_t sd_block_device_driver = {
  .desc = "SD(SC/HC) block device driver",
  .size = sizeof(sd_state_t),
  .pass = SECOND_PASS,
  .probe = sd_probe,
  .attach = sd_attach,
};

DEVCLASS_ENTRY(emmc, sd_block_device_driver);