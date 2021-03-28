#define KL_LOG KL_USB
#include <sys/devclass.h>
#include <sys/device.h>
#include <sys/libkern.h>
#include <sys/klog.h>
#include <sys/vnode.h>
#include <sys/devfs.h>
#include <dev/scsi.h>
#include <dev/usb.h>
#include <dev/umass.h>

typedef struct umass_state {
  uint32_t tag;
  uint32_t nblocks;
  uint32_t block_length;
  uint8_t nluns;
} umass_state_t;

static int umass_probe(device_t *dev) {
  usb_device_t *usbd = usb_device_of(dev);
  usb_device_descriptor_t *dd = &usbd->dd;

  /* We look for a mass storage device using SCSI command set
   * and bulk-only command specification for transfering data. */
  if (dd->bDeviceClass != UICLASS_MASS ||
      dd->bDeviceSubClass != UISUBCLASS_SCSI ||
      dd->bDeviceProtocol != UIPROTO_MASS_BBB)
    return 0;

  /* We need two bulk endpoints: IN and OUT. */
  if (usbd->nendps < 2)
    return 0;
  if (usb_endp_type(usbd, 0) != UE_BULK || !usb_endp_dir(usbd, 0))
    return 0;
  if (usb_endp_type(usbd, 1) != UE_BULK || usb_endp_dir(usbd, 1))
    return 0;

  return 1;
}

static inline uint8_t cbw_flags(int direction) {
  return (direction == TF_INPUT ? CBWFLAGS_IN : CBWFLAGS_OUT);
}

static int umass_reset(device_t *dev) {
  if (usb_bbb_reset(dev))
    return 1;
  if (usb_unhalt_endp(dev, 0))
    return 1;
  if (usb_unhalt_endp(dev, 1))
    return 1;

  return 0;
}

static int umass_send(device_t *dev, usb_buf_t *usbb) {
  usb_bulk_transfer(dev, usbb);
  WITH_SPIN_LOCK (&usbb->lock) { usb_wait(usbb); }
  if (usb_transfer_error(usbb))
    return 1;
  return 0;
}

static int umass_send_cbw(device_t *dev, usb_buf_t *usbb) {
  int error = 0;

  if (umass_send(dev, usbb) && usbb->flags & TF_STALLED) {
    if (umass_reset(dev))
      return 1;
    usb_reset_bulk_buf(usbb);
    error = umass_send(dev, usbb);
  }

  return error;
}

/* this is implied by the uhci physical buffer size (currently 1024) */
#define UMASS_MAX_SIZE 640

static int umass_transfer(device_t *dev, void *data, size_t size, int direction,
                          void *cmd, size_t cmd_len, uint8_t lun) {
  assert(size <= UMASS_MAX_SIZE);
  assert(cmd_len <= CBWCDBLENGTH);

  umass_state_t *umass = dev->state;
  uint32_t tag = umass->tag++;
  uint8_t flags = cbw_flags(direction);

  umass_bbb_cbw_t cbw = (umass_bbb_cbw_t){
    .dCBWSignature = CBWSIGNATURE,
    .dCBWTag = tag,
    .dCBWDataTransferLength = size,
    .bCBWFlags = flags,
    .bCBWLUN = lun,
    .bCDBLength = cmd_len,
  };
  memcpy(cbw.CBWCDB, cmd, cmd_len);

  usb_buf_t *usbb = usb_alloc_bulk_buf(&cbw, sizeof(umass_bbb_cbw_t), TF_BULK);
  int error = 0;

  /* Send Command Block Wrapper. */
  if ((error = umass_send_cbw(dev, usbb)))
    goto bad;

  /* Transfer the actual data. */
  usb_reuse_bulk_buf(usbb, data, size, direction | TF_BULK);
  error = umass_send(dev, usbb);

  /* Receive a Command Status Block. */
  umass_bbb_csw_t csb;
  usb_reuse_bulk_buf(usbb, &csb, sizeof(umass_bbb_csw_t), TF_INPUT | TF_BULK);
  if ((error |= umass_send(dev, usbb)))
    goto bad;

  error |= (csb.dCSWSignature != CSWSIGNATURE || csb.dCSWTag != tag ||
            csb.bCSWStatus & CSWSTATUS_FAILED);

bad:
  usb_free_buf(usbb);
  return error;
}

static void print_vendor(scsi_inquiry_data_t *sid) {
  char vendor[SID_VENDOR_SIZE + 1] = {};
  memcpy(vendor, sid->vendor, SID_VENDOR_SIZE);
  klog("vendor: %s", vendor);
}

static void print_product(scsi_inquiry_data_t *sid) {
  char product[SID_PRODUCT_SIZE + 1] = {};
  memcpy(product, sid->product, SID_PRODUCT_SIZE);
  klog("product: %s", product);
}

static void print_revision(scsi_inquiry_data_t *sid) {
  char revision[SID_REVISION_SIZE + 1] = {};
  memcpy(revision, sid->revision, SID_REVISION_SIZE);
  klog("revision: %s", revision);
}

static uintmax_t le2be(uintmax_t val, size_t size) {
  assert(size <= sizeof(uintmax_t));

  uint8_t *bytes = (uint8_t *)&val;
  for (size_t i = 0; i < size / 2; i++)
    swap(bytes[size - i - 1], bytes[i]);

  return val;
}

#define be2le(v, s) le2be((v), (s))

