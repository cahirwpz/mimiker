#include <sync.h>
#include <clock.h>
#include <stdc.h>
#define _KLOG_PRIVATE
#include <klog.h>

klog_t klog;

/* Borrowed from mips/malta.c */
char *kenv_get(char *key);

void klog_init() {
  const char *mask = kenv_get("klog-mask");
  klog.mask = mask ? (unsigned)strtol(mask, NULL, 16) : KL_ALL;
  klog.verbose = kenv_get("klog-verbose") ? 1 : 0;
  klog.first = 0;
  klog.last = 0;
}

static inline unsigned next(unsigned i) {
  return (i + 1) % KL_SIZE;
}

static void klog_entry_dump(klog_entry_t *entry) {
  kprintf("(%lld) [%s:%d] ", entry->kl_timestamp, entry->kl_file,
          entry->kl_line);
  kprintf(entry->kl_format, entry->kl_params[0], entry->kl_params[1],
          entry->kl_params[2], entry->kl_params[3], entry->kl_params[4],
          entry->kl_params[5]);
  kprintf("\n");
}

void klog_append(unsigned mask, const char *file, unsigned line,
                 const char *format, intptr_t arg1, intptr_t arg2,
                 intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6)
{
  if (!(mask & klog.mask))
    return;

  critical_enter();

  klog_entry_t *entry = &klog.array[klog.last];

  *entry = (klog_entry_t){
    .kl_timestamp = clock_get(),
    .kl_line = line,
    .kl_file = file,
    .kl_format = format,
    .kl_params = {arg1, arg2, arg3, arg4, arg5, arg6}
  };

  klog.last = next(klog.last);
  if (klog.first == klog.last)
    klog.first = next(klog.first);

  critical_leave();

  if (klog.verbose)
    klog_entry_dump(entry);
}

void klog_dump() {
  klog_entry_t entry;

  while (klog.first != klog.last) {
    critical_enter();
    entry = klog.array[klog.first];
    klog.first = next(klog.first);
    critical_leave();
    klog_entry_dump(&entry);
  }
}

void klog_clear() {
  klog.first = 0;
  klog.last = 0;
}
