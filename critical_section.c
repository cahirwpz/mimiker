#include <critical_section.h>
#include <interrupts.h>
#include <common.h>
#include <libkern.h>

volatile int cs_level = 0;

void cs_enter() {
  if (during_intr_handler())
    return;
  intr_disable();
  cs_level++;
}

void cs_leave() {
  if (during_intr_handler())
    return;

  assert(cs_level > 0);

  cs_level--;
  if (cs_level == 0)
    intr_enable();
}
