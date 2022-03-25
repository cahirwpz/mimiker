/* Based on NetBSD's sys/sys/event.h */

#ifndef _SYS_EVENT_H_
#define _SYS_EVENT_H_

#include <sys/types.h>

/*
 * kevent - kernel event notification mechanism. The concept of kevent is taken
 * from *BSDs, so for the details please refer to the kevent(2).
 *
 * IMPORTANT: for the sake of simplicity we assumed the following:
 *  - kqueue isn't thread safe. All operations on kqueue, that is adding,
 *    removing or waiting for events should be done on a single thread. However,
 *    this does not apply to issuing events. In other words, any thread can
 *    report an event to the kqueue.
 *  - The queue is not inherited by a child created with fork.
 */

/* Filter types */
#define EVFILT_READ 0U
#define EVFILT_WRITE 1U
#define EVFILT_SYSCOUNT 2U /* number of filters */

struct kevent {
  uintptr_t ident; /* identifier for this event */
  uint32_t filter; /* filter for event */
  uint32_t flags;  /* action flags for kqueue */
  uint32_t fflags; /* filter flag value */
  int64_t data;    /* filter data value */
  void *udata;     /* opaque user data identifier */
};

#define EV_SET(kevp_, a, b, c, d, e, f)                                        \
  *(kevp_) = (struct kevent){                                                  \
    .ident = (a),                                                              \
    .filter = (b),                                                             \
    .flags = (c),                                                              \
    .fflags = (d),                                                             \
    .data = (e),                                                               \
    .udata = (f),                                                              \
  };

/* actions */
#define EV_ADD 0x0001U    /* add event to kq */
#define EV_DELETE 0x0002U /* delete event from kq */

/* returned values */
#define EV_ERROR 0x4000U /* error, data contains errno */

#ifdef _KERNEL

#include <sys/queue.h>
#include <sys/time.h>

typedef struct knote knote_t;
typedef struct kevent kevent_t;
typedef struct kqueue kqueue_t;
typedef struct mtx mtx_t;

typedef int filt_attach_t(knote_t *kn);
typedef void filt_detach_t(knote_t *kn);
typedef int filt_event_t(knote_t *kn, long hint);

/*
 * Callback methods for each filter type.
 */
typedef struct filterops {
  filt_attach_t *filt_attach;
  filt_detach_t *filt_detach;
  filt_event_t *filt_event;
} filterops_t;

typedef SLIST_HEAD(, knote) knlist_t;

/* Status of knote. */
#define KN_QUEUED 0x01U /* event is on queue */

/*
 * Field locking:
 *
 * (q) - kn_kq->kq_lock
 * (o) - object mutex (e.g. device driver or vnode interlock)
 * (!) - read-only access
 */

typedef struct knote {
  SLIST_ENTRY(knote) kn_hashlink; /* (q) for hashmap in kqueue */
  TAILQ_ENTRY(knote) kn_penlink;  /* (q) for list of pending events in kqueue */
  SLIST_ENTRY(knote) kn_objlink;  /* (o) for list of knotes in the object*/
  kqueue_t *kn_kq;                /* (!) which queue we are on */
  void *kn_obj;                   /* (!) monitored object */
  uint32_t kn_status;             /* (q) flags above */

  /* (o) only applies to `fflags`, `data`, `udata`. The rest should remain
   * unmodified. */
  kevent_t kn_kevent;

  /* Following fields should be only set in the filt_attach function and aren't
   * protected by any lock. */
  filterops_t *kn_filtops; /* Filter operations */
  void *kn_hook;           /* Arbitrary data set by filter */
  mtx_t *kn_objlock;       /* Lock protecting the object */
} knote_t;

/*
 * Walk down a list of knotes, activating them if their event has
 * triggered.  The caller's object lock (e.g. device driver lock)
 * must be held.
 */
void knote(knlist_t *knlist, long hint);

int do_kqueue1(proc_t *p, int flags, int *fd);
int do_kevent(proc_t *p, int kq, kevent_t *changelist, size_t nchanges,
              kevent_t *eventlist, size_t nevents, timespec_t *timeout,
              int *retval);

#else /* !_KERNEL */

struct timespec;

__BEGIN_DECLS
int kqueue(void);
int kqueue1(int);
int kevent(int, const struct kevent *, size_t, struct kevent *, size_t,
           const struct timespec *);
__END_DECLS

#endif /* !_KERNEL */

#endif /* !_SYS_EVENT_H_ */
