#ifndef _PCPU_H_
#define _PCPU_H_

#include <thread.h>

typedef struct pcpu {
    thread_t* current_thread;
} pcpu_t;


extern pcpu_t _pcpu_data[1];

/* Read pcpu.h from FreeBSD for API reference */
#define PCPU_ADD(member, value) (_pcpu_data->member += (value))
#define PCPU_GET(member)        (_pcpu_data->member)
#define PCPU_INC(member)        PCPU_ADD(member, 1)
#define PCPU_PTR(member)        (&_pcpu_data->member)
#define PCPU_SET(member, value) (_pcpu_data->member = (value))
#define PCPU_LAZY_INC(member)   (++_pcpu_data->member)

void pcpu_init(void);

#endif /* _PCPU_H_ */