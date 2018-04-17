#ifndef _SYS_TIMER_H_
#define _SYS_TIMER_H_

#include <queue.h>
#include <time.h>

typedef struct timer timer_t;
typedef TAILQ_HEAD(, timer) timer_list_t;

typedef bintime_t (*tm_gettime_t)(timer_t *tm);
typedef int (*tm_start_t)(timer_t *tm, unsigned flags, const bintime_t value);
typedef int (*tm_stop_t)(timer_t *tm);

/*! \brief Callback function type.
 *
 * \warning It will be called in interrupt context! */
typedef void (*tm_event_cb_t)(timer_t *tm, void *arg);

#define TMF_ONESHOT 0x0001  /*!< triggers callback once */
#define TMF_PERIODIC 0x0002 /*!< triggers callback on regular basis */
#define TMF_TYPEMASK 0x0003 /*!< don't use other bits! */

typedef struct timer {
  TAILQ_ENTRY(timer) tm_link; /*!< entry on list of all timers */
  const char *tm_name;        /*!< name of the timer */
  unsigned tm_flags;          /*!< TMF_* flags */
  bintime_t tm_min_period;    /*!< valid only for TMF_PERIODIC */
  bintime_t tm_max_period;    /*!< same as above */
  tm_gettime_t tm_gettime;    /*!< returns current time value */
  tm_start_t tm_start;        /*!< makes timer operational */
  tm_stop_t tm_stop;          /*!< ceases timer from generating new events */
  tm_event_cb_t tm_event_cb;  /*!< callback called when timer triggers */
  void *tm_arg;               /*!< an argument for callback */
  void *tm_priv;              /*!< private data (usually device_t *) */
} timer_t;

/*! \brief Used by a driver to make timer device available to the system. */
int tm_register(timer_t *tm);
/*! \brief Called when unloading a timer driver. */
int tm_deregister(timer_t *tm);

/*! \brief Allocate a timer for exclusive use according to criteria.
 *
 * \arg name specifies name of the timer (can be NULL)
 * \arg flags specifies timer capabilities
 */
timer_t *tm_alloc(const char *name, unsigned flags);
/*! \brief Releases a timer if it's not needed anymore. */
int tm_free(timer_t *tm);

/*! \brief Prepares timer to call event trigger callback. */
int tm_init(timer_t *tm, tm_event_cb_t event, void *arg);
/*! \brief Reads current timer value. */
bintime_t tm_gettime(timer_t *tm);
/*! \brief Configures timer to trigger callback(s). */
int tm_start(timer_t *tm, unsigned flags, const bintime_t value);
/*! \brief Stops timer from triggering a callback. */
int tm_stop(timer_t *tm);
/*! \brief Used by interrupt filter routine to trigger a callback. */
void tm_trigger(timer_t *tm);

#endif /* !_SYS_TIMER_H_ */
