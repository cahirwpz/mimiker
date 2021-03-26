#ifndef _SYS_KGPROF_H_
#define _SYS_KGPROF_H_

#if KGPROF
void init_kgprof(void);
void kgprof_tick(void);
#else
#define init_kgprof() __nothing
#define kgprof_tick() __nothing
#endif

#endif /* !_SYS_KGPROF_H_ */
