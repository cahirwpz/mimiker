#include <sysinit.h>
#include <stdc.h>
static void dummy() {
}
char *first[] = {NULL};
char *mid[] = {"first", NULL};
char *last[] = {"mid1","mid2", NULL};
SYSINIT_ADD(first, dummy, first);
SYSINIT_ADD(mid1, dummy, mid);
SYSINIT_ADD(mid2, dummy, mid);
SYSINIT_ADD(last, dummy, last);

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
    kprintf("[sysinit] found: %s\n", (*ptr)->name);
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
    kprintf("[sysinit] launch: %s\n", p->name);
    p->func();
  }
}
