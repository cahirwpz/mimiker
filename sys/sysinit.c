#include <sysinit.h>
#include <stdc.h>
#define KL_LOG KL_INIT
#include <klog.h>

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
void sysinit_sort() {

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

  TAILQ_HEAD(tailhead, sysinit_entry) head = TAILQ_HEAD_INITIALIZER(head);
  TAILQ_INIT(&head);
  SET_FOREACH(ptr, sysinit) {
    if ((*ptr)->dependants == 0)
      TAILQ_INSERT_HEAD(&head, *ptr, entries);
  }
  sysinit_entry_t *p;
  TAILQ_FOREACH_REVERSE(p, &head, tailhead, entries) {
    char **deps = p->deps;
    while (*deps) {
      sysinit_entry_t *dependency = find(*deps);
      dependency->dependants--;
      if (dependency->dependants == 0)
        TAILQ_INSERT_HEAD(&head, dependency, entries);
      deps++;
    }
  }
  TAILQ_FOREACH (p, &head, entries) {
    klog("launching module: %s", p->name);
    if (p->func)
      p->func();
  }
}
