#ifndef _CPIO_H_
#include <vm.h>
#include <vnode.h>

typedef struct cpio_hdr {
  char c_magic[6]; /* "070701" for "new" portable format
                      "070702" for CRC format */
  char c_ino[8];
  char c_mode[8];
  char c_uid[8];
  char c_gid[8];
  char c_nlink[8];
  char c_mtime[8];
  char c_filesize[8]; /* must be 0 for FIFOs and directories */
  char c_dev_maj[8];
  char c_dev_min[8];
  char c_rdev_maj[8]; /* only valid for chr and blk special files */
  char c_rdev_min[8]; /* only valid for chr and blk special files */
  char c_namesize[8]; /* count includes terminating NUL in pathname */
  char c_chksum[8];   /* 0 for "new" portable format; for CRC format
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
  char *c_data;

  vnode_t *vnode;

  TAILQ_ENTRY(cpio_file_stat) stat_list;

} cpio_file_stat_t;

typedef TAILQ_HEAD(stat_head, cpio_file_stat) stat_head_t;

vm_addr_t initrd_get_start();
vm_addr_t initrd_get_size();
stat_head_t *initrd_get_headers();

void initrd_collect_headers(stat_head_t *hd, char *tape);
void ramdisk_init();
void dump_cpio_stat(cpio_file_stat_t *stat);

#endif /* _CPIO_H_ */
