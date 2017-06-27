#ifndef _SYS_UNISTD_H
#define _SYS_UNISTD_H_

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define F_OK 0      /* test for existence of file */
#define X_OK 0x01   /* test for execute or search permission */
#define W_OK 0x02   /* test for write permission */
#define R_OK 0x04   /* test for read permission */
#define ALL_OK 0x07 /* test all execute, write, read */

#endif /* !_SYS_UNISTD_H_ */
