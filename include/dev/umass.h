/*
 * Based on definitions contained in FreeBSD `umass.c`.
 */
#ifndef _UMASS_H_
#define _UMASS_H_

/* Bulk-Only features */

#define UR_BBB_RESET 0xff       /* Bulk-Only reset */
#define UR_BBB_GET_MAX_LUN 0xfe /* Get maximum lun */

/* Command Block Wrapper */
#define CBWCDBLENGTH 16

typedef struct {
  uint32_t dCBWSignature;
  uint32_t dCBWTag;
  uint32_t dCBWDataTransferLength;
  uint8_t bCBWFlags;
  uint8_t bCBWLUN;
  uint8_t bCDBLength;
  uint8_t CBWCDB[CBWCDBLENGTH];
} __packed umass_bbb_cbw_t;

#define CBWSIGNATURE 0x43425355

#define CBWFLAGS_OUT 0x00
#define CBWFLAGS_IN 0x80

/* Command Status Wrapper */
typedef struct {
  uint32_t dCSWSignature;
  uint32_t dCSWTag;
  uint32_t dCSWDataResidue;
  uint8_t bCSWStatus;
} __packed umass_bbb_csw_t;

#define CSWSIGNATURE 0x53425355

#define CSWSTATUS_GOOD 0x0
#define CSWSTATUS_FAILED 0x1

#endif /* _UMASS_H_ */
