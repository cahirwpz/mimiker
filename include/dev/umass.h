#ifndef _UMASS_H_
#define _UMASS_H_

/* Bulk-Only features */

#define UR_BULK_ONLY_RESET 0xff /* Bulk-Only reset */
#define UR_GET_MAX_LUN 0xfe     /* Get maximum lun */

/* Command Block Wrapper */
typedef struct {
  uint32_t signature;
  uint32_t tag;
  uint32_t data_len;
  uint8_t flags;
  uint8_t lun;
  uint8_t cmd_len;
  uint8_t cmd[16];
} __packed umass_cbw_t;

#define CBWSIGNATURE 0x43425355

#define CBWFLAGS_OUT 0x00
#define CBWFLAGS_IN 0x80

#define CBWCDBLENGTH 16

/* Command Status Wrapper */
typedef struct {
  uint32_t signature;
  uint32_t tag;
  uint32_t data_residue;
  uint8_t status;
} __packed umass_csw_t;

#define CSWSIGNATURE 0x53425355

#define CSWSTATUS_GOOD 0x0
#define CSWSTATUS_FAILED 0x1

#endif /* _UMASS_H_ */
