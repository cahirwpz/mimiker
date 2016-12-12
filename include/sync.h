#ifndef _SYS_SYNC_H_
#define _SYS_SYNC_H_

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

#endif /* !_SYS_SYNC_H_ */
