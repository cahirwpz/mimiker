#ifndef _SYS_UNISTD_H_
#define _SYS_UNISTD_H_

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define F_OK 0 /* test for existence of file */
#define X_OK 1 /* test for execute or search permission */
#define W_OK 2 /* test for write permission */
#define R_OK 4 /* test for read permission */

/* configurable pathname variables; use as argument to pathconf(3) */
#define _PC_LINK_MAX 1
#define _PC_MAX_CANON 2
#define _PC_MAX_INPUT 3
#define _PC_NAME_MAX 4
#define _PC_PATH_MAX 5
#define _PC_PIPE_BUF 6

/* configurable system variables; use as argument to sysconf(3) */
#define _SC_ARG_MAX 1
#define _SC_CHILD_MAX 2
#define _SC_NGROUPS_MAX 4
#define _SC_OPEN_MAX 5
#define _SC_JOB_CONTROL 6
#define _SC_PASS_MAX 7

#endif /* !_SYS_UNISTD_H_ */
