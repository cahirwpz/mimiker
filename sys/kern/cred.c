#include <sys/cred.h>

void cred_init_root(cred_t *c) {
  c->cr_ruid = 0;
  c->cr_euid = 0;
  c->cr_suid = 0;
  c->cr_rgid = 0;
  c->cr_egid = 0;
  c->cr_sgid = 0;
  c->cr_ngroups = 0;
}
