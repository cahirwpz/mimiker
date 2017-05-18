#ifndef _SYS_SYNC_H_
#define _SYS_SYNC_H_

#include <common.h>

/*
 * Entering critical section turns off interrupts - use with care!
 *
 * In most scenarios threads should not yield or switch when inside
 * critical section. However it should behave correctly, since
 * context switch routine restores interrupts state of target thread.
 *
 * Calls to @critical_enter can nest, you must use same number of
 * @critical_leave to actually leave the critical section.
 */
void critical_enter();
void critical_leave();

unsigned _critical_enter();

#define critical_scoped()                                                      \
  unsigned __CONCAT(__cs_, __LINE__) __cleanup(critical_leave) =               \
    _critical_enter()

#define IN_CRITICAL_SECTION()                                                  \
  for (critical_scoped(), __CONCAT(__loop_, __LINE__) = 1;                     \
       __CONCAT(__loop_, __LINE__); __CONCAT(__loop_, __LINE__) = 0)

#endif /* !_SYS_SYNC_H_ */
