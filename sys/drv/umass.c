/*
 * USB mass storage device driver.
 *
 * For explanation of terms used throughout the code
 * please see the following documents:
 *
 * - (1) Universal Serial BusMass Storage ClassBulk-Only Transport Revision 1.0,
 *   September 31, 1999:
 *     https://www.usb.org/sites/default/files/usbmassbulk_10.pdf
 *
 * - (2) SCSI Commands Reference Manual:
 *     https://manuals.plus/wp-content/sideloads/seagate-scsi-commands-reference-manual-optimized.pdf
 */
#define KL_LOG KL_DEV
#include <sys/devclass.h>
#include <sys/devfs.h>
#include <sys/device.h>
#include <sys/endian.h>
#include <sys/klog.h>
#include <sys/libkern.h>
#include <dev/scsi.h>
#include <dev/usb.h>
#include <dev/umass.h>
#include <sys/vnode.h>

/* We assume that block size >= 512. */
#define UMASS_MIN_BLOCK_SIZE 512

typedef struct umass_state {
  uint32_t next_tag;                    /* next CBS tag to grant */
  uint32_t nblocks;                     /* number of available blocks */
  uint32_t block_size;                  /* size of a single block */
  char vendor[SID_VENDOR_SIZE + 1];     /* vandor string */
  char product[SID_PRODUCT_SIZE + 1];   /* product string */
  char revision[SID_REVISION_SIZE + 1]; /* revision string */
} umass_state_t;

/*
 * Auxiliary functions.
 */

static void umass_print(device_t *dev) {
  umass_state_t *umass = dev->state;

  klog("USB mass storage device:");
  klog("command set: SCSI Block Commands version 3");

  if (*umass->vendor)
    klog("vendor: %s", umass->vendor);

  if (*umass->product)
    klog("product: %s", umass->product);

  if (*umass->revision)
    klog("revision: %s", umass->revision);

  klog("number of logical blocks: %u", umass->nblocks);
  klog("block size: %u", umass->block_size);
}

/*
 * Command/Data/Status auxiliary functions.
 */

/* Reset recovery is described in (1) 5.3.4. */
static int umass_reset_recovery(device_t *dev) {
  int error = 0;

  /* Bulk-Only reset request. */
  if ((error = usb_bbb_reset(dev)))
    return error;

  /* Unhalt the bulk input endpoint. */
  if ((error = usb_unhalt_endpt(dev, USB_TFR_BULK, USB_DIR_INPUT)))
    return error;

  /* Unhalt the bulk output endpoint. */
  if ((error = usb_unhalt_endpt(dev, USB_TFR_BULK, USB_DIR_OUTPUT)))
    return error;

  return 0;
}

/*
 * Command Block Wrapper handling functions.
 */

/* Translate USB direction flags to CBW flags. */
static uint8_t cbw_flags(usb_direction_t dir) {
  switch (dir) {
    case USB_DIR_INPUT:
      return CBWFLAGS_IN;
    case USB_DIR_OUTPUT:
      return CBWFLAGS_OUT;
    default:
      panic("Invalid USB direction!");
  }
}

static void cbw_setup(umass_bbb_cbw_t *cbw, uint32_t tag, uint32_t size,
                      uint8_t flags, void *cmd, uint8_t cmdsize) {
  assert(cmdsize <= CBWCDBLENGTH);

  cbw->dCBWSignature = CBWSIGNATURE;
  cbw->dCBWTag = tag;
  cbw->dCBWDataTransferLength = size;
  cbw->bCBWFlags = flags;
  cbw->bCBWLUN = 0;
  cbw->bCDBLength = cmdsize;

  memcpy(cbw->CBWCDB, cmd, cmdsize);
}

/* Command transport is described in (1) 5.3.1. */
static int cbw_send(device_t *dev, umass_bbb_cbw_t *cbw, usb_buf_t *buf) {
  int error = 0;

  usb_data_transfer(dev, buf, cbw, sizeof(umass_bbb_cbw_t), USB_TFR_BULK,
                    USB_DIR_OUTPUT);
  if (!(error = usb_buf_wait(buf)))
    return 0;

  /* If a STALL ocurrs during the first attempt, the device should be reset
   * and the transfer should be reissued. */
  if (buf->error != USB_ERR_STALLED)
    return error;

  if ((error = umass_reset_recovery(dev)))
    return error;

  usb_data_transfer(dev, buf, cbw, sizeof(umass_bbb_cbw_t), USB_TFR_BULK,
                    USB_DIR_OUTPUT);
  return usb_buf_wait(buf);
}

/*
 * Command Status Wrapper handling functions.
 */

static inline bool csw_valid_p(umass_bbb_csw_t *csw, uint32_t expected_tag) {
  return csw->dCSWSignature == CSWSIGNATURE && csw->dCSWTag == expected_tag;
}

