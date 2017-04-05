#include <sync.h>
#include <clock.h>
#include <stdc.h>
#include <klog.h>

/* Borrowed from mips/malta.c */
char *kenv_get(char *key);

#ifndef KL_SIZE
#define KL_SIZE 1024
int kl_size = KL_SIZE;
#endif

#ifndef KL_MASK
#define KL_MASK                                                                \
  kenv_get("klog-mask") ? (unsigned)strtol(kenv_get("klog-mask"), NULL, 16)    \
                        : KL_NONE
volatile int first_message = 0;
volatile int last_message = 0;
#endif

#ifndef KL_TIME
#define KL_TIME kenv_get("klog-time") ? 1 : 0
#endif

#ifndef KL_VERBOSE
#define KL_VERBOSE kenv_get("klog-verbose") ? 1 : 0
#endif

typedef struct klog_entry {
  realtime_t kl_timestamp;
  unsigned kl_line;
  const char *kl_file;
  const char *kl_format;
  intptr_t kl_params[6];
} klog_entry_t;

static klog_entry_t klog_array[KL_SIZE];

void klog_append(unsigned mask, const char *file, unsigned line,
                 const char *format, intptr_t arg1, intptr_t arg2,
                 intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6) {

  if (((KL_MASK)&mask) == 0)
    return;

  klog_entry_t *entry = &klog_array[last_message];

  last_message = (last_message + 1) & (kl_size - 1);
  if (first_message == last_message)
    first_message = (first_message + 1) & (kl_size - 1);

  entry->kl_timestamp = clock_get();
  entry->kl_line = line;
  entry->kl_file = file;
  entry->kl_format = format;
  entry->kl_params[0] = arg1;
  entry->kl_params[1] = arg2;
  entry->kl_params[2] = arg3;
  entry->kl_params[3] = arg4;
  entry->kl_params[4] = arg5;
  entry->kl_params[5] = arg6;

  if (KL_VERBOSE) {
    if (KL_TIME)
      kprintf("[%lld]", entry->kl_timestamp);
    kprintf("[%s:%d]\t", file, line);
    kprintf(format, arg1, arg2, arg3, arg4, arg5, arg6);
    kprintf("\n");
  }
}

void klog_dump_all() {
  while (first_message != last_message) {
    klog_entry_t *entry = &klog_array[first_message];
    if (KL_TIME)
      kprintf("[%lld]", entry->kl_timestamp);
    kprintf("[%s:%d]\t", entry->kl_file, entry->kl_line);
    kprintf(entry->kl_format, entry->kl_params[0], entry->kl_params[1],
            entry->kl_params[2], entry->kl_params[3], entry->kl_params[4],
            entry->kl_params[5]);
    kprintf("\n");
    first_message = (first_message + 1) & (kl_size - 1);
  }
}

void klog_clear() {
  first_message = last_message = 0;
}
