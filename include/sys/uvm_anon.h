#ifndef _SYS_UVM_ANON_H_
#define _SYS_UVM_ANON_H_

#ifndef _KERNEL
#error "Do not use this header file outside kernel code!"
#endif

#include <sys/mutex.h>
#include <sys/vm.h>

typedef struct uvm_anon uvm_anon_t;

/*
 * Field markings and the corresponding locks:
 *  (@) uvm_anon:an_lock
 */

struct uvm_anon {
  mtx_t an_lock;
  int an_ref;         /* (@) Reference counter */
  vm_page_t *an_page; /* (@) Pointer to holded page */
};

/* Allocate new anon */
uvm_anon_t *uvm_anon_alloc(void);
/* Acquire anon->an_lock */
void uvm_anon_lock(uvm_anon_t *anon);
/* Release anon->an_lock */
void uvm_anon_unlock(uvm_anon_t *anon);
/* Increase anon's reference counter */
void uvm_anon_hold(uvm_anon_t *anon);
/* Decrease anon's reference counter and destroy anon if it reached 0 */
void uvm_anon_drop(uvm_anon_t *anon);

#endif /* !_SYS_UVM_ANON_H_ */
