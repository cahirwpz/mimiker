#ifndef _SYS_TURNSTILE_H_
#define _SYS_TURNSTILE_H_

typedef struct turnstile turnstile_t;

void turnstile_init(void);

turnstile_t *turnstile_alloc(void);

void turnstile_destroy(turnstile_t *ts);

#endif /* !_SYS_TURNSTILE_H_ */
