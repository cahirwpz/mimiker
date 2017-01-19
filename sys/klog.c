#include <klog.h>

typedef struct klog_entry {
  uint64_t kl_timestamp;
  unsigned kl_line;
  const char *kl_file;
  const char *kl_format;
  intptr_t kl_params[6];
} klog_entry_t;

void klog_append(unsigned mask, const char *file, unsigned line,
                 const char *format, intptr_t arg1, intptr_t arg2,
                 intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6) {
}
