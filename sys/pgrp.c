#define KL_LOG KL_PROC
#include <klog.h>
#include <pool.h>
#include <proc.h>

static POOL_DEFINE(P_PGRP, "pgrp", sizeof(pgrp_t));

pgrp_t *pgrp_create(pgid_t pgid) {
  pgrp_t *pgrp = pool_alloc(P_PGRP, PF_ZERO);
  /* do sth to pgrp */
  return pgrp;
}

/* Delete a process group. */
void pgrp_destroy(pgrp_t *pgrp) {
  /* do sth to pgrp */

  pool_free(P_PGRP, pgrp);

#if 0
  assert(mtx_owned(all_proc_mtx));
  assert(!mtx_owned(pgrp->pg_mtx));

  WITH_MTX_LOCK (&pgrp->pg_mtx)
    LIST_REMOVE(pgrp, pg_hash);

  mtx_destroy(&pgrp->pg_mtx);
  kfree(pgrp, M_PGRP);
#endif
}

// http://mimiker.ii.uni.wroc.pl/source/xref/FreeBSD/head/sys/kern/kern_proc.c#448
/* Locate a process group by number. The caller must hold proctree_lock. */
pgrp_t *pgrp_find(pgid_t pgid) {
#if 0
  pgrp_t *pgrp;

  assert(mtx_owned(all_proc_mtx));

  LIST_FOREACH(pgrp, PGRPHASH(pgid), pg_hash) {
    if (pgrp->pg_id == pgid) {
      mutex_lock(pgrp->pg_mtx);
      return pgrp;
    }
  }
#endif
  return NULL;
}

#if 0
/* Move p to an existing process group. */
int enterthispgrp(proc_t *p, pgrp_t *pgrp) {
  assert(mtx_owned(all_proc_mtx));
  aassert(mtx_owned(p->p_lock) == false);
  assert(mtx_owned(pgrp->pg_mtx) == false);
  assert(mtx_owned(p->p_pgrp->pg_mtx) == false);
  assert(pgrp != p->p_pgrp);

  doenterpgrp(p, pgrp);

  return 0;
}

/* Move p to a process group. */
int doenterpgrp(proc_t *p, pgrp_t *pgrp) {
  pgrp_t *savepgrp;

  assert(mtx_owned(all_proc_mtx));
  assert(mtx_owned(p->p_lock) == false);
  assert(mtx_owned(pgrp->pg_mtx) == false);
  assert(mtx_owned(p->p_pgrp->pg_mtx) == false);

  savepgrp = p->p_pgrp;

  mutex_lock(pgrp->pg_mtx);
  mutex_lock(savepgrp->pg_mtx);
  proc_lock(p);
  LIST_REMOVE(p, p_pglist);
  p->p_pgrp = pgrp;
  proc_unlock(p);
  LIST_INSERT_HEAD(&pgrp->pg_members, p, p_pglist);
  mutex_unlock(savepgrp->pg_mtx);
  mutex_unlock(pgrp->pg_mtx);
  if (LIST_EMPTY(&savepgrp->pg_members))
    pgdelete(savepgrp);
  return 0;
}

int leavepgrp(proc_t *p) {
  pgrp_t *savepgrp;

  assert(mtx_owned(all_proc_mtx));
  savepgrp = p->p_pgrp;
  mutex_lock(savepgrp->pg_mtx);
  proc_lock(p);
  LIST_REMOVE(p, p_pglist);
  p->p_pgrp = NULL;
  proc_unlock(p);
  mutex_unlock(savepgrp->pg_mtx);
  if (LIST_EMPTY(&savepgrp->pg_members))
    pgdelete(savepgrp);
  return 0;
}
#endif
