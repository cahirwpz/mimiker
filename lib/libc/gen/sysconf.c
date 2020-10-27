#include <sys/syslimits.h>
#include <unistd.h>
#include <errno.h>
#include <pwd.h>

long sysconf(int name) {
  switch (name) {
    case _SC_ARG_MAX:
      return ARG_MAX;
    case _SC_CHILD_MAX:
      return CHILD_MAX;
    case _SC_NGROUPS_MAX:
      return NGROUPS_MAX;
    case _SC_OPEN_MAX:
      return OPEN_MAX;
    case _SC_PASS_MAX:
      return _PASSWORD_LEN;
    case _SC_JOB_CONTROL:
    default:
      errno = EINVAL;
      return -1;
  }
}
