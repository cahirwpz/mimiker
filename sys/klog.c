#include <sync.h>
#include <clock.h>
#include <stdc.h>
#define _KLOG_PRIVATE
#include <klog.h>

klog_t klog;

static const char *subsystems[] =
  {[KL_RUNQ] = "runq",   [KL_SLEEPQ] = "sleepq",   [KL_CALLOUT] = "callout",
   [KL_INIT] = "init",   [KL_PMAP] = "pmap",       [KL_VM] = "vm",
   [KL_KMEM] = "kmem",   [KL_POOL] = "pool",       [KL_LOCK] = "lock",
   [KL_SCHED] = "sched", [KL_THREAD] = "thread",   [KL_INTR] = "intr",
   [KL_DEV] = "dev",     [KL_VFS] = "vfs",         [KL_VNODE] = "vnode",
   [KL_PROC] = "proc",   [KL_SYSCALL] = "syscall", [KL_USER] = "user",
   [KL_TEST] = "test",   [KL_SIGNAL] = "signal",   [KL_FILESYS] = "filesys",
   [KL_UNDEF] = "???"};

/* Borrowed from mips/malta.c */
char *kenv_get(char *key);

void klog_init() {
  const char *mask = kenv_get("klog-mask");
  klog.mask = mask ? (unsigned)strtol(mask, NULL, 16) : KL_DEFAULT_MASK;
  klog.verbose = kenv_get("klog-quiet") ? 0 : 1;
  klog.first = 0;
  klog.last = 0;
}

static inline unsigned next(unsigned i) {
  return (i + 1) % KL_SIZE;
}

static void klog_entry_dump(klog_entry_t *entry) {
  if (entry->kl_origin == KL_UNDEF)
    kprintf("[%s:%d] ", entry->kl_file, entry->kl_line);
  else
    kprintf("[%s] ", subsystems[entry->kl_origin]);
  kprintf(entry->kl_format, entry->kl_params[0], entry->kl_params[1],
          entry->kl_params[2], entry->kl_params[3], entry->kl_params[4],
          entry->kl_params[5]);
  kprintf("\n");
}

void klog_append(klog_origin_t origin, const char *file, unsigned line,
                 const char *format, intptr_t arg1, intptr_t arg2,
                 intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6) {
  if (!(KL_MASK(origin) & klog.mask))
    return;

  klog_entry_t *entry;

  CRITICAL_SECTION {
    entry = &klog.array[klog.last];

    *entry = (klog_entry_t){.kl_timestamp = get_uptime(),
                            .kl_line = line,
                            .kl_file = file,
                            .kl_origin = origin,
                            .kl_format = format,
                            .kl_params = {arg1, arg2, arg3, arg4, arg5, arg6}};

    klog.last = next(klog.last);
    if (klog.first == klog.last)
      klog.first = next(klog.first);
  }

  if (klog.verbose)
    klog_entry_dump(entry);
}

unsigned klog_setmask(unsigned newmask) {
  unsigned oldmask;

  CRITICAL_SECTION {
    oldmask = klog.mask;
    klog.mask = newmask;
  }
  return oldmask;
}

void klog_dump() {
  klog_entry_t entry;

  while (klog.first != klog.last) {
    CRITICAL_SECTION {
      entry = klog.array[klog.first];
      klog.first = next(klog.first);
    }
    klog_entry_dump(&entry);
  }
}

void klog_clear() {
  klog.first = 0;
  klog.last = 0;
}