/* Status transport is described in (1) 5.3.3. */
static int csw_receive(device_t *dev, uint32_t expected_tag, usb_buf_t *buf) {
  umass_bbb_csw_t csw;
  int error = 0;

  usb_data_transfer(dev, buf, &csw, sizeof(umass_bbb_csw_t), USB_TFR_BULK,
                    USB_DIR_INPUT);
  if ((error = usb_buf_wait(buf))) {
    /* Clear the STALL condition. */
    if ((error = usb_unhalt_endpt(dev, USB_TFR_BULK, USB_DIR_INPUT)))
      goto bad;

    usb_data_transfer(dev, buf, &csw, sizeof(umass_bbb_csw_t), USB_TFR_BULK,
                      USB_DIR_INPUT);
    if ((error = usb_buf_wait(buf)))
      goto bad;
  }

  if (!csw_valid_p(&csw, expected_tag)) {
    error = EIO;
    goto bad;
  }

  if (csw.bCSWStatus == CSWSTATUS_PHASE) {
    error = EIO;
    goto bad;
  }

  return (csw.bCSWStatus == CSWSTATUS_GOOD) ? 0 : EIO;

bad:
  umass_reset_recovery(dev);
  return error;
}

/*
 * Bulk-Only transfer functions.
 */

/* Command/Data/Status protocol transfers are described in (1) 5.  */
static int umass_transfer(device_t *dev, void *cmd, uint8_t cmdsize,
                          usb_direction_t dir, void *data, uint32_t size) {

  umass_state_t *umass = dev->state;
  usb_buf_t *buf = usb_buf_alloc();
  int csw_error = 0;
  int error = 0;

  /*
   * Command Block Wrapper phase.
   */

  umass_bbb_cbw_t cbw;
  uint32_t tag = umass->next_tag++;
  cbw_setup(&cbw, tag, size, cbw_flags(dir), cmd, cmdsize);
  if ((error = cbw_send(dev, &cbw, buf)))
    goto end;

  /*
   * Data transfer phase.
   */

  /* Transfer data using block size units. */
  for (uint32_t nbytes = 0; nbytes != size;) {
    uint32_t tfrsize = min(size, umass->block_size);

    usb_data_transfer(dev, buf, data, tfrsize, USB_TFR_BULK, dir);
    if ((error = usb_buf_wait(buf))) {
      /* If we encounter an error in the data phase,
       * we still need to receive a CSW. */
      break;
    }

    nbytes += tfrsize;
    data += tfrsize;
  }

  /*
   * Command Status Block phase.
   */

  csw_error = csw_receive(dev, tag, buf);

end:
  usb_buf_free(buf);
  return error ? error : csw_error;
}

/*
 * SCSI command functions.
 */

/* The INQUIRY command is described in (2) 3.8. */
static int umass_inquiry(device_t *dev) {
  scsi_inquiry_data_t inq_data;
  scsi_inquiry_t inq = (scsi_inquiry_t){
    .opcode = INQUIRY,
    .length = htobe16(SHORT_INQUIRY_LENGTH),
  };
  int error = 0;

  if ((error = umass_transfer(dev, &inq, sizeof(scsi_inquiry_t), USB_DIR_INPUT,
                              &inq_data, SHORT_INQUIRY_LENGTH)))
    return error;

  /* We only handle direct access block devices (i.e. command set SBC-3). */
  if (SID_TYPE(&inq_data) != T_DIRECT)
    return ENXIO;

  /* The device must be connected to the logical unit. */
  if (SID_QUAL(&inq_data) != SID_QUAL_LU_CONNECTED)
    return ENXIO;

  /* Copy supplied strings. */
  umass_state_t *umass = dev->state;
  memcpy(umass->vendor, inq_data.vendor, SID_VENDOR_SIZE);
  memcpy(umass->product, inq_data.product, SID_PRODUCT_SIZE);
  memcpy(umass->revision, inq_data.revision, SID_REVISION_SIZE);

  return 0;
}

/* The REQUEST SENSE command is described in (2) 3.40. */
static int umass_request_sense(device_t *dev) {
  scsi_sense_data_t sen_data;
  scsi_request_sense_t sen = (scsi_request_sense_t){
    .opcode = REQUEST_SENSE,
    .length = sizeof(scsi_sense_data_t),
  };
  /* XXX: in the future, some recovery logic may be needed.
   * FTTB, simply performing the command is sufficient. */
  return umass_transfer(dev, &sen, sizeof(scsi_request_sense_t), USB_DIR_INPUT,
                        &sen_data, sizeof(scsi_sense_data_t));
}

/*
 * For the read capacity request to succeed, most devices will require to
 * perform a sequence of the following pairs of requests:
 * (read capacity, request sense).
 * We have to establish some maximum number of possible iterations.
 */
#define READ_CAPACITY_MAX_ITR 4