static int umass_inquiry(device_t *dev, uint8_t lun) {
  scsi_inquiry_data_t sid;
  scsi_inquiry_t siq = (scsi_inquiry_t){
    .opcode = INQUIRY,
    .length = le2be(SHORT_INQUIRY_LENGTH, 2),
  };
  int error = umass_transfer(dev, &sid, SHORT_INQUIRY_LENGTH, TF_INPUT, &siq,
                             sizeof(scsi_inquiry_t), lun);

  if (error)
    return 1;

  /* We only handle a direct access block device. */
  if (SID_TYPE(&sid) != 0x00)
    return 1;

  /* The device must be connected to logical unit. */
  if (SID_QUAL(&sid) != SID_QUAL_LU_CONNECTED)
    return 1;

  /* The SPC-4 states that response format should be 0x02
   * (0x01 is possible but obsolete). */
  uint8_t format = SID_FORMAT(&sid);
  if (format != 0x02 && format != 0x01)
    return 1;

  klog("USB mass storage device:");
  klog("command set: SCSI Block Commands version 3");

  print_vendor(&sid);
  print_product(&sid);
  print_revision(&sid);

  if (SID_IS_REMOVABLE(&sid))
    klog("the device is removable");
  else
    klog("the device is not removable");

  return 0;
}

static int umass_request_sense(device_t *dev, uint8_t lun) {
  scsi_sense_data_t ssd;
  scsi_request_sense_t srs = (scsi_request_sense_t){
    .opcode = REQUEST_SENSE,
    .length = sizeof(scsi_sense_data_t),
  };
  int error = umass_transfer(dev, &ssd, sizeof(scsi_sense_data_t), TF_INPUT,
                             &srs, sizeof(scsi_request_sense_t), lun);

  /* (MichalBlk) in the future, some recovery logic may be needed. */
  return error;
}

static int umass_read_capacity(device_t *dev, uint8_t lun) {
  scsi_read_capacity_data_t srcd;
  scsi_read_capacity_t src = (scsi_read_capacity_t){
    .opcode = READ_CAPACITY,
  };

  /* A device may require a sequence of request pairs of the form
   * read capacity + request sense. */
  for (int i = 0; i < 4; i++) {
    umass_transfer(dev, &srcd, sizeof(scsi_read_capacity_data_t), TF_INPUT,
                   &src, sizeof(scsi_read_capacity_t), lun);
    umass_request_sense(dev, 0);
  }

  if (umass_transfer(dev, &srcd, sizeof(scsi_read_capacity_data_t), TF_INPUT,
                     &src, sizeof(scsi_read_capacity_t), lun))
    return 1;

  /* (MichalBlk) FTTB, we assume number of logical block <= UINT32_MAX. */
  if (srcd.addr == UINT32_MAX)
    return 1;

  umass_state_t *umass = dev->state;
  umass->nblocks = be2le(srcd.addr, 4) + 1;
  umass->block_length = be2le(srcd.length, 4);

  klog("number of logical blocks: %u", umass->nblocks);
  klog("lobical block length: %u", umass->block_length);

  return 0;
}

/* (MichalBlk) this is a very simple read function that is provided
 * for testing purposes only. */
static int umass_read(vnode_t *v, uio_t *uio, __unused int ioflags) {
  device_t *dev = devfs_node_data(v);
  umass_state_t *umass = dev->state;

  uio->uio_offset = 0;

  void *buf = kmalloc(M_DEV, umass->block_length, M_WAITOK);

  /* (MichalBlk) we assume that block length <= UINT16_MAX. */
  scsi_rw_10_t srw = (scsi_rw_10_t){
    .opcode = READ_10,
    .addr = le2be(42, 4),
    .length = le2be(1, 2),
  };
  int error = umass_transfer(dev, buf, umass->block_length, TF_INPUT, &srw,
                             sizeof(scsi_rw_10_t), 0);

  if (error)
    goto bad;

  error = uiomove_frombuf(buf, umass->block_length, uio);

bad:
  kfree(M_DEV, buf);
  return error;
}

/* (MichalBlk) this is a very simple write function that is provided
 * for testing purposes only. */
static int umass_write(vnode_t *v, uio_t *uio, __unused int ioflags) {
  device_t *dev = devfs_node_data(v);
  umass_state_t *umass = dev->state;

  uio->uio_offset = 0;

  void *buf = kmalloc(M_DEV, umass->block_length, M_ZERO);
  int error = uiomove(buf, uio->uio_resid, uio);

  if (error)
    goto bad;

  /* (MichalBlk) we assume that block length <= UINT16_MAX. */
  scsi_rw_10_t srw = (scsi_rw_10_t){
    .opcode = WRITE_10,
    .addr = le2be(42, 4),
    .length = le2be(1, 2),
  };
  error = umass_transfer(dev, buf, umass->block_length, 0, &srw,
                         sizeof(scsi_rw_10_t), 0);

bad:
  kfree(M_DEV, buf);
  return error;
}

static vnodeops_t umass_ops = {
  .v_read = umass_read,
  .v_write = umass_write,
};

static int umass_attach(device_t *dev) {
  umass_state_t *umass = dev->state;

  uint8_t maxlun;
  if (usb_bbb_get_max_lun(dev, &maxlun))
    return 1;
  umass->nluns = (maxlun == 0xff ? 1 : maxlun + 1);

  if (umass_inquiry(dev, 0))
    return 1;

  /* After receiving a successful inquiry transfer,
   * the drive is ready for use. */

  /* Obtain the number and length of the logical blocks. */
  if (umass_read_capacity(dev, 0))
    return 1;

  /* Prepare /dev/umass interface. */
  devfs_makedev(NULL, "umass", &umass_ops, dev, NULL);

  return 0;
}

static driver_t umass_driver = {
  .desc = "USB mass storage device driver",
  .size = sizeof(umass_state_t),
  .pass = SECOND_PASS,
  .probe = umass_probe,
  .attach = umass_attach,
};

DEVCLASS_DECLARE(usbhc);
DEVCLASS_ENTRY(usbhc, umass_driver);
