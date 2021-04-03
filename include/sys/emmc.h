#ifndef _SYS_EMMC_H_
#define _SYS_EMMC_H_

#include <inttypes.h>
#include <sys/mimiker.h>
#include <sys/device.h>
#include <sys/endian.h>
#include <stddef.h>

/* These operation modes are described in JESD86-A441, with a brief summary
   at page 27 */
typedef enum emmc_op_mode {
  EMMCMODE_BOOT,
  EMMCMODE_CARD_ID,
  EMMCMODE_INTR,
  EMMCMODE_TRANSFER,
  EMMCMODE_INA,
} emmc_op_mode_t;

typedef enum emmc_result {
  EMMC_OK,
  EMMC_ERR_IERR,
  EMMC_ERR_TIMEOUT,
  EMMC_ERR_NOT_RESPONDING,
  EMMC_ERR_INTR,
  EMMC_ERR_BUSY,
} emmc_result_t;

/* Response types R1-R5 are described in JESD86-A441, page 94.
 * Response R1 and R1b are normally both represented as `EMMCRESP_R1`,
 * because they are structurally identical. */
typedef enum emmc_resp_type {
  EMMCRESP_NONE,
  EMMCRESP_R1,
  EMMCRESP_R1B, /* Can be used only as an argument for `emmc_send_app_cmd` */
  EMMCRESP_R2,
  EMMCRESP_R3,
  EMMCRESP_R4,
  EMMCRESP_R5,
} emmc_resp_type_t;

typedef struct emmc_response {
  uint32_t r[4]; /* Response bits 32-1/128-1 */
} emmc_response_t;

/* Raw 48-bit response */
#define EMMC_RESPV48(resp) ((resp)->r[0])

#define EMMC_FMASK48(r, o, n) ((EMMC_RESPV48((r)) >> (o)) & ((1 << (n)) - 1))

/* Accessors for fields in R1-R5 response.
 * End bit CRC7 and headers are ommited, because not every controller provides
 * access tio them */
#define EMMC_R1_CARD_STATUS(resp) EMMC_FMASK48(resp, 0, 32)
#if BYTE_ORDER == _BIG_ENDIAN
#define EMMC_R2_CIDCSD_H(resp) ((uint64_t *)((resp)->r))[0]
#define EMMC_R2_CIDCSD_L(resp) ((uint64_t *)((resp)->r))[1]
#endif
#if BYTE_ORDER == _LITTLE_ENDIAN
#define EMMC_R2_CIDCSD_H(resp) ((uint64_t *)((resp)->r))[1]
#define EMMC_R2_CIDCSD_L(resp) ((uint64_t *)((resp)->r))[0]
#endif
#define EMMC_R3_OCR(resp) EMMC_FMASK48((resp), 0, 32)
#define EMMC_R4_ARGUMENT(resp) EMMC_FMASK48((resp), 0, 32)
#define EMMC_R4_RRC(resp) EMMC_FMASK48((resp), 0, 8)
#define EMMC_R4_RA(resp) EMMC_FMASK48((resp), 8, 7)
#define EMMC_R4_STATUS(resp) EMMC_FMASK48((resp), 15, 1)
#define EMMC_R4_RCA(resp) EMMC_FMASK48((resp), 16, 16)
#define EMMC_R5_UDEF(resp) EMMC_FMASK48((resp), 0, 16)
#define EMMC_R5_WIN_RCA(resp) EMMC_FMASK48((resp), 16, 16)

