#define KL_LOG KL_PROC
#include <klog.h>
#include <pool.h>
#include <proc.h>

static POOL_DEFINE(P_PGRP, "pgrp", sizeof(pgrp_t));
static LIST_HEAD(, pgrp) pgrphashtable[4];
static unsigned long pgrphash = 0x3;

#define PGRPHASH(pgid) pgrphashtable[(pgid) & pgrphash]

pgrp_t *pgrp_create(pgid_t pgid) {
  pgrp_t *pgrp = pool_alloc(P_PGRP, PF_ZERO);

  LIST_INIT(&pgrp->pg_members);
  pgrp->pg_id = pgid;
  // mtx_init(&pgrp->pg_mtx, ??);

  LIST_INSERT_HEAD(&PGRPHASH(pgid), pgrp, pg_hash);

  return pgrp;
}

/* Delete a process group. */
void pgrp_destroy(pgrp_t *pgrp) {
  assert(LIST_EMPTY(&pgrp->pg_members));

  LIST_REMOVE(pgrp, pg_hash);
  
  pool_free(P_PGRP, pgrp);
}

/* Locate a process group by number. */
pgrp_t *pgrp_find(pgid_t pgid) {
  pgrp_t *pgrp = NULL;
  LIST_FOREACH(pgrp, &PGRPHASH(pgid), pg_hash)
    if(pgrp->pg_id == pgid)
      break;

  return pgrp;
}
