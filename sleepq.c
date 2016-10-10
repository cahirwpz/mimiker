#include <stdc.h>
#include <sleepq.h>
#include <thread.h>

int main() {
  thread_t t1, t2;
  sleepq_t sq1, sq2;
  memset(&t1, 0, sizeof(t1));
  memset(&sq1, 0, sizeof(sq1));
  memset(&t2, 0, sizeof(t2));
  memset(&sq2, 0, sizeof(sq2));

  t1.td_sleepqueue = &sq1;
  t2.td_sleepqueue = &sq2;

  void *wchan = (void *)0x123;

  sleepq_add(wchan, NULL, &t1);
  sleepq_add(wchan, NULL, &t2);

  sleepq_signal(wchan);
  sleepq_signal(wchan);

  sleepq_add(wchan, NULL, &t2);
  sleepq_add(wchan, NULL, &t1);

  sleepq_broadcast(wchan);

  void *wchan2 = (void *)0x124;

  sleepq_add(wchan, NULL, &t1);
  sleepq_add(wchan2, NULL, &t2);

  sleepq_signal(wchan);
  sleepq_signal(wchan2);

  sleepq_add(wchan, NULL, &t1);
  sleepq_add(wchan2, NULL, &t2);

  sleepq_remove(&t1, wchan);
  sleepq_remove(&t2, wchan2);

  return 0;
}
