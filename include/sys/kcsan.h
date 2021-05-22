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
 * When KCSAN is enabled, the code is instrumented to allow the monitoring of
 * memory accesses. Specifically, each memory access will be augmented by a
 * function call. The compiler distinguishes different memory accesses depending
 * on the type of the variable: plain, volatile or atomic. In our implementation
 * last two are discarded, because they don't meet the conditions of the
 * previously defined data race.
 *
 * On every memory access KCSAN, basically do two things: checks for the already
 * existing data race on that address and occasionally sets up a watchpoint. The
 * second part is skipped (SKIP_COUNT - 1) out of SKIP_COUNT times; to do
 * otherwise would slow the kernel to a point of complete unusability. On the
 * SKIP_COUNT th time, though, KCSAN keeps an eye on the address for a period of
 * time, looking for other accesses. While running in the context of the thread
 * where the access was performed, KCSAN will set a "watchpoint", which is done
 * by recording the address, the size of the data access, and whether the access
 * was a write in a small table. This thread will then simply delay for a set,
 * fixed amount of time.
 *
 * Note that, before deciding whether to ignore an access, KCSAN looks to see if
 * there is already a watchpoint established for the address. If so, and if
 * either the current access or the access that created the watchpoint is a
 * write, then a race condition has been detected and a report will be sent to
 * the system log.
 *
 * Meanwhile, the original thread is delaying after having set the watchpoint.
 * At the end of the delay period, the watchpoint will be deleted and monitoring
 * of that address stops. But before execution continues, the value at the
 * accessed address will be checked; if it has changed since the watchpoint was
 * set, a race condition is once again deemed to have occurred.
 *
 * Currently, KCSAN is only implemented for the MIPS architecture, although GCC
 * supports instrumenting other architectures as well.
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
