#ifndef _SYS_PROF_H_
#define _SYS_PROF_H_

#if KPROF
void init_prof(void);
#else
#define init_prof() __nothing
#endif

#endif /* !_SYS_PROF_H_ */