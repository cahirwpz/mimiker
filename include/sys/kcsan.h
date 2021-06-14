#ifndef _SYS_KCSAN_H_
#define _SYS_KCSAN_H_

/*
 * The Kernel Concurrency Sanitizer (KCSAN) is a dynamic race detector, which
 * relies on compile-time instrumentation, and uses a watchpoint-based sampling
 * approach to detect races.
 *
 * KCSAN finds potential problems by monitoring access to memory locations and
 * identifying patterns where
 *  - multiple threads of execution access the location,
 *  - those accesses are unordered - not protected by a lock, for example, and,
 *  - at least one of those accesses is a write.
 *
 * To enable, compile the kernel with KCSAN=1 flag.
 */

#if KCSAN

/*
 * The initializing function doesn't have to be called too early. Preferably
 * just before forking to the init process, since KCSAN is useless if only one
 * thread is running.
 */
void init_kcsan(void);
#else /* !KCSAN */
#define init_kcsan() __nothing
#endif

#endif /* !_SYS_KCSAN_H_ */
