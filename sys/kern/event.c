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

#define KN_HASHSIZE 8

static POOL_DEFINE(P_KNOTE, "knote", sizeof(knote_t));

static kqueue_t *kqueue_create(void);
static void kqueue_drain(kqueue_t *kq);
static void kqueue_destroy(kqueue_t *kq);

static void knote_enqueue(knote_t *kn);
static void knote_dequeue(knote_t *kn);
static void knote_drop(knote_t *kn);

typedef TAILQ_HEAD(, knote) knote_tailq_t;

/* kqueue stands for the kernel event queue. Each event that can be triggered
 * in a kqueue is represented by knote (owned by that instance of kqueue).
 *
 * All fields are protected by `kq_lock`.
 *
 * `kn_objlock` (the lock protecting the object) must be taken BEFORE `kq_lock`.
 */
typedef struct kqueue {
  int kq_count;          /* number of pending events */
  knote_tailq_t kq_head; /* list of pending events */
  mtx_t kq_lock;         /* mutex for queue access */
  condvar_t kq_cv;
  knlist_t kq_knhash[KN_HASHSIZE]; /* hash table for knotes */
} kqueue_t;

/* Returns a hash bucket for a given object */
static inline knlist_t *kq_get_hashbucket(kqueue_t *kq, void *obj) {
  return &kq->kq_knhash[((uintptr_t)obj) % KN_HASHSIZE];
}

static kqueue_t *kqueue_create(void) {
  kqueue_t *kq = kmalloc(M_DEV, sizeof(kqueue_t), M_ZERO);
  mtx_init(&kq->kq_lock, 0);
  cv_init(&kq->kq_cv, 0);

  TAILQ_INIT(&kq->kq_head);
  for (int i = 0; i < KN_HASHSIZE; i++)
    SLIST_INIT(&kq->kq_knhash[i]);

  return kq;
}

/* Disconnects all knotes from the given kqueue. */
static void kqueue_drain(kqueue_t *kq) {
  knote_t *kn;

  for (int i = 0; i < KN_HASHSIZE; i++) {
    while ((kn = SLIST_FIRST(&kq->kq_knhash[i])) != NULL) {
      knote_drop(kn);
    }
  }
}

static void kqueue_destroy(kqueue_t *kq) {
  kqueue_drain(kq);
  cv_destroy(&kq->kq_cv);
  mtx_destroy(&kq->kq_lock);
  kfree(M_DEV, kq);
}

static int filt_fileattach(knote_t *kn) {
  file_t *fp = kn->kn_obj;
  return fp->f_ops->fo_kqfilter(fp, kn);
}

/* Here we only need to define filt_attach function, as the rest will be
 * specified in filterops of a concrete file type.
 */
static filterops_t file_filtops = {
  .filt_attach = filt_fileattach,
};

static filterops_t *sys_kfilters[EVFILT_SYSCOUNT] = {
  [EVFILT_READ] = &file_filtops,
  [EVFILT_WRITE] = &file_filtops,
};

static filterops_t *filt_getops(uint32_t filter) {
  if (filter >= EVFILT_SYSCOUNT)
    return NULL;
  return sys_kfilters[filter];
}

static int kqueue_read(file_t *f, uio_t *uio) {
  return EOPNOTSUPP;
}

static int kqueue_write(file_t *f, uio_t *uio) {
  return EOPNOTSUPP;
}

static int kqueue_close(file_t *f) {
  kqueue_t *kq = f->f_data;
  kqueue_destroy(kq);
  f->f_data = NULL;

  return 0;
}

static int kqueue_stat(file_t *f, stat_t *sb) {
  return EOPNOTSUPP;
}

static int kqueue_seek(file_t *f, off_t offset, int whence, off_t *newoffp) {
  return EOPNOTSUPP;
}

static int kqueue_ioctl(file_t *f, u_long cmd, void *data) {
  return EOPNOTSUPP;
}

static fileops_t kqueueops = {
  .fo_read = kqueue_read,
  .fo_write = kqueue_write,
  .fo_close = kqueue_close,
  .fo_stat = kqueue_stat,
  .fo_seek = kqueue_seek,
  .fo_ioctl = kqueue_ioctl,
};

static int kqueue_get_obj(proc_t *p, kevent_t *kev, void **obj) {
  if (kev->filter == EVFILT_READ || kev->filter == EVFILT_WRITE)
    return fdtab_get_file(p->p_fdtable, kev->ident, FF_READ, (file_t **)obj);

  return EINVAL;
}

/* Drops the object connected to the knote. */
static void knote_drop_obj(knote_t *kn) {
  uint32_t filter = kn->kn_kevent.filter;
  if (filter == EVFILT_READ || filter == EVFILT_WRITE)
    file_drop(kn->kn_obj);
}

