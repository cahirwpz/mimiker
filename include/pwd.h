#ifndef _PWD_H_
#define _PWD_H_

#include <sys/types.h>

struct passwd {
  const char *pw_name;   /* user name */
  const char *pw_passwd; /* encrypted password */
  uid_t pw_uid;          /* user uid */
  gid_t pw_gid;          /* user gid */
  time_t pw_change;      /* password change time */
  const char *pw_class;  /* user login class */
  const char *pw_gecos;  /* general information */
  const char *pw_dir;    /* home directory */
  const char *pw_shell;  /* default shell */
  time_t pw_expire;      /* account expiration */
};

__BEGIN_DECLS
struct passwd *getpwnam(const char *);
const char *user_from_uid(uid_t, int);
__END_DECLS

#endif /* !_PWD_H_ */
