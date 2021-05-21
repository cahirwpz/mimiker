#include <sys/event.h>
#include <sys/errno.h>

int do_kqueue1(proc_t *p, int flags, int *fd) {
  return ENOTSUP;
}

int do_kevent(proc_t *p, int kq, kevent_t *changelist, size_t nchanges,
              kevent_t *eventlist, size_t nevents, timespec_t *timeout,
              int *retval) {
  return ENOTSUP;
}

void knote(knlist_t *list, long hint) {
}