/* The READ CAPACITY (10) command is described in (2) 3.24. */
static int umass_read_capacity(device_t *dev) {
  scsi_read_capacity_data_t cap_data;
  scsi_read_capacity_t cap = (scsi_read_capacity_t){
    .opcode = READ_CAPACITY,
  };
  int i, error = 0;

  for (i = 0; i < READ_CAPACITY_MAX_ITR; i++) {
    /* Try to issue the read capacity command. */
    if (!umass_transfer(dev, &cap, sizeof(scsi_read_capacity_t), USB_DIR_INPUT,
                        &cap_data, sizeof(scsi_read_capacity_data_t)))
      break;

    /* We've failed. Let's perform the request sense command. */
    if ((error = umass_request_sense(dev))) {
      /* This kind of error is unacceptable. Return with `error`. */
      return error;
    }
  }
  if (i == READ_CAPACITY_MAX_ITR) {
    /* We did our best. */
    return ENODEV;
  }

  /* We assume that number of logical blocks is < `UINT32_MAX`. */
  if (cap_data.addr == UINT32_MAX)
    return ENXIO;

  /* Save number of logical blocks and block size. */
  umass_state_t *umass = dev->state;
  umass->nblocks = be32toh(cap_data.addr) + 1;
  umass->block_size = be32toh(cap_data.length);

  return 0;
}

/*
 * Device node interface.
 */

static inline usb_direction_t uioop_to_usbdir(uio_op_t op) {
  switch (op) {
    case UIO_READ:
      return USB_DIR_INPUT;
    case UIO_WRITE:
      return USB_DIR_OUTPUT;
    default:
      panic("Invalid UIO operation!");
  }
}

static int umass_op(devnode_t *node, uio_t *uio) {
  device_t *dev = node->data;
  umass_state_t *umass = dev->state;
  usb_direction_t dir = uioop_to_usbdir(uio->uio_op);
  uint32_t block_size = umass->block_size;
  int error = 0;

  if (!is_aligned(uio->uio_offset, block_size))
    return EINVAL;
  if (!is_aligned(uio->uio_resid, block_size))
    return EINVAL;

  uint32_t start = uio->uio_offset / block_size;
  uint32_t nblocks = uio->uio_resid / block_size;
  uint32_t size = nblocks * block_size;

  /* NOTE: we assume that number of blocks to read is <= `UINT16_MAX`. */
  if (nblocks > UINT16_MAX)
    return EINVAL;

  void *buf = kmalloc(M_DEV, size, M_WAITOK);

  if (dir == USB_DIR_OUTPUT) {
    if ((error = uiomove(buf, uio->uio_resid, uio)))
      goto end;
  }

  scsi_rw_10_t rw10 = (scsi_rw_10_t){
    .opcode = (dir == USB_DIR_INPUT) ? READ_10 : WRITE_10,
    .addr = htobe32(start),
    .length = htobe16(nblocks),
  };
  error = umass_transfer(dev, &rw10, sizeof(scsi_rw_10_t), dir, buf, size);
  if (error)
    goto end;

  if (dir == USB_DIR_INPUT)
    error = uiomove_frombuf(buf, size, uio);

end:
  kfree(M_DEV, buf);
  return error;
}

static devops_t umass_devops = {
  .d_type = DT_SEEKABLE, /* TODO: should be `DT_DISK`. */
  .d_read = umass_op,
  .d_write = umass_op,
};

/*
 * Driver interface implementation.
 */

static int umass_probe(device_t *dev) {
  usb_device_t *udev = usb_device_of(dev);
  if (!udev)
    return 0;

  /* We're looking for a USB mass storage device using SCSI transparent
   * command set and Bulk-Only transfer protocol. */
  if (udev->class_code != UICLASS_MASS ||
      udev->subclass_code != UISUBCLASS_SCSI ||
      udev->protocol_code != UIPROTO_MASS_BBB)
    return 0;

  return 1;
}

static int umass_attach(device_t *dev) {
  int error = 0;

  /* Obtain the max Logical Unit Number of the drive. */
  uint8_t maxlun;
  if (usb_bbb_get_max_lun(dev, &maxlun))
    return ENXIO;

  /* We only support drives with a single logical unit. */
  if (maxlun)
    return ENXIO;

  /* Set the initial block size. */
  umass_state_t *umass = dev->state;
  umass->block_size = UMASS_MIN_BLOCK_SIZE;

  /* Identify the connected device. */
  if ((error = umass_inquiry(dev)))
    return ENXIO;

  /*
   * After receiving a successful inquiry transfer,
   * the drive is ready for use.
   */

  /* Obtain the number and length of the logical blocks. */
  if ((error = umass_read_capacity(dev)))
    return ENXIO;

  umass_print(dev);

  /* Prepare /dev/umass interface. */
  devfs_makedev_new(NULL, "umass", &umass_devops, dev, NULL);

  return 0;
}

static driver_t umass_driver = {
  .desc = "USB mass storage device driver",
  .size = sizeof(umass_state_t),
  .pass = SECOND_PASS,
  .probe = umass_probe,
  .attach = umass_attach,
};

DEVCLASS_ENTRY(usb, umass_driver);
