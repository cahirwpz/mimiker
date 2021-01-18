#ifndef _SCSI_H_
#define _SCSI_H_

/* Opcodes. */
#define REQUEST_SENSE 0x03
#define INQUIRY 0x12
#define READ_CAPACITY 0x25
#define READ_10 0x28
#define WRITE_10 0x2A

typedef struct scsi_request_sense {
  uint8_t opcode; /* REQUEST_SENSE */
  uint8_t byte2;
  uint8_t unused[2];
  uint8_t length;
  uint8_t control;
} __packed scsi_request_sense_t;

typedef struct {
  uint8_t opcode; /* INQUIRY */
  uint8_t byte2;
  uint8_t page_code;
  uint16_t length;
  uint8_t control;
} __packed scsi_inquiry_t;

typedef struct scsi_read_capacity {
  uint8_t opcode; /* READ_CAPACITY */
  uint8_t byte2;
  uint8_t addr[4];
  uint8_t unused[2];
  uint8_t pmi;
  uint8_t control;
} __packed scsi_read_capacity_t;

typedef struct scsi_rw_10 {
  uint8_t opcode;
  uint8_t byte2;
  uint32_t addr;
  uint8_t reserved;
  uint16_t length;
  uint8_t control;
} __packed scsi_rw_10_t;

typedef struct scsi_sense_data {
  uint8_t error_code;
  uint8_t segment;
  uint8_t flags;
  uint8_t info[4];
  uint8_t extra_len;
  uint8_t cmd_spec_info[4];
  uint8_t add_sense_code;
  uint8_t add_sense_code_qual;
  uint8_t fru;
  uint8_t sense_key_spec[3];
} __packed scsi_sense_data_t;

#define SHORT_INQUIRY_LENGTH 36

#define SID_VENDOR_SIZE 8
#define SID_PRODUCT_SIZE 16
#define SID_REVISION_SIZE 4

typedef struct {
  uint8_t device;
  uint8_t dev_qual2;
  uint8_t version;
  uint8_t response_format;
  uint8_t additional_length;
  uint8_t spc3_flags;
  uint8_t spc2_flags;
  uint8_t flags;
  char vendor[SID_VENDOR_SIZE];
  char product[SID_PRODUCT_SIZE];
  char revision[SID_REVISION_SIZE];
} __packed scsi_inquiry_data_t;

#define SID_TYPE(inq_data) ((inq_data)->device & 0x1f)

#define SID_QUAL(inq_data) (((inq_data)->device & 0xE0) >> 5)
#define SID_QUAL_LU_CONNECTED 0x00

#define SID_RMB 0x80
#define SID_IS_REMOVABLE(inq_data) (((inq_data)->dev_qual2 & SID_RMB) != 0)

#define SID_FORMAT(inq_data) ((inq_data)->response_format & 0x0f)

typedef struct scsi_read_capacity_data {
  uint32_t addr;
  uint32_t length;
} __packed scsi_read_capacity_data_t;

#define REQUEST_TIMEOUT 4

#endif /* _SCSI_H_ */
