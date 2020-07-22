#include <sys/mimiker.h>
#include <aarch64/exception.h>

void exc_frame_init(exc_frame_t *frame, void *pc, void *sp, unsigned flags) {
  panic("Not implemented!");
}

void exc_frame_copy(exc_frame_t *to, exc_frame_t *from) {
  panic("Not implemented!");
}

void exc_frame_setup_call(exc_frame_t *frame, register_t retaddr,
                          register_t arg) {
  panic("Not implemented!");
}

void exc_frame_set_retval(exc_frame_t *frame, register_t value,
                          register_t error) {
  panic("Not implemented!");
}

void user_exc_leave(void) {
  panic("Not implemented!");
}
