#ifndef _SYS_KGPROF_H_
#define _SYS_KGPROF_H_

#if KGPROF
void init_kgprof(void);
#else
#define init_kgprof() __nothing
#endif

#endif /* !_SYS_KGPROF_H_ */