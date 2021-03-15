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
 * euid   pid    ppid    pgrp   session  state    command
 * 0       1       0       1       1       R      /bin/ksh
 */

/* maximum size of output string of proc info
 * command + state (1 character)+ spaces (10 characters) +
 *  + 5 * max length of uint32 (10 characters)*/
#define MAX_P_STRING (PATH_MAX + 5 * 10 + 1 + 10)

/* maximum amount of processes that procstat can handle */
#define MAX_PROC 40

/* length of displayed command of process (includes terminating null byte) */
#define PROC_COMM_MAX 128

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
  proc_state_t proc_state;
  char *command;
} ps_entry_t;

typedef struct ps_buf {
  mtx_t lock;
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

static char *get_command(proc_t *p) {
  char *buf = kmalloc(M_TEMP, PROC_COMM_MAX, M_ZERO);

  /* copy program's name from elfpath (without starting slash) */
  char *start = strrchr(p->p_elfpath, '/') + 1;
  if (start == NULL)
    start = p->p_elfpath;

  ssize_t len = strlcpy(buf, start, PROC_COMM_MAX);
  len = min(len, PROC_COMM_MAX - 1);

  /* if there is enough space append space after program name */
  if (len + 1 < PROC_COMM_MAX)
    buf[len++] = ' ';

  strlcpy(buf + len, p->p_args, PROC_COMM_MAX - len);

  return buf;
}

static void ps_entry_fill(ps_entry_t *pe, proc_t *p) {
  SCOPED_MTX_LOCK(&p->p_lock);
  pe->uid = p->p_cred.cr_euid;
  pe->pid = p->p_pid;
  pe->ppid = p->p_parent->p_pid;
  pe->pgrp = p->p_pgrp->pg_id;
  pe->sid = p->p_pgrp->pg_session->s_sid;
  pe->proc_state = p->p_state;
  pe->command = get_command(p);
}

/* buf must be at least MAX_P_STRING long
 * returns length of written string
 */
static int ps_entry_tostring(char *buf, ps_entry_t *pe) {
  int r = snprintf(buf, MAX_P_STRING, "%d\t%d\t%d\t%d\t%d\t%c\t%s\n", pe->uid,
                   pe->pid, pe->ppid, pe->pgrp, pe->sid,
                   proc_state[pe->proc_state], pe->command);

  return min(r, MAX_P_STRING);
}

static int dev_procstat_open(vnode_t *v, int mode, file_t *fp) {
  WITH_MTX_LOCK (&procstat_lock) {
    if (ps_opencnt >= MAX_PROCSTAT)
      return EMFILE;
    ps_opencnt++;
    assert(ps_opencnt <= MAX_PROCSTAT);
  }

  int error = 0;
  proc_t *p;
  ps_buf_t *ps;

  if ((error = vnode_open_generic(v, mode, fp))) {
    WITH_MTX_LOCK (&procstat_lock) {
      ps_opencnt--;
      assert(ps_opencnt >= 0);
    }
    return error;
  }

  ps = kmalloc(M_TEMP, MAX_PROC * sizeof(ps_entry_t) + sizeof(ps_buf_t), 0);

  ps->nproc = 0;
  mtx_init(&ps->lock, 0);

  WITH_MTX_LOCK (&all_proc_mtx) {
    TAILQ_FOREACH (p, &proc_list, p_all) {
      if (p->p_pid == 0)
        continue; /* we don't want to show proc0 to user */
      if (ps->nproc >= MAX_PROC)
        goto out;
      ps_entry_fill(&ps->ps[ps->nproc++], p);
    }
    TAILQ_FOREACH (p, &zombie_list, p_zombie) {
      if (ps->nproc >= MAX_PROC)
        goto out;
      ps_entry_fill(&ps->ps[ps->nproc++], p);
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

  SCOPED_MTX_LOCK(&ps->lock);

  if (ps->cur_proc >= ps->nproc)
    return 0;

  uio->uio_offset = f->f_offset;
  while (uio->uio_resid > 0) {
    ssize_t len = min(uio->uio_resid, ps->psize - ps->offset);
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
    kfree(M_TEMP, ps->ps[i].command);
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
