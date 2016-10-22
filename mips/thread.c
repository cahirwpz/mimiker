#include <mips/mips.h>
#include <thread.h>

thread_t* _current_thread = NULL;

thread_t *thread_self() {
  return _current_thread;
}
