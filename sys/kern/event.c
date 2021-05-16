#include <sys/event.h>
#include <sys/mimiker.h>
#include <sys/libkern.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/condvar.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/pool.h>
#include <sys/time.h>

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
