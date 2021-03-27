#include <sys/vnode.h>
#include <sys/filedesc.h>
#include <sys/mutex.h>
#include <sys/ringbuf.h>
#include <sys/ttycom.h>
#include <stdatomic.h>
#include <sys/condvar.h>
#include <sys/mimiker.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/devfs.h>
#include <sys/linker_set.h>
#include <sys/tty.h>

#define MAX_PTYS 16

/* Must be greater than or equal to the length of the path to any slave device.
 * Right now slave devices are at /dev/pts/<number>, so this should suffice. */
#define PTY_PATH_MAX_LEN 15

static devfs_node_t *pts_dir;

typedef struct {
  atomic_int pt_number; /* PTY number, if allocated. -1 means free. */
  condvar_t pt_incv;    /* CV for readers */
  condvar_t pt_outcv;   /* CV for writers */
} pty_t;

static pty_t pty_array[MAX_PTYS];

static pty_t *pty_alloc(void) {
  for (int i = 0; i < MAX_PTYS; i++) {
    pty_t *pty = &pty_array[i];
    if (atomic_compare_exchange_strong(&pty->pt_number, &(int){-1}, i + 1))
      /* Got a free PTY. */
      return pty;
  }
  return NULL;
}

static void pty_free(pty_t *pty) {
  assert(pty->pt_number >= 0);
  pty->pt_number = -1;
}

static int pty_read(file_t *f, uio_t *uio) {
  tty_t *tty = f->f_data;
  pty_t *pty = tty->t_data;
  int error;
  size_t start_resid = uio->uio_resid;

  if (uio->uio_resid == 0)
    return 0;

  SCOPED_MTX_LOCK(&tty->t_lock);

  /* Wait until there is at least one byte of data. */
  while (ringbuf_empty(&tty->t_outq)) {
    /* Don't wait for data if slave device isn't opened. */
    if (!tty_opened(tty))
      return 0;
    if (cv_wait_intr(&pty->pt_incv, &tty->t_lock))
      return ERESTARTSYS;
  }

  /* Data is available: transfer as much as we can. */
  error = ringbuf_read(&tty->t_outq, uio);
  tty_getc_done(tty);

  /* Don't report errors on partial reads. */
  if (start_resid > uio->uio_resid)
    error = 0;

  return error;
}

/* Write at single character to the master side of a pseudoterminal,
 * i.e. to the input queue of the slave tty.
 * If the input queue is full, sleep only if the slave tty has users. */
static int pty_putc_sleep(tty_t *tty, pty_t *pty, uint8_t c) {
  while (!tty_input(tty, c)) {
    if (!tty_opened(tty))
      return EIO;
    if (cv_wait_intr(&pty->pt_outcv, &tty->t_lock))
      return ERESTARTSYS;
  }
  return 0;
}

static int pty_write(file_t *f, uio_t *uio) {
  tty_t *tty = f->f_data;
  pty_t *pty = tty->t_data;
  int error = 0;
  uint8_t c;

  if (uio->uio_resid == 0)
    return 0;

  size_t start_resid = uio->uio_resid;
  uiostate_t save;

  SCOPED_MTX_LOCK(&tty->t_lock);

  while (uio->uio_resid > 0) {
    uio_save(uio, &save);
    if ((error = uiomove(&c, 1, uio)))
      break;
    if ((error = pty_putc_sleep(tty, pty, c))) {
      /* Undo the last uiomove(). */
      uio_restore(uio, &save);
      break;
    }
  }

  /* Don't report errors on partial writes. */
  if (start_resid > uio->uio_resid)
    error = 0;

  return error;
}

static int pty_close(file_t *f) {
  /* PTY master devices can't be accessed via the filesystem, so here we know
   * that the device won't ever be used again. */
  tty_t *tty = f->f_data;
  pty_t *pty = tty->t_data;

  mtx_lock(&tty->t_lock);
  tty_detach_driver(tty); /* Releases t_lock. */
  pty_free(pty);
  return 0;
}

static int pty_seek(file_t *f, off_t offset, int whence, off_t *newoffp) {
  return ESPIPE;
}

static int pty_stat(file_t *f, stat_t *sb) {
  /* Delegate to the slave tty.
   * We can't call default_vnstat() because we don't have a file_t pointing to
   * the slave tty, so we need to copy a bit of code from default_vnstat(). */
  tty_t *tty = f->f_data;
  vattr_t va;
  int error;
  if ((error = VOP_GETATTR(tty->t_vnode, &va)))
    return error;
  vattr_convert(&va, sb);
  return 0;
}