/* For detailed command description, refer to JESD86-A441, page 87 */
typedef enum emmc_command {
  EMMC_CMD_GO_IDLE = 0,
  EMMC_CMD_SEND_OP_COND = 1,
  EMMC_CMD_ALL_SEND_CID = 2,
  EMMC_CMD_SET_RELATIVE_ADDR = 3,
  EMMC_CMD_SET_DSR = 4,
  EMMC_CMD_SLEEP_AWAKE = 5,
  EMMC_CMD_SWITCH = 6,
  EMMC_CMD_SELECT_CARD = 7,
  EMMC_CMD_SEND_EXT_CSD = 8,
  EMMC_CMD_SEND_CSD = 9,
  EMMC_CMD_SEND_CID = 10,
  EMMC_CMD_READ_DAT_UNTIL_STOP = 11,
  EMMC_CMD_STOP_TRANSMISSION = 12,
  EMMC_CMD_SEND_STATUS = 13,
  EMMC_CMD_BUSTEST_R = 14,
  EMMC_CMD_GO_INACTIVE_STATE = 15,
  EMMC_CMD_BUSTEST_W = 19,
  EMMC_CMD_SET_BLOCKLEN = 16,
  EMMC_CMD_READ_BLOCK = 17,
  EMMC_CMD_READ_MULTIPLE_BLOCKS = 18,
  EMMC_CMD_WRITE_DAT_UNTIL_STOP = 20,
  EMMC_CMD_SET_BLOCK_COUNT = 23,
  EMMC_CMD_WRITE_BLOCK = 24,
  EMMC_CMD_WRITE_MULTIPLE_BLOCKS = 25,
  EMMC_CMD_PROGRAM_CID = 26,
  EMMC_CMD_PROGRAM_CSD = 27,
  EMMC_CMD_SET_WRITE_PROT = 28,
  EMMC_CMD_CLR_WRITE_PROT = 29,
  EMMC_CMD_SEND_WRITE_PROT = 30,
  EMMC_CMD_SEND_WRITE_PROT_TYPE = 31,
  EMMC_CMD_ERASE_GROUP_START = 35,
  EMMC_CMD_ERASE_GROUP_END = 36,
  EMMC_CMD_ERASE = 38,
  EMMC_CMD_FAST_IO = 39,
  EMMC_CMD_GO_IRQ_STATE = 40,
  EMMC_CMD_LOCK_UNLOCK = 42,
  /* Illegal as `emmc_send_cmd` arguments. Use dedicated procedures instead. */
  EMMC_CMD_APP_CMD = 55,
  EMMC_CMD_GEN_CMD = 56,
} emmc_command_t;

typedef enum {
  EMMC_I_DATA_DONE = 0x01,
  EMMC_I_WRITE_READY = 0x02,
  EMMC_I_READ_READY = 0x04,
} emmc_wait_flags_t;

typedef enum emmc_app_flags {
  /* At most one of these */
  EMMC_APP_SUSPEND = 0x01,
  EMMC_APP_RESUME = 0x02,
  EMMC_APP_ABORT = 0x03,
  /* Any subset of these */
  EMMC_APP_DATA = 0x04,
  EMMC_APP_CHKIDX = 0x08,
  EMMC_APP_CHKCRC = 0x10,
  /* At most one of these */
  EMMC_APP_RESP136 = 0x20,
  EMMC_APP_RESP48 = 0x40,
  EMMC_APP_RESP48B = 0x60,
} emmc_app_flags_t;

/* Useful for drivers to build masks for app-flags */
#define EMMC_APP_CMDTYPE 0x03
#define EMMC_APP_RESPTYPE 0x60

typedef enum emmc_prop_id {
  EMMC_PROP_R_MODE,
  EMMC_PROP_RW_BLKSIZE,
  EMMC_PROP_RW_BLKCNT,
  EMMC_PROP_R_INT_FLAGS,
} emmc_prop_id_t;
typedef uint64_t emmc_prop_val_t;

/* For a detailed explanation on semantics refer to the comments above
 * respective wrappers */
typedef emmc_result_t (*emmc_send_cmd_t)(device_t *dev, emmc_command_t cmd,
                                         uint32_t arg1, uint32_t arg2,
                                         emmc_response_t *res);
typedef emmc_result_t (*emmc_send_app_cmd_t)(device_t *dev, uint32_t cmd,
                                             uint32_t arg1, uint32_t arg2,
                                             emmc_resp_type_t exp_rt,
                                             emmc_app_flags_t app_flags,
                                             emmc_response_t *res);
typedef int (*emmc_wait_t)(device_t *dev, emmc_wait_flags_t wflags);
typedef emmc_result_t (*emmc_read_dat_t)(device_t *dev, void *buf, size_t len,
                                         size_t *read);
typedef emmc_result_t (*emmc_write_dat_t)(device_t *dev, const void *buf,
                                          size_t len, size_t *wrote);
typedef int (*emmc_get_prop_t)(device_t *dev, emmc_prop_id_t id,
                               emmc_prop_val_t *val);
typedef int (*emmc_set_prop_t)(device_t *dev, emmc_prop_id_t id,
                               emmc_prop_val_t val);

typedef struct emmc_methods {
  emmc_send_cmd_t send_cmd;
  emmc_send_app_cmd_t send_app_cmd;
  emmc_wait_t wait;
  emmc_read_dat_t read;
  emmc_write_dat_t write;
  emmc_get_prop_t get_prop;
  emmc_set_prop_t set_prop;
} emmc_methods_t;

typedef struct emmc_device {
  uint64_t cid[2];
  uint64_t csd[2];
  uint32_t rca;
  uint32_t hostver;
  uint32_t appflags;
} emmc_device_t;

static inline emmc_methods_t *emmc_methods(device_t *dev) {
  return (emmc_methods_t *)dev->driver->interfaces[DIF_EMMC];
}

