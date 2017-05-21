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

#define SCOPED_CRITICAL_SECTION()                                              \
  SCOPED_STMT(void, critical_enter, critical_leave, NULL)

#define CRITICAL_SECTION WITH_STMT(void, critical_enter, critical_leave, NULL)

#endif /* !_SYS_SYNC_H_ */
