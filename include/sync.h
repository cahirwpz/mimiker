#ifndef __SYS_SYNC_H__
#define __SYS_SYNC_H__

/*
 * Entering critical section turns off interrupts - use with care!
 *
 * Thread must not yield or switch when inside critical section!
 *
 * Calls to @cs_enter can nest, you must use same number of @cs_leave to
 * actually leave a critical section.
 */
void cs_enter();
void cs_leave();

#endif
