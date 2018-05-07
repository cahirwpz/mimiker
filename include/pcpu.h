#ifndef _PCPU_H_
#define _PCPU_H_

typedef struct thread thread_t;
typedef struct pmap pmap_t;
typedef struct vm_map vm_map_t;

/*! \brief Private per-cpu structure. */
typedef struct pcpu {
  thread_t *curthread;   /*!< thread running on this CPU */
  thread_t *idle_thread; /*!< idle thread executed on this CPU */
  pmap_t *curpmap;       /*!< current page table */
  vm_map_t *uspace;      /*!< user space virtual memory map */
} pcpu_t;

extern pcpu_t _pcpu_data[1];

/* Read pcpu.h from FreeBSD for API reference */
#define PCPU_GET(member) (_pcpu_data->member)
#define PCPU_PTR(member) (&_pcpu_data->member)
#define PCPU_SET(member, value) (_pcpu_data->member = (value))

void pcpu_init(void);

#endif /* _PCPU_H_ */
