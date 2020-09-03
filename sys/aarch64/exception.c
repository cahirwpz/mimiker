#include <sys/mimiker.h>
#include <aarch64/exception.h>
#include <aarch64/armreg.h>
#include <sys/libkern.h>
#include <sys/exception.h>

void exc_frame_init(exc_frame_t *frame, void *pc, void *sp, unsigned flags) {
  /* TODO(pj) implement usermode */
  assert((flags & EF_USER) == 0);

  bzero(frame, sizeof(exc_frame_t));

  frame->pc = (register_t)pc;
  frame->sp = (register_t)sp;
  
  /* We don't handle fast interrupts in kernel. */
  frame->spsr = PSR_F | PSR_M_EL1h; 
}

void exc_frame_copy(exc_frame_t *to, exc_frame_t *from) {
  memcpy(to, from, sizeof(exc_frame_t));
}

void exc_frame_setup_call(exc_frame_t *frame, register_t retaddr,
                          register_t arg) {
  frame->lr = retaddr;
  frame->x[0] = arg;
}

void exc_frame_set_retval(exc_frame_t *frame, register_t value,
                          register_t error) {
  frame->x[0] = value;
  frame->x[1] = error;
  frame->pc += 4;
}

void kern_exc_leave(void) {
  panic("Not implemented!");
}

void user_exc_leave(void) {
  panic("Not implemented!");
}
