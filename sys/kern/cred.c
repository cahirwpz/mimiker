#include <sys/cred.h>
#include <sys/libkern.h>

void cred_init_root(cred_t *c) {
  c->cr_ruid = 0;
  c->cr_euid = 0;
  c->cr_suid = 0;
  c->cr_rgid = 0;
  c->cr_egid = 0;
  c->cr_sgid = 0;
  c->cr_ngroups = 0;
}

void cred_copy(cred_t *src, cred_t *dest) {
  memcpy(dest, src, sizeof(cred_t));
}
