#ifndef _CPIO_H_
#include <vm.h>
typedef struct cpio_hdr
{
  char c_magic[6];     /* "070701" for "new" portable format
			  "070702" for CRC format */
  char c_ino[8];
  char c_mode[8];
  char c_uid[8];
  char c_gid[8];
  char c_nlink[8];
  char c_mtime[8];
  char c_filesize[8];  /* must be 0 for FIFOs and directories */
  char c_dev_maj[8];
  char c_dev_min[8];
  char c_rdev_maj[8];      /* only valid for chr and blk special files */
  char c_rdev_min[8];      /* only valid for chr and blk special files */
  char c_namesize[8];  /* count includes terminating NUL in pathname */
  char c_chksum[8];    /* 0 for "new" portable format; for CRC format
			  the sum of all the bytes in the file  */
} cpio_hdr_t;


typedef struct cpio_file_stat /* Internal representation of a CPIO header */
{
  unsigned short c_magic;
  ino_t c_ino;
  mode_t c_mode;
  uid_t c_uid;
  gid_t c_gid;
  size_t c_nlink;
  time_t c_mtime;
  off_t c_filesize;
  long c_dev_maj;
  long c_dev_min;
  long c_rdev_maj;
  long c_rdev_min;
  size_t c_namesize;
  uint32_t c_chksum;
  char *c_name;
  //char *c_tar_linkname; /* We probably don't need this field, but need to make sure */
} cpio_file_stat_t;

vm_addr_t get_rd_start();
vm_addr_t get_rd_size();

void cpio_init();
void ramdisk_init(vm_addr_t, vm_addr_t);
void dump_cpio_stat(cpio_file_stat_t *stat);
void fill_header(char** tape, cpio_file_stat_t *hdr);

#endif /* _CPIO_H_ */
