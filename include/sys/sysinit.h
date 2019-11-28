#ifndef _SYS_SYSINIT_H_
#define _SYS_SYSINIT_H_

#include <sys/linker_set.h>
#include <sys/queue.h>

#define ARRAY_INIT(...)                                                        \
  { __VA_ARGS__ }
#define DEPS(...) (char *[]) ARRAY_INIT(__VA_ARGS__, NULL)
#define NODEPS                                                                 \
  (char *[]) {                                                                 \
    NULL                                                                       \
  }

typedef struct sysinit_entry sysinit_entry_t;
typedef void sysinit_func_t(void);

struct sysinit_entry {
  const char *name;     /* name of module */
  sysinit_func_t *func; /* init function */
  char **deps;          /* names of dependencies */
  unsigned dependants;  /* number of modules depending on this module
                           used during topological ordering */
  TAILQ_ENTRY(sysinit_entry) entries; /* linked list reperesenting ordering */
};

SET_DECLARE(sysinit, sysinit_entry_t);

#define SYSINIT_ADD(mod_name, init_func, deps_names)                           \
  sysinit_entry_t mod_name##_sysinit = {.name = (#mod_name),                   \
                                        .func = (init_func),                   \
                                        .deps = (deps_names),                  \
                                        .dependants = 0};                      \
  SET_ENTRY(sysinit, mod_name##_sysinit);

void sysinit(void);

#endif /* !_SYS_SYSINIT_H_ */