/* Drops an already detached knote. */
static void knote_drop_detached(knote_t *kn) {
  kqueue_t *kq = kn->kn_kq;
  knlist_t *knote_list = kq_get_hashbucket(kq, kn->kn_obj);
  SLIST_REMOVE(knote_list, kn, knote, kn_hashlink);

  WITH_MTX_LOCK (&kq->kq_lock) {
    if (kn->kn_status & KN_QUEUED) {
      knote_dequeue(kn);
    }
  }

  knote_drop_obj(kn);
  pool_free(P_KNOTE, kn);
}

/* Detaches the knote from the object and then removes it. */
static void knote_drop(knote_t *kn) {
  kn->kn_filtops->filt_detach(kn);
  knote_drop_detached(kn);
}

/* Modifies the knote based on the flags in the kevent (or creates
 * a new one if necessary).
 */
static int kqueue_register(kqueue_t *kq, kevent_t *kev, void *obj) {
  knote_t *kn = NULL;
  int error, event;

  filterops_t *filtops = filt_getops(kev->filter);

  /* The filter is invalid. */
  if (filtops == NULL)
    return EINVAL;

  /* Find an existing knote to use for this kevent. */
  knlist_t *knote_list = kq_get_hashbucket(kq, obj);
  SLIST_FOREACH(kn, knote_list, kn_hashlink) {
    if (kev->filter == kn->kn_kevent.filter && kn->kn_obj == obj)
      break;
  }

  /* There isn't the matching knote. Create a new one. */
  if (kn == NULL) {
    /* We weren't asked to create a new knote, so just return an error. */
    if ((kev->flags & EV_ADD) == 0)
      return ENOENT;

    kn = pool_alloc(P_KNOTE, M_ZERO);
    kn->kn_kq = kq;
    kn->kn_kevent = *kev;
    kn->kn_obj = obj;
    kn->kn_filtops = filtops;

    SLIST_INSERT_HEAD(knote_list, kn, kn_hashlink);

    if ((error = kn->kn_filtops->filt_attach(kn)) != 0) {
      knote_drop_detached(kn);
      return error;
    }
  }

  if (kev->flags & EV_DELETE) {
    knote_drop(kn);
    return 0;
  }

  /* `kn_objlock` must be taken to access kn_kevent.udata and
   * to call filt_event.
   */

  WITH_MTX_LOCK (kn->kn_objlock) {
    /*
     * The user may change some filter values after the
     * initial EV_ADD, but doing so will not reset any
     * filter which have already been triggered.
     */
    kn->kn_kevent.udata = kev->udata;

    event = kn->kn_filtops->filt_event(kn, 0);
    WITH_MTX_LOCK (&kq->kq_lock) {
      if (event && (kn->kn_status & KN_QUEUED) == 0)
        knote_enqueue(kn);
    }
  }

  return 0;
}

/* Scans for events triggered in a given kqueue. Returns at most `nevents`
 * events via `eventlist`. If there are no events available, blocks for at
 * most `tsp`. If `tsp` is null, blocks infinitely. If the timeout is not
 * positive, returns immediately.
 */
static int kqueue_scan(kqueue_t *kq, kevent_t *eventlist, size_t nevents,
                       timespec_t *tsp, int *retval) {
  int error, event, timeout;
  size_t count = 0;
  systime_t sleepts;
  knote_tailq_t knqueue;
  knote_t *kn;

  TAILQ_INIT(&knqueue);

  if (tsp) {
    timeout = ts2hz(tsp);
    sleepts = getsystime() + timeout;
    if (timeout <= 0)
      timeout = -1; /* don't block */
  } else {
    timeout = 0; /* no timeout, wait forever */
  }

  mtx_lock(&kq->kq_lock);

  /* Block until there are no events or we time out. */
  while (kq->kq_count == 0) {
    if (timeout < 0) {
      error = 0;
      goto done;
    }

    error = cv_wait_timed(&kq->kq_cv, &kq->kq_lock, timeout);
    if (error == EINTR) {
      mtx_unlock(&kq->kq_lock);
      return EINTR;
    }

    if (tsp)
      timeout = sleepts - getsystime();
  }

  /* To ensure the correctness of the iteration over pending events,
   * we need to move already processed knotes to another list. `kq_head`
   * can change in between iterations, because we are releasing the lock
   * for a moment.
   */

  while (count < nevents) {
    assert(mtx_owned(&kq->kq_lock));

    if (TAILQ_EMPTY(&kq->kq_head))
      break;

    kn = TAILQ_FIRST(&kq->kq_head);
    TAILQ_REMOVE(&kq->kq_head, kn, kn_penlink);

    /* To call `filt_event` we need to lock `kn_objlock`, but firstly
     * we need to release `kq_lock` since holding it would violate the locking
     * order. `kn_objlock` must be taken before `kq_lock`.
     */
    mtx_unlock(&kq->kq_lock);
    WITH_MTX_LOCK (kn->kn_objlock) {
      event = kn->kn_filtops->filt_event(kn, 0);
    }
    mtx_lock(&kq->kq_lock);

    /* Make sure that the event is still active. */
    if (event == 0) {
      kn->kn_status &= ~KN_QUEUED;
      kq->kq_count--;
      continue;
    }

    TAILQ_INSERT_HEAD(&knqueue, kn, kn_penlink);

    eventlist[count++] = kn->kn_kevent;
  }

  TAILQ_CONCAT(&kq->kq_head, &knqueue, kn_penlink);

done:
  mtx_unlock(&kq->kq_lock);
  *retval = count;

  return 0;
}

