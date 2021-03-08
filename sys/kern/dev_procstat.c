#include <sys/devfs.h>
#include <sys/vnode.h>
#include <sys/linker_set.h>
#include <sys/queue.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <sys/malloc.h>
#include <stdio.h>

/* Implementation of /dev/procstat
 *
 * When /dev/procstat is opened it takes a snapshot of info of all existing
 * processes.
 *
 * During read calls to /dev/procstat this info can be read.
 *
 * Example:
 * euid   pid    ppid    pgrp   session  state    elfpath
 * 0       1       0       1       1       R      /bin/ksh
 */

/* maximum size of output string of proc info
 * name + path + state (1 character)+ spaces (10 characters) +
 *  + 5 * max length of uint32 (10 characters)*/
#define MAX_P_STRING (PATH_MAX + 5 * 10 + 1 + 10)

/* maximum amount of processes that procstat can handle */
#define MAX_PROC 40

/* we want to have at most 10 instances of procstat */
#define MAX_PROCSTAT 10

static char proc_state[4] = {
  [PS_NORMAL] = 'R',
  [PS_STOPPED] = 'S',
  [PS_DYING] = 'D',
  [PS_ZOMBIE] = 'Z',
};

typedef struct ps_entry {
  pid_t pid;
  pid_t ppid;
  uid_t uid;
  pgid_t pgrp;
  sid_t sid;
  proc_state_t state;
  char *elfpath;
} ps_entry_t;

typedef struct ps_buf {
  int nproc;
  int cur_proc;
  size_t offset; /* offset in buffer of current proc string */
  size_t psize;  /* size of current proc string */
  char buf[MAX_P_STRING];
  ps_entry_t ps[];
} ps_buf_t;

static int ps_opencnt = 0;
static mtx_t procstat_lock;

static int dev_procstat_read(file_t *f, uio_t *uio);
static int dev_procstat_close(vnode_t *v, file_t *fp);
static int dev_procstat_open(vnode_t *v, int mode, file_t *fp);

static fileops_t dev_procstat_fileops = {
  .fo_read = dev_procstat_read,
  .fo_close = default_vnclose,
  .fo_stat = default_vnstat,
  .fo_ioctl = default_vnioctl,
};

static vnodeops_t dev_procstat_vnodeops = {
  .v_open = dev_procstat_open,
  .v_close = dev_procstat_close,
};

static void ps_entry_fill(proc_t *p, ps_entry_t *pi) {
  SCOPED_MTX_LOCK(&p->p_lock);
  pi->uid = p->p_cred.cr_euid;
  pi->pid = p->p_pid;
  pi->ppid = p->p_parent->p_pid;
  pi->pgrp = p->p_pgrp->pg_id;
  pi->sid = p->p_pgrp->pg_session->s_sid;
  pi->proc_state = p->p_state;
  pi->elfpath = kstrndup(M_TEMP, p->p_elfpath, PATH_MAX);
}

/* buf must be at least MAX_P_STRING long
 * returns length of written string
 */
static int ps_entry_tostring(char *buf, ps_entry_t *pi) {
  int r = snprintf(buf, MAX_P_STRING, "%d\t%d\t%d\t%d\t%d\t%c\t%s\n", pi->uid,
                   pi->pid, pi->ppid, pi->pgrp, pi->sid,
                   proc_state[pi->proc_state], pi->elfpath);

  return MIN(r, MAX_P_STRING);
}

static int dev_procstat_open(vnode_t *v, int mode, file_t *fp) {
  WITH_MTX_LOCK (&procstat_lock) {
    if (ps_opencnt >= MAX_PROCSTAT)
      return EMFILE;
    ps_opencnt++;
    assert(ps_opencnt <= MAX_PROCSTAT);
  }

  int error;
  if ((error = vnode_open_generic(v, mode, fp)))
    return error;

  proc_t *p;
  ps_buf_t *ps =
    kmalloc(M_TEMP, MAX_PROC * sizeof(ps_entry_t) + sizeof(ps_buf_t), 0);
  if (ps == NULL)
    return ENOMEM;

  ps->nproc = 0;

  WITH_MTX_LOCK (all_proc_mtx) {
    TAILQ_FOREACH (p, &proc_list, p_all) {
      if (p->p_pid == 0)
        continue; /* we don't want to show proc0 to user */
      if (ps->nproc >= MAX_PROC)
        goto out;
      ps_entry_fill(p, &ps->ps[ps->nproc++]);
    }
    TAILQ_FOREACH (p, &zombie_list, p_zombie) {
      if (ps->nproc >= MAX_PROC)
        goto out;
      ps_entry_fill(p, &ps->ps[ps->nproc++]);
    }
  }
out:
  /* get first proc into buffer */
  ps->psize = ps_entry_tostring(ps->buf, &ps->ps[0]);
  ps->offset = 0;
  ps->cur_proc = 0;

  fp->f_ops = &dev_procstat_fileops;
  fp->f_data = ps;
  return 0;
}

static int dev_procstat_read(file_t *f, uio_t *uio) {
  ps_buf_t *ps = f->f_data;
  int error = 0;

  if (ps->cur_proc >= ps->nproc)
    return 0;

  uio->uio_offset = f->f_offset;
  while (uio->uio_resid > 0) {
    size_t len = MIN(uio->uio_resid, ps->psize - ps->offset);
    if ((error = uiomove(ps->buf, len, uio)))
      break;

    ps->offset += len;

    /* have to jump to next process info string */
    if (ps->offset >= ps->psize) {
      if (++ps->cur_proc >= ps->nproc)
        break;
      ps->psize = ps_entry_tostring(ps->buf, &ps->ps[ps->cur_proc]);
      ps->offset = 0;
    }
  }

  f->f_offset = uio->uio_offset;
  return error;
}

static int dev_procstat_close(vnode_t *v, file_t *fp) {
  ps_buf_t *ps = fp->f_data;
  for (int i = 0; i < ps->nproc; ++i) {
    kfree(M_TEMP, ps->ps[i].elfpath);
  }
  kfree(M_TEMP, ps);

  WITH_MTX_LOCK (&procstat_lock) {
    ps_opencnt--;
    assert(ps_opencnt >= 0);
  }
  return 0;
}

static void init_dev_procstat(void) {
  mtx_init(&procstat_lock, 0);
  devfs_makedev(NULL, "procstat", &dev_procstat_vnodeops, NULL, NULL);
}

SET_ENTRY(devfs_init, init_dev_procstat);
