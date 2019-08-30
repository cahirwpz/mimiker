#ifndef _PTHREAD_H_
#define _PTHREAD_H_

typedef struct __pthread_once_st pthread_once_t;

struct __pthread_once_st {
  int pto_done;
};

#endif /* !_PTHREAD_H_ */
