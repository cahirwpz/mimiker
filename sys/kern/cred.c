#include <sys/cred.h>
#include <sys/proc.h>
#include <sys/libkern.h>

void cred_fork(proc_t *to, proc_t *from) {
  memcpy(&to->p_cred, &from->p_cred, sizeof(cred_t));
}
