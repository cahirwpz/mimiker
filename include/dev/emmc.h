#ifndef _DEV_EMMC_H_
#define _DEV_EMMC_H_

/* This interface is based on a JESD86-A441 specification, which can be
 * obtained for free from JEDEC standards and documents archive at:
 * https://www.jedec.org/document_search?search_api_views_fulltext=jesd84-a441
 */

#include <inttypes.h>
#include <sys/mimiker.h>
#include <sys/device.h>
#include <sys/endian.h>
#include <stddef.h>

/* Response types R1-R5 are described in JESD86-A441, page 94. */
typedef enum emmc_resp_type {
  EMMCRESP_NONE,
  EMMCRESP_R1,
  EMMCRESP_R1B,
  EMMCRESP_R2,
  EMMCRESP_R3,
  EMMCRESP_R4,
  EMMCRESP_R5,
} emmc_resp_type_t;

typedef struct emmc_resp {
  uint32_t r[4]; /* Response bits 32-1/128-1 */
} emmc_resp_t;

/* Raw 48-bit response */
#define EMMC_RESPV48(resp) ((resp)->r[0])

/* Read a bitfield */
#define EMMC_FMASK48(r, o, n) ((EMMC_RESPV48((r)) >> (o)) & ((1 << (n)) - 1))
/* Write to a bitfield */
#define EMMC_FMASK48_WR(r, o, n, v)                                            \
  EMMC_RESPV48((r)) = (EMMC_RESPV48((r)) & ~(((1 << (n)) - 1) << (o))) |       \
                      (((v) & ((1 << (n)) - 1)) << (o))

/* Accessors for fields in R1-R5 response.
 * End bit CRC7 and headers are ommited, because not every controller provides
 * access to them */
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
#define EMMC_CMD_GO_IDLE 0
#define EMMC_CMD_SEND_OP_COND 1
#define EMMC_CMD_ALL_SEND_CID 2
#define EMMC_CMD_SET_RELATIVE_ADDR 3
#define EMMC_CMD_SET_DSR 4
#define EMMC_CMD_SLEEP_AWAKE 5
#define EMMC_CMD_SWITCH 6
#define EMMC_CMD_SELECT_CARD 7
#define EMMC_CMD_SEND_EXT_CSD 8
#define EMMC_CMD_SEND_CSD 9
#define EMMC_CMD_SEND_CID 10
#define EMMC_CMD_READ_DAT_UNTIL_STOP 11
#define EMMC_CMD_STOP_TRANSMISSION 12
#define EMMC_CMD_SEND_STATUS 13
#define EMMC_CMD_BUSTEST_R 14
#define EMMC_CMD_GO_INACTIVE_STATE 15
#define EMMC_CMD_BUSTEST_W 19
#define EMMC_CMD_SET_BLOCKLEN 16
#define EMMC_CMD_READ_BLOCK 17
#define EMMC_CMD_READ_MULTIPLE_BLOCKS 18
#define EMMC_CMD_WRITE_DAT_UNTIL_STOP 20
#define EMMC_CMD_SET_BLOCK_COUNT 23
#define EMMC_CMD_WRITE_BLOCK 24
#define EMMC_CMD_WRITE_MULTIPLE_BLOCKS 25
#define EMMC_CMD_PROGRAM_CID 26
#define EMMC_CMD_PROGRAM_CSD 27
#define EMMC_CMD_SET_WRITE_PROT 28
#define EMMC_CMD_CLR_WRITE_PROT 29
#define EMMC_CMD_SEND_WRITE_PROT 30
#define EMMC_CMD_SEND_WRITE_PROT_TYPE 31
#define EMMC_CMD_ERASE_GROUP_START 35
#define EMMC_CMD_ERASE_GROUP_END 36
#define EMMC_CMD_ERASE 38
#define EMMC_CMD_FAST_IO 39
#define EMMC_CMD_GO_IRQ_STATE 40
#define EMMC_CMD_LOCK_UNLOCK 42
/* Illegal as `cmd_idx` of `emmc_cmd_t`. Use command flags instead. */
#define EMMC_CMD_APP_CMD 55
#define EMMC_CMD_GEN_CMD 56

typedef uint32_t emmc_cmd_idx_t;

typedef enum {
  EMMC_I_DATA_DONE = 0x01,
  EMMC_I_WRITE_READY = 0x02,
  EMMC_I_READ_READY = 0x04,
} emmc_wait_flags_t;

typedef enum emmc_cmd_flags {
  /* At most one of these */
  EMMC_F_SUSPEND = 0x01, /* Suspend current data transfer */
  EMMC_F_RESUME = 0x02,  /* Resume last data transfer */
  EMMC_F_ABORT = 0x03,   /* Abort current data transfer */
  /* At most one of these */
  EMMC_F_DATA_READ = 0x04,  /* Read data transfer is expected */
  EMMC_F_DATA_WRITE = 0x08, /* Write data transfer is expected */
  /* Any subset of these */
  EMMC_F_DATA_MULTI = 0x10, /* Multi-block transfer */
  EMMC_F_CHKIDX = 0x20,     /* Check commands index in response */
  EMMC_F_CHKCRC = 0x40,     /* Check CRC in response */
  EMMC_F_APP = 0x80,        /* App-specific command */
} emmc_cmd_flags_t;

