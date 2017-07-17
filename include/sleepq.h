#ifndef _SYS_SLEEPQ_H_
#define _SYS_SLEEPQ_H_

#include <common.h>
#include <queue.h>

typedef struct thread thread_t;
typedef struct sleepq sleepq_t;

/*! \file sleepq.h */

/*! \brief Initializes sleep queues.
 *
 * \warning To be called only from early kernel initialization! */
void sleepq_init(void);

/*! \brief Allocates sleep queue entry. */
sleepq_t *sleepq_alloc(void);

/*! \brief Deallocates sleep queue entry. */
void sleepq_destroy(sleepq_t *sq);

/*! \brief Blocks the current thread until it is awakened from its sleep queue.
 *
 * \param wchan unique sleep queue identifier
 * \param waitpt caller associated with sleep action
 */
void sleepq_wait(void *wchan, const void *waitpt);

/*! \brief Take first thread sleeping on \a wchan and resume it.
 *
 * \param wchan unique sleep queue identifier
 */
bool sleepq_signal(void *wchan);

/*! \brief Resume all threads sleeping on \a wchan.
 *
 * \param wchan unique sleep queue identifier
 */
bool sleepq_broadcast(void *wchan);

#endif /* !_SYS_SLEEPQ_H_ */