#define EMMC_METHOD_PROVIDER(dev, method)                                      \
  (device_method_provider((dev), DIF_EMMC,                                     \
                          offsetof(struct emmc_methods, method)))

/**
 * \brief Send an e.MMC command
 * \param dev e.MMC controller device
 * \param cmd e.MMC command (described in JESD86-A441)
 * \param arg1 first argument
 * \param arg2 second argument
 * \param res pointer for response data to be written to or NULL
 * \return EMMC_OK on success
 */
static inline emmc_result_t emmc_send_cmd(device_t *dev, emmc_command_t cmd,
                                          uint32_t arg1, uint32_t arg2,
                                          emmc_response_t *resp) {
  device_t *idev = EMMC_METHOD_PROVIDER(dev, send_cmd);
  return emmc_methods(idev->parent)->send_cmd(dev, cmd, arg1, arg2, resp);
}

/**
 * \brief Send an app-specific e.MMC command
 * \param dev e.MMC controller device
 * \param cmd e.MMC command (described in JESD86-A441)
 * \param arg1 first argument
 * \param arg2 second argument
 * \param exp_rt expected response type
 * \param app_flags a set of flags defining how the controller should handle
 *                  the command
 * \param res pointer for response data to be written to or NULL
 * \return EMMC_OK on success
 */
static inline emmc_result_t emmc_send_app_cmd(device_t *dev, uint32_t cmd,
                                              uint32_t arg1, uint32_t arg2,
                                              emmc_resp_type_t exp_rt,
                                              emmc_app_flags_t app_flags,
                                              emmc_response_t *resp) {
  device_t *idev = EMMC_METHOD_PROVIDER(dev, send_app_cmd);
  return emmc_methods(idev->parent)
    ->send_app_cmd(dev, cmd, arg1, arg2, exp_rt, app_flags, resp);
}

/**
 * \brief Wait until e.MMC signals
 * \param dev e.MMC controller device
 * \param wflags conditions to be met
 */
static inline int emmc_wait(device_t *dev, emmc_wait_flags_t wflags) {
  device_t *idev = EMMC_METHOD_PROVIDER(dev, wait);
  return emmc_methods(idev->parent)->wait(dev, wflags);
}

/**
 * \brief Read data fetched by the controller
 * \param dev e.MMC controller device
 * \param buf pointer to where the data should be written to
 * \param len expected data length
 * \param read pointer for the number of read bytes or NULL
 * \return EMMC_OK on success
 */
static inline emmc_result_t emmc_read(device_t *dev, void *buf, size_t len,
                                      size_t *read) {
  device_t *idev = EMMC_METHOD_PROVIDER(dev, read);
  return emmc_methods(idev->parent)->read(dev, buf, len, read);
}

/**
 * \brief Write data to the controller
 * \param dev e.MMC controller device
 * \param buf pointer to where the data should be read from
 * \param len data length
 * \param wrote pointer for the number of written bytes in or NULL
 * \return EMMC_OK on success
 */
static inline emmc_result_t emmc_write(device_t *dev, const void *buf,
                                       size_t len, size_t *wrote) {
  device_t *idev = EMMC_METHOD_PROVIDER(dev, write);
  return emmc_methods(idev->parent)->write(dev, buf, len, wrote);
}

/**
 * \brief Geta a value describing the e.MMC device
 * \param dev e.MMC controller device
 * \param id value identifier
 * \param var pointer to where the associated value should be written to
 * \return -1 if value was fetched successfully, 0 if it wasn't
 */
static inline int emmc_get_prop(device_t *dev, emmc_prop_id_t id,
                                emmc_prop_val_t *val) {
  device_t *idev = EMMC_METHOD_PROVIDER(dev, get_prop);
  return emmc_methods(idev->parent)->get_prop(dev, id, val);
}

/**
 * \brief Seta a value describing the e.MMC device
 * \param dev e.MMC controller device
 * \param id value identifier
 * \param var pointer to where the associated value should be written to
 * \return -1 if value was fetched successfully, 0 if it wasn't
 */
static inline int emmc_set_prop(device_t *dev, emmc_prop_id_t id,
                                emmc_prop_val_t val) {
  device_t *idev = EMMC_METHOD_PROVIDER(dev, set_prop);
  return emmc_methods(idev->parent)->set_prop(dev, id, val);
}

static inline emmc_device_t *emmc_device_of(device_t *device) {
  return (emmc_device_t *)((device->bus == DEV_BUS_EMMC) ? device->instance
                                                         : NULL);
}

extern emmc_resp_type_t emmc_resp_type[57];

#endif