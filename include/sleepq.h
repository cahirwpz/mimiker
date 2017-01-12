#ifndef _SYS_SLEEPQ_H_
#define _SYS_SLEEPQ_H_

#include <common.h>
#include <queue.h>

typedef struct thread thread_t;
typedef struct sleepq sleepq_t;

void sleepq_init();
sleepq_t *sleepq_alloc();
void sleepq_destroy(sleepq_t *sq);

/*
 * Lookup the sleep queue associated with a given wait channel.
 * If no queue is found, NULL is returned.
 */
sleepq_t *sleepq_lookup(void *wchan);

/*
 * Block the current thread until it is awakened from its sleep queue.
 */
void sleepq_wait(void *wchan, const char *wmesg);

/*
 * Find the highest priority thread sleeping on a wait channel and resume it.
 */
bool sleepq_signal(void *wchan);

/*
 * Resume all threads sleeping on a specified wait channel.
 */
bool sleepq_broadcast(void *wchan);

/*
 * Resumes a specific thread from the sleep queue associated with a specific
 * wait channel if it is on that queue.
 */
void sleepq_remove(thread_t *td, void *wchan);

#endif /* !_SYS_SLEEPQ_H_ */
