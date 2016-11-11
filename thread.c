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

int main(int argc, char **argv, char **envp) {
  kprintf("Thread '%s' started.\n", thread_self()->td_name);
  kprintf("argc = %d\n", argc);
  kprintf("argv = %p\n", argv);
  kprintf("argp = %p\n", envp);

  td0 = thread_self();
  td1 = thread_create("first", demo_thread_1);
  td2 = thread_create("second", demo_thread_2);

  thread_switch_to(td1);

  kprintf("Thread '%s' running.\n", thread_self()->td_name);

  thread_dump_all();

  thread_delete(td2);
  thread_delete(td1);

  return 0;
}
