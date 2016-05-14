#ifndef __SLEEPQ_H__
#define __SLEEPQ_H__

#include <common.h>

typedef struct thread thread_t;

typedef struct sleepq {
  TAILQ_HEAD(, thread) sq_blocked;  /* Blocked threads. */
  uint32_t sq_nblocked;             /* Number of blocked threads. */
  void *sq_wchan;                   /* Wait channel. */
} sleepq_t;

typedef struct sleepqueue_chain {
  LIST_HEAD(, sleepqueue) sc_queues;  /* List of sleep queues. */
} sleepq_chain_t;

void sleepq_init();

/*
 * Lookup the sleep queue associated with a given wait channel.
 * If no queue is found, NULL is returned.
 */
sleepq_t *sleepq_lookup(void *wchan);

/*
 * Places the current thread on the sleep queue for the specified wait channel.
 */
void sleepq_add(void *wchan, const char *wmesg);

/*
 * Block the current thread until it is awakened from its sleep queue.
 */
void sleepq_wait(void *wchan);

/*
 * Find the highest priority thread sleeping on a wait channel and resume it.
 */
void sleepq_signal(void *wchan);

/*
 * Resume all threads sleeping on a specified wait channel. 
 */
void sleepq_broadcast(void *wchan);

/*
 * Resumes a specific thread from the sleep queue associated with a specific
 * wait channel if it is on that queue.
 */
void sleepq_remove(thread_t *td, void *wchan);

#endif