static inline emmc_cmd_flags_t emmc_cmdtype(emmc_cmd_flags_t flags) {
  return flags & (EMMC_F_SUSPEND | EMMC_F_RESUME | EMMC_F_ABORT);
}

typedef struct emmc_cmd {
  emmc_cmd_idx_t cmd_idx;
  emmc_cmd_flags_t flags;
  emmc_resp_type_t exp_resp;
} emmc_cmd_t;

#define EMMC_VOLTAGE_WINDOW_LOW 0x01 /* 1.1V - 1.3V */
#define EMMC_VOLTAGE_WINDOW_MID 0x02 /* 1.65V - 1.95V */
#define EMMC_VOLTAGE_WINDOW_HI 0x04  /* 2.7V -3.6V */

#define EMMC_BUSWIDTH_1 0x01 /* 1-bit bus */
#define EMMC_BUSWIDTH_4 0x02 /* 4-bit bus */
#define EMMC_BUSWIDTH_8 0x04 /* 8-bit but */

typedef enum {
  EMMC_ERROR_TIMEOUT = 0x01,          /* Controller timed-out on a request */
  EMMC_ERROR_PROP_NOTSUP = 0x02,      /* Requested property is not supported */
  EMMC_ERROR_PROP_INVALID_ARG = 0x04, /* Can't set the property for the
                                       * requested value */
  EMMC_ERROR_INVALID_STATE = 0x08,    /* Controller was in an invalid state.
                                       * Request for operation has been
                                       * ignored. */
  EMMC_ERROR_INTERNAL = 0x10,         /* Raised if an error was unexpected.
                                       * If raised, stops further operations.
                                       * The only way to clear this flag is
                                       * to completely reset the controller
                                       * with `emmc_reset` */
} emmc_error_t;

/* R stands for "read"
 * W stands for "write" */
typedef enum emmc_prop_id {
  EMMC_PROP_RW_BLKSIZE,       /* Transfer block size */
  EMMC_PROP_RW_BLKCNT,        /* Transfer block count */
  EMMC_PROP_R_MAXBLKSIZE,     /* The biggest supported size of a block */
  EMMC_PROP_R_MAXBLKCNT,      /* The biggest supported number of blocks per
                               * transfer */
  EMMC_PROP_R_VOLTAGE_SUPPLY, /* A subset of EMMC_VOLTAGE_WINDOW_* flags,
                               * describing voltage windows in which the
                               * controller is able to operate */
  EMMC_PROP_RW_RESP_LOW,      /* Low 64 bits of response register(s) */
  EMMC_PROP_RW_RESP_HI,       /* High 64 bits of response register(s) */
  EMMC_PROP_RW_CLOCK_FREQ,    /* Clocking frequency (Hz) */
  EMMC_PROP_RW_BUSWIDTH,      /* Bus width, ie. no. of data lanes. */
  EMMC_PROP_RW_RCA,           /* Relative card address */
  EMMC_PROP_RW_ERRORS,        /* Last reported set of errors */
  EMMC_PROP_RW_ALLOW_ERRORS,  /* Do not assume invalid state on specified error
                               * flags. EMMC_ERROR_INVALID_STATE can't be
                               * allowed */
} emmc_prop_id_t;
typedef uint64_t emmc_prop_val_t;

/* For a detailed explanation on semantics refer to the comments above
 * respective wrappers */
typedef emmc_error_t (*emmc_send_cmd_t)(device_t *dev, emmc_cmd_t cmd,
                                        uint32_t arg, emmc_resp_t *res);
typedef emmc_error_t (*emmc_wait_t)(device_t *dev, emmc_wait_flags_t wflags);
typedef emmc_error_t (*emmc_read_dat_t)(device_t *dev, void *buf, size_t len,
                                        size_t *n);
typedef emmc_error_t (*emmc_write_dat_t)(device_t *dev, const void *buf,
                                         size_t len, size_t *n);
typedef emmc_error_t (*emmc_get_prop_t)(device_t *dev, emmc_prop_id_t id,
                                        emmc_prop_val_t *val);
typedef emmc_error_t (*emmc_set_prop_t)(device_t *dev, emmc_prop_id_t id,
                                        emmc_prop_val_t val);
typedef emmc_error_t (*emmc_reset_t)(device_t *dev);

typedef struct emmc_methods {
  emmc_send_cmd_t send_cmd;
  emmc_wait_t wait;
  emmc_read_dat_t read;
  emmc_write_dat_t write;
  emmc_get_prop_t get_prop;
  emmc_set_prop_t set_prop;
  emmc_reset_t reset;
} emmc_methods_t;

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
 * \param resp pointer for response data to be written to or NULL
 * \return 0 on success EMMC_ERROR_TIMEOUT on timeout, EMMC_ERROR_INVALID_STATE
 * on invalid internal state
 */
