#include <sys/cred.h>
#include <sys/proc.h>
#include <sys/libkern.h>
#include <sys/mutex.h>

void cred_fork(proc_t *to, proc_t *from) {
  assert(mtx_owned(&from->p_lock));
  memcpy(&to->p_cred, &from->p_cred, sizeof(cred_t));
}
