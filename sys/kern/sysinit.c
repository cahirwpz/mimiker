#include <sys/sysinit.h>
#include <sys/mimiker.h>
#include <sys/libkern.h>
#define KL_LOG KL_INIT
#include <sys/klog.h>

typedef TAILQ_HEAD(tailhead, sysinit_entry) sysinit_tailq_t;

static sysinit_entry_t *find(const char *name) {
  /* locates sysinit_entry_t with given name */
  sysinit_entry_t **ptr;
  SET_FOREACH (ptr, sysinit) {
    if (strcmp(name, (*ptr)->name) == 0)
      return *ptr;
  }
  return NULL;
}

static void push_last(sysinit_tailq_t *head) {
  /* dependants field for every sysinit_entry_t need to be calculated */
  sysinit_entry_t **ptr;
  SET_FOREACH (ptr, sysinit) {
    if ((*ptr)->dependants == 0)
      TAILQ_INSERT_HEAD(head, *ptr, entries);
  }
}

static void build_queue(sysinit_tailq_t *head) {
  /* dependants field for every sysinit_entry_t need to be calculated */
  push_last(head);

  sysinit_entry_t *p;
  TAILQ_FOREACH_REVERSE (p, head, tailhead, entries) {
    char **deps = p->deps;
    while (*deps) {
      sysinit_entry_t *dependency = find(*deps);
      dependency->dependants--;
      if (dependency->dependants == 0)
        TAILQ_INSERT_HEAD(head, dependency, entries);
      deps++;
    }
  }
}

static void count_dependants(void) {
  /* requires dependants field to be zeroed */
  sysinit_entry_t **ptr;
  SET_FOREACH (ptr, sysinit) {
    klog("found module '%s'", (*ptr)->name);
    char **deps = (*ptr)->deps;
    while (*deps) {
      sysinit_entry_t *dependency = find(*deps);
      if (dependency == NULL)
        panic("module '%s' depends on '%s', which has not been found",
              (*ptr)->name, *deps);
      dependency->dependants++;
      deps++;
    }
  }
}

static void dump_cycle(void) {
  /* if there's some cycle prints its content, and panics
     works properly only after build_queue */
  sysinit_entry_t **ptr;
  bool cycle_found = false;
  SET_FOREACH (ptr, sysinit) {
    if ((*ptr)->dependants != 0) {
      klog("unreachable module '%s'", (*ptr)->name);
      cycle_found = true;
    }
  }
  if (cycle_found)
    panic("found cycle in modules dependencies");
}

void sysinit(void) {
  /* builds topological ordering and lauches modules with built order
   result is constructed from back, because of direction of relations
   given edges point to earlier modules */
  count_dependants();
  sysinit_tailq_t list = TAILQ_HEAD_INITIALIZER(list);
  build_queue(&list);
  dump_cycle();
  sysinit_entry_t *p;
  TAILQ_FOREACH (p, &list, entries) {
    if (p->func) {
      klog("initialize '%s' module", p->name);
      p->func();
    }
  }
}