static inline emmc_error_t emmc_send_cmd(device_t *dev, emmc_cmd_t cmd,
                                         uint32_t arg, emmc_resp_t *resp) {
  device_t *idev = EMMC_METHOD_PROVIDER(dev, send_cmd);
  return emmc_methods(idev->parent)->send_cmd(dev, cmd, arg, resp);
}

/* NOTE: All of the methods below might return a mask of error codes instead of
 * a single code. */

/**
 * \brief Wait until e.MMC signals
 * \param dev e.MMC controller device
 * \param wflags conditions to be met
 * \return 0 on success EMMC_ERROR_TIMEOUT on timeout, EMMC_ERROR_INVALID_STATE
 * on invalid internal state
 */
static inline emmc_error_t emmc_wait(device_t *dev, emmc_wait_flags_t wflags) {
  device_t *idev = EMMC_METHOD_PROVIDER(dev, wait);
  return emmc_methods(idev->parent)->wait(dev, wflags);
}

/**
 * \brief Read data fetched by the controller
 * \param dev e.MMC controller device
 * \param buf pointer to where the data should be written to
 * \param len expected data length
 * \param n pointer for the number of read bytes or NULL
 * \return 0 on success EMMC_ERROR_TIMEOUT on timeout, EMMC_ERROR_INVALID_STATE
 * on invalid internal state
 */
static inline emmc_error_t emmc_read(device_t *dev, void *buf, size_t len,
                                     size_t *n) {
  device_t *idev = EMMC_METHOD_PROVIDER(dev, read);
  return emmc_methods(idev->parent)->read(dev, buf, len, n);
}

/**
 * \brief Write data to the controller
 * \param dev e.MMC controller device
 * \param buf pointer to where the data should be read from
 * \param len data length
 * \param n pointer for the number of written bytes in or NULL
 * \return 0 on success EMMC_ERROR_TIMEOUT on timeout, EMMC_ERROR_INVALID_STATE
 * on invalid internal state
 */
static inline emmc_error_t emmc_write(device_t *dev, const void *buf,
                                      size_t len, size_t *n) {
  device_t *idev = EMMC_METHOD_PROVIDER(dev, write);
  return emmc_methods(idev->parent)->write(dev, buf, len, n);
}

/**
 * \brief Gets a value describing the e.MMC device
 * \param dev e.MMC controller device
 * \param id value identifier
 * \param var pointer to where the associated value should be written to
 * \return 0 on success EMMC_ERROR_TIMEOUT on timeout, EMMC_ERROR_PROP_NOTSUP
 * if the property is not supported, EMMC_ERROR_INVALID_STATE
 * on invalid internal state (unless id = EMMC_PROP_RW_ERRORS)
 */
static inline emmc_error_t emmc_get_prop(device_t *dev, emmc_prop_id_t id,
                                         emmc_prop_val_t *val) {
  device_t *idev = EMMC_METHOD_PROVIDER(dev, get_prop);
  return emmc_methods(idev->parent)->get_prop(dev, id, val);
}

/**
 * \brief Sets a value describing the e.MMC device
 * \param dev e.MMC controller device
 * \param id value identifier
 * \param var pointer to where the associated value should be written to
 * \return 0 on success EMMC_ERROR_TIMEOUT on timeout, EMMC_ERROR_PROP_NOTSUP
 * if the property is not supported, EMMC_ERROR_PROP_INVALID_ARG if provided
 * value is invalid for a given proprty, EMMC_ERROR_INVALID_STATE on invalid
 * internal state
 */
static inline emmc_error_t emmc_set_prop(device_t *dev, emmc_prop_id_t id,
                                         emmc_prop_val_t val) {
  device_t *idev = EMMC_METHOD_PROVIDER(dev, set_prop);
  return emmc_methods(idev->parent)->set_prop(dev, id, val);
}

/**
 * Reset the e.MMC controller. This should bring the controller back from
 * an invalid state to an intial state.
 * \param dev e.MMC controller device
 * \return 0 on success, non-zero on failure
 */
static inline emmc_error_t emmc_reset(device_t *dev) {
  device_t *idev = EMMC_METHOD_PROVIDER(dev, reset);
  return emmc_methods(idev->parent)->reset(dev);
}

/* Use if a command is expected to respond with R1b */
static inline emmc_cmd_t emmc_r1b(emmc_cmd_t cmd) {
  cmd.exp_resp = EMMCRESP_R1B;
  return cmd;
}

/* Disclaimer: Responses of type R1 and R1b are all reported as R1.
 * If device is expected to respond with R1b, wrap the command with
 * `emmc_r1b`, like `emmc_r1b(EMMC_CMD(SWITCH)). */
extern const emmc_cmd_t default_emmc_commands[57];
#define EMMC_CMD(idx) default_emmc_commands[EMMC_CMD_##idx]

#endif /* !_DEV_EMMC_H_ */
