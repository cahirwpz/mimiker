#ifndef _SYS_SYSINIT_H_
#define _SYS_SYSINIT_H_

#include <linker_set.h>
#include <queue.h>

typedef struct sysinit_entry sysinit_entry_t;
typedef void sysinit_func_t();

typedef struct sysinit_entry {
  const char *name;
  sysinit_func_t *func;
  char **deps;
  uint32_t dependants;
  TAILQ_ENTRY(sysinit_entry) entries;
} sysinit_entry_t;

SET_DECLARE(sysinit, sysinit_entry_t);

#define SYSINIT_ADD(name, func, deps)                                          \
  sysinit_entry_t name##_sysinit = {#name, func, deps, 0};                     \
  SET_ENTRY(sysinit, name##_sysinit);
void sysinit_sort();
#endif /*!_SYS_SYSINIT_H_*/
