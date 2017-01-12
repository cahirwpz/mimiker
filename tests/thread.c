#include <stdc.h>
#include <thread.h>

static thread_t *td0, *td1, *td2;

static void demo_thread_1() {
  int times = 3;

  kprintf("Thread '%s' started.\n", thread_self()->td_name);

  do {
    thread_switch_to(td2);
    kprintf("Thread '%s' running.\n", thread_self()->td_name);
  } while (--times);

  thread_switch_to(td0);
}

static void demo_thread_2() {
  int times = 3;

  kprintf("Thread '%s' started.\n", thread_self()->td_name);

  do {
    thread_switch_to(td1);
    kprintf("Thread '%s' running.\n", thread_self()->td_name);
  } while (--times);

  panic("This line need not be reached!");
}

int main() {
  kprintf("Thread '%s' started.\n", thread_self()->td_name);

  td0 = thread_self();
  td1 = thread_create("first", demo_thread_1, NULL);
  td2 = thread_create("second", demo_thread_2, NULL);

  thread_switch_to(td1);

  kprintf("Thread '%s' running.\n", thread_self()->td_name);

  assert(td0 == thread_get_by_tid(td0->td_tid));
  assert(td1 == thread_get_by_tid(td1->td_tid));
  assert(td2 == thread_get_by_tid(td2->td_tid));
  assert(NULL == thread_get_by_tid(1234));

  thread_dump_all();

  thread_delete(td2);
  thread_delete(td1);

  return 0;
}
