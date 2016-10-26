#ifndef _PCPU_H_
#define _PCPU_H_

#include <thread.h>

typedef struct pcpu {
    thread_t *curthread;
} pcpu_t;

extern pcpu_t _pcpu_data[1];

/* Read pcpu.h from FreeBSD for API reference */
#define PCPU_GET(member)        (_pcpu_data->member)
#define PCPU_PTR(member)        (&_pcpu_data->member)
#define PCPU_SET(member, value) (_pcpu_data->member = (value))

void pcpu_init(void);

#endif /* _PCPU_H_ */
