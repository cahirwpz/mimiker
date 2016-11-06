#include <stdc.h>
#include <sched.h>
#include <turnstile.h>
#include <thread.h>

mtx_sleep_t mtx;
volatile int32_t value;


thread_t *td2;

void thread1_main()
{
  mtx_sleep_lock(&mtx);
  for(size_t i = 0; i < 10; i++) {
    //`for(size_t j = 0; j < 100000; j++);
    kprintf("%s: %ld\n", thread_self()->td_name, (long)value);
    value++;
  }
  mtx_sleep_unlock(&mtx);
  while(1);
}

void never_unlock()
{
    mtx_sleep_lock(&mtx);
    while(1);
}

void test1()
{
    mtx_sleep_init(&mtx);
    thread_t *td1 = thread_create("td1", thread1_main);
    thread_t *td2 = thread_create("td2", thread1_main);
    thread_t *td3 = thread_create("td3", thread1_main);
    thread_t *td4 = thread_create("td4", thread1_main);
    thread_t *td5 = thread_create("td5", thread1_main);
    sched_add(td1);
    sched_add(td2);
    sched_add(td3);
    sched_add(td4);
    sched_add(td5);
    sched_run();
}

int main()
{
    test1();
    return 0;
}
