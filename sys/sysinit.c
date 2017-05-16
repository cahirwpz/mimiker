#include <sysinit.h>
#include <stdc.h>
#define KL_LOG KL_INIT
#include <klog.h>

typedef TAILQ_HEAD(tailhead, sysinit_entry) sysinit_tailq_t;
static void dummy() {
}
SYSINIT_ADD(first, dummy, DEPS(NULL));
SYSINIT_ADD(mid1, dummy, DEPS("first", NULL));
SYSINIT_ADD(mid2, dummy, DEPS("first", NULL));
SYSINIT_ADD(last, dummy, DEPS("mid1", "mid2", NULL));

static sysinit_entry_t *find(const char *name) {
  // locates sysinit_entry_t with given name
  sysinit_entry_t **ptr;
  SET_FOREACH(ptr, sysinit) {
    if (strcmp(name, (*ptr)->name) == 0)
      return *ptr;
  }
  return NULL;
}
static void push_last(sysinit_tailq_t *head) {
  // dependants field for every sysinit_entry_t need to be calculated
  sysinit_entry_t **ptr;
  SET_FOREACH(ptr, sysinit) {
    if ((*ptr)->dependants == 0)
      TAILQ_INSERT_HEAD(head, *ptr, entries);
  }
}
static void build_queue(sysinit_tailq_t *head) {
  // dependants field for every sysinit_entry_t need to be calculated
  push_last(head);

  sysinit_entry_t *p;
  TAILQ_FOREACH_REVERSE(p, head, tailhead, entries) {
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
static void count_dependants() {
  // requires dependants field to be zeroed
  sysinit_entry_t **ptr;
  SET_FOREACH(ptr, sysinit) {
    klog("found module: %s", (*ptr)->name);
    char **deps = (*ptr)->deps;
    while (*deps) {
      sysinit_entry_t *dependency = find(*deps);
      assert(dependency != NULL);
      dependency->dependants++;
      deps++;
    }
  }
}
static void dump_cycle() {
  // if there's some cycle prints its content, and panics
  // works properly only after build_queue
  sysinit_entry_t **ptr;
  bool cycle_found = false;
  SET_FOREACH(ptr, sysinit) {
    if ((*ptr)->dependants != 0) {
      klog("element of some cycle: %s", (*ptr)->name);
      cycle_found=true;
    }
  }
  if(cycle_found)
    panic("Found cycle in modules dependencies");
}
void sysinit_sort() {
  // builds topological ordering and lauches modules with built order
  // result is constructed from back, because of direction of relations
  // given edges point to earlier modules
  count_dependants();
  sysinit_tailq_t list = TAILQ_HEAD_INITIALIZER(list);
  build_queue(&list);
  dump_cycle();
  sysinit_entry_t *p;
  TAILQ_FOREACH (p, &list, entries) {
    klog("launching module: %s", p->name);
    if (p->func)
      p->func();
  }
}
