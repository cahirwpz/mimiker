#ifndef _SYS_SYSINIT_H_
#define _SYS_SYSINIT_H_

#include <linker_set.h>
#include <queue.h>
#define ARRAY_INIT(...)                                                        \
  { __VA_ARGS__ }
#define DEPS(...) (char *[]) ARRAY_INIT(__VA_ARGS__, NULL)
#define NODEPS                                                                 \
  (char *[]) {                                                                 \
    NULL                                                                       \
  }

typedef struct sysinit_entry sysinit_entry_t;
typedef void sysinit_func_t();

struct sysinit_entry {
  const char *name;     /* name of module */
  sysinit_func_t *func; /* init function */
  char **deps;          /* names of dependencies */
  unsigned dependants;  /* number of modules depending on this module
                           used during topological ordering */
  TAILQ_ENTRY(sysinit_entry) entries; /* linked list reperesenting ordering */

};

SET_DECLARE(sysinit, sysinit_entry_t);

#define SYSINIT_ADD(name, func, deps)                                          \
  sysinit_entry_t name##_sysinit = {#name, func, deps, 0};                     \
  SET_ENTRY(sysinit, name##_sysinit);
void sysinit_sort();
#endif /*!_SYS_SYSINIT_H_*/
