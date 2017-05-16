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
  sysinit_entry_t **ptr;
  SET_FOREACH(ptr, sysinit) {
    if (strcmp(name, (*ptr)->name) == 0)
      return *ptr;
  }
  return NULL;
}
static void push_last(sysinit_tailq_t *head) {
  sysinit_entry_t **ptr;
  SET_FOREACH(ptr, sysinit) {
    if ((*ptr)->dependants == 0)
      TAILQ_INSERT_HEAD(head, *ptr, entries);
  }
}
static void build_queue(sysinit_tailq_t *head) {
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
void sysinit_sort() {
  count_dependants();
  sysinit_tailq_t list = TAILQ_HEAD_INITIALIZER(list);
  build_queue(&list);
  sysinit_entry_t *p;
  TAILQ_FOREACH (p, &list, entries) {
    klog("launching module: %s", p->name);
    if (p->func)
      p->func();
  }
}
