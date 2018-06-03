#ifndef _SYS_SLEEPQ_H_
#define _SYS_SLEEPQ_H_

#include <common.h>
#include <queue.h>

#define SLPF_INT 0x01

typedef uint8_t sleep_flags_t;

typedef struct thread thread_t;
typedef struct sleepq sleepq_t;

typedef enum {
  SLP_WKP_REG, /* regular wakeup */
  SLP_WKP_INT  /* thread interrupted */
} slp_wakeup_t;

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
void sleepq_wait(void *wchan, const void *waitpt, sleep_flags_t flags);

/*! \brief Wakes up highest priority thread waiting on \a wchan.
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
