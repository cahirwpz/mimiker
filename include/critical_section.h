#ifndef __CRITICAL_SECTION_H__
#define __CRITICAL_SECTION_H__

extern volatile int cs_level;

void cs_enter();

void cs_leave();

#endif /* __CRITICAL_SECTION_H__ */