static int kevent(proc_t *p, kqueue_t *kq, kevent_t *changelist,
                  size_t nchanges, kevent_t *eventlist, size_t nevents,
                  timespec_t *timeout, int *retval) {
  int error;
  size_t nerrors = 0;
  kevent_t *kev;
  void *obj;

  /* First, make changes to the kqueue as requested. */
  for (size_t i = 0; i < nchanges; i++) {
    kev = &changelist[i];

    error = kqueue_get_obj(p, kev, &obj);

    if (error == 0 && (error = kqueue_register(kq, kev, obj)) == 0)
      continue;

    /* No more space left in the the eventlist, so simply return the error. */
    if (nevents == nerrors)
      return error;

    kev->flags = EV_ERROR;
    kev->data = error;
    memcpy(&eventlist[nerrors], kev, sizeof(kevent_t));
    nerrors++;
  }

  if (nerrors) {
    *retval = nerrors;
    return 0;
  }

  /* Second, scan for triggered events. */
  return kqueue_scan(kq, eventlist, nevents, timeout, retval);
}

/* The entry point of kqueue1(2) syscall. */
int do_kqueue1(proc_t *p, int flags, int *fd) {
  int error;
  kqueue_t *kq = kqueue_create();

  file_t *file = file_alloc();
  file->f_data = kq;
  file->f_ops = &kqueueops;
  file->f_type = FT_KQUEUE;
  file->f_flags = 0;

  if (!(error = fdtab_install_file(p->p_fdtable, file, 0, fd))) {
    if (!(error = fd_set_cloexec(p->p_fdtable, *fd, (flags & O_CLOEXEC) != 0)))
      return 0;

    fdtab_close_fd(p->p_fdtable, *fd);
  }

  kqueue_close(file);
  return error;
}

/* The entry point of kevent(2) syscall. */
int do_kevent(proc_t *p, int kq, kevent_t *changelist, size_t nchanges,
              kevent_t *eventlist, size_t nevents, timespec_t *timeout,
              int *retval) {
  file_t *f;
  int error;

  if ((error = fdtab_get_file(p->p_fdtable, kq, 0, &f)))
    return error;

  if (f->f_type != FT_KQUEUE) {
    file_drop(f);
    return EBADF;
  }

  error = kevent(p, f->f_data, changelist, nchanges, eventlist, nevents,
                 timeout, retval);
  file_drop(f);

  return error;
}

/*
 * Queue a new event for knote.
 *
 * `kq_lock` must be held.
 */
static void knote_enqueue(knote_t *kn) {
  kqueue_t *kq = kn->kn_kq;
  assert(mtx_owned(&kq->kq_lock));

  TAILQ_INSERT_TAIL(&kq->kq_head, kn, kn_penlink);
  kq->kq_count++;
  kn->kn_status |= KN_QUEUED;

  cv_broadcast(&kq->kq_cv);
}

/*
 * Remove a queued event from the kqueue.
 *
 * `kq_lock` must be held.
 */
static void knote_dequeue(knote_t *kn) {
  kqueue_t *kq = kn->kn_kq;
  assert(mtx_owned(&kq->kq_lock));

  TAILQ_REMOVE(&kq->kq_head, kn, kn_penlink);
  kn->kn_status &= ~KN_QUEUED;
  kq->kq_count--;
}

void knote(knlist_t *list, long hint) {
  knote_t *kn;

  SLIST_FOREACH(kn, list, kn_objlink) {
    assert(mtx_owned(kn->kn_objlock));

    if (kn->kn_filtops->filt_event(kn, hint)) {
      SCOPED_MTX_LOCK(&kn->kn_kq->kq_lock);
      if (kn->kn_status & KN_QUEUED)
        continue;

      knote_enqueue(kn);
    }
  }
}