static int pty_ioctl(file_t *f, u_long cmd, void *data) {
  tty_t *tty = f->f_data;

  switch (cmd) {
    case TIOCPTSNAME: {
      char *user_buf = *(char **)data;
      char tmp_buf[PTY_PATH_MAX_LEN + 1];
      int error;
      pty_t *pty = tty->t_data;

      snprintf(tmp_buf, sizeof(tmp_buf), "/dev/pts/%d", pty->pt_number);
      if ((error = copyout(tmp_buf, user_buf, strlen(tmp_buf) + 1)))
        return error;
      return 0;
    }
    case TIOCSETAF:
    case TIOCSETAW:
      /* Convert TIOCSETAF and TIOCSETAW to TIOCSETA.
       * If we dont't do this, the process might deadlock waiting for all data
       * to be read from the master device. */
      cmd = TIOCSETA;
      break;
    case TIOCGPGRP: {
      /* Re-implement TIOCGPGRP, this time bypassing
       * the controlling terminal check. */
      int *pgid_p = data;

      SCOPED_MTX_LOCK(&tty->t_lock);

      if (tty->t_pgrp)
        *pgid_p = tty->t_pgrp->pg_id;
      else
        *pgid_p = INT_MAX;
      return 0;
    }
  }

  /* Redirect the call to the slave device. */
  return tty_ioctl(f, cmd, data);
}

static fileops_t pty_fileops = {.fo_read = pty_read,
                                .fo_write = pty_write,
                                .fo_close = pty_close,
                                .fo_seek = pty_seek,
                                .fo_stat = pty_stat,
                                .fo_ioctl = pty_ioctl};

static void pty_notify_out(tty_t *tty) {
  pty_t *pty = tty->t_data;
  /* Notify PTY readers: input is available. */
  cv_broadcast(&pty->pt_incv);
}

static void pty_notify_in(tty_t *tty) {
  pty_t *pty = tty->t_data;
  /* Notify PTY writers: there is space in the slave TTY's input buffer. */
  cv_broadcast(&pty->pt_outcv);
}

static void pty_notify_inactive(tty_t *tty) {
  pty_t *pty = tty->t_data;
  /* Notify PTY readers and writers so that they abort. */
  cv_broadcast(&pty->pt_incv);
  cv_broadcast(&pty->pt_outcv);
}

static ttyops_t pty_ttyops = {.t_notify_out = pty_notify_out,
                              .t_notify_in = pty_notify_in,
                              .t_notify_inactive = pty_notify_inactive};

int do_posix_openpt(proc_t *p, int flags, register_t *res) {
  int error, fd;

  if (flags & ~(O_RDWR | O_NOCTTY | O_CLOEXEC))
    return EINVAL;

  pty_t *pty = pty_alloc();
  if (!pty)
    return EAGAIN;

  tty_t *tty = tty_alloc();
  tty->t_ops = pty_ttyops;
  tty->t_data = pty;

  if (!(flags & O_NOCTTY)) {
    WITH_MTX_LOCK (&all_proc_mtx)
      WITH_MTX_LOCK (&tty->t_lock)
        maybe_assoc_ctty(p, tty);
  }

  file_t *f = file_alloc();
  f->f_data = tty;
  f->f_ops = &pty_fileops;
  f->f_type = FT_PTY;
  f->f_flags = FF_READ;
  if (flags & O_RDWR)
    f->f_flags |= FF_WRITE;

  char name_buf[4];
  snprintf(name_buf, 4, "%d", pty->pt_number);

  if ((error = tty_makedev(pts_dir, name_buf, tty)))
    goto err;
  if ((error = fdtab_install_file(p->p_fdtable, f, 0, &fd)))
    goto err;
  if ((error = fd_set_cloexec(p->p_fdtable, fd, (flags & O_CLOEXEC)))) {
    fdtab_close_fd(p->p_fdtable, fd);
    goto err;
  }
  *res = fd;
  return 0;

err:
  file_destroy(f);
  pty_free(pty);
  tty_free(tty);
  return error;
}

static void init_pty(void) {
  if (devfs_makedir(NULL, "pts", &pts_dir) != 0)
    panic("failed to create /dev/pts directory");
  /* Initialize pty_array. */
  for (int i = 0; i < MAX_PTYS; i++) {
    pty_t *pty = &pty_array[i];
    pty->pt_number = -1;
    cv_init(&pty->pt_incv, "pt_incv");
    cv_init(&pty->pt_outcv, "pt_outcv");
  }
}

SET_ENTRY(devfs_init, init_pty);
