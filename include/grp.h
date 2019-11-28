#ifndef _GRP_H_
#define _GRP_H_

#include <sys/types.h>

struct group {
  const char *gr_name;       /* group name */
  const char *gr_passwd;     /* group password */
  gid_t gr_gid;              /* group id */
  const char *const *gr_mem; /* group members */
};

__BEGIN_DECLS
struct group *getgrnam(const char *);
const char *group_from_gid(gid_t, int);
__END_DECLS

#endif /* !_GRP_H_ */
