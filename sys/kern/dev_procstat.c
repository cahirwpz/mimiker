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
 * euid   pid    ppid    pgrp   session  state    td_name  elfpath
 * 0       1       0       1       1       R       init    /bin/ksh
 */

/* maximum size of output string of proc info
 * name + path + state (1 character)+ spaces (10 characters) +
 *  + 5 * max length of uint32 (10 characters)*/
#define MAX_P_STRING (TD_NAME_MAX + PATH_MAX + 5 * 10 + 1 + 10)

/* maximum amount of processes that procstat can handle */
#define MAX_PROC 40

static char state[4] = {
  [PS_NORMAL] = 'R',
  [PS_STOPPED] = 'S',
  [PS_DYING] = 'D',
  [PS_ZOMBIE] = 'Z',
};

typedef struct proc_info {
  pid_t pid;
  pid_t ppid;
  uid_t uid;
  pgid_t pgrp;
  sid_t sid;
  proc_state_t state;
  char td_name[TD_NAME_MAX];
  char elfpath[PATH_MAX];
} proc_info_t;

typedef struct pstat {
  int nproc;
  int cur_proc;
  size_t offset; /* offset in buffer of current proc string */
  size_t psize;  /* size of current proc string */
  char buf[MAX_P_STRING];
  proc_info_t ps[];
} pstat_t;

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

/* must be called with proc_t::p_lock held */
static void fill_proc_info(proc_t *p, proc_info_t *pi) {
  pi->uid = p->p_cred.cr_euid;
  pi->pid = p->p_pid;
  pi->ppid = p->p_parent->p_pid;
  pi->pgrp = p->p_pgrp->pg_id;
  pi->sid = p->p_pgrp->pg_session->s_sid;
  pi->state = p->p_state;
  strncpy(pi->td_name, p->p_thread->td_name, TD_NAME_MAX);
  strncpy(pi->elfpath, p->p_elfpath, PATH_MAX);

  /* check if strings were copied properly */
  if (pi->td_name[TD_NAME_MAX - 1] != '\0')
    pi->td_name[TD_NAME_MAX - 1] = '\0';
  if (pi->elfpath[PATH_MAX - 1] != '\0')
    pi->elfpath[PATH_MAX - 1] = '\0';
}

/* buf must be at least MAX_P_STRING long
 * returns length of written string
 */
static int sprint_proc(char *buf, proc_info_t *pi) {
  int r = snprintf(buf, MAX_P_STRING, "%d\t%d\t%d\t%d\t%d\t%c\t%s\t%s\n",
                   pi->uid, pi->pid, pi->ppid, pi->pgrp, pi->sid,
                   state[pi->state], pi->td_name, pi->elfpath);

  return MIN(r, MAX_P_STRING);
}

static int dev_procstat_open(vnode_t *v, int mode, file_t *fp) {
  int error;
  if ((error = vnode_open_generic(v, mode, fp)))
    return error;

  proc_t *p;
  pstat_t *ps =
    kmalloc(M_TEMP, MAX_PROC * sizeof(proc_info_t) + sizeof(pstat_t), 0);
  if (ps == NULL)
    return ENOMEM;

  ps->nproc = 0;

  WITH_MTX_LOCK (all_proc_mtx) {
    TAILQ_FOREACH (p, &proc_list, p_all) {
      if (p->p_pid == 0)
        continue; /* we don't want to show proc0 to user */
      if (ps->nproc >= MAX_PROC)
        goto out;
      fill_proc_info(p, &ps->ps[ps->nproc++]);
    }
    TAILQ_FOREACH (p, &zombie_list, p_zombie) {
      if (ps->nproc >= MAX_PROC)
        goto out;
      fill_proc_info(p, &ps->ps[ps->nproc++]);
    }
  }
out:
  /* get first proc into buffer */
  ps->psize = sprint_proc(ps->buf, &ps->ps[0]);
  ps->offset = 0;
  ps->cur_proc = 0;

  fp->f_ops = &dev_procstat_fileops;
  fp->f_data = ps;
  return 0;
}

static int dev_procstat_read(file_t *f, uio_t *uio) {
  pstat_t *ps = f->f_data;
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
      ps->psize = sprint_proc(ps->buf, &ps->ps[ps->cur_proc]);
      ps->offset = 0;
    }
  }

  f->f_offset = uio->uio_offset;
  return error;
}

static int dev_procstat_close(vnode_t *v, file_t *fp) {
  kfree(M_TEMP, fp->f_data);
  return 0;
}

static void init_dev_procstat(void) {
  devfs_makedev(NULL, "procstat", &dev_procstat_vnodeops, NULL, NULL);
}

SET_ENTRY(devfs_init, init_dev_procstat);
