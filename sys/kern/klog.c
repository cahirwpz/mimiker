#include <sys/mimiker.h>
#include <sys/mutex.h>
#include <sys/kenv.h>
#include <sys/time.h>
#include <sys/libkern.h>
#include <sys/thread.h>
#define _KLOG_PRIVATE
#include <sys/klog.h>

klog_t klog;

static spin_t klog_lock = SPIN_INITIALIZER(LK_RECURSIVE);

static const char *subsystems[] = {
  [KL_RUNQ] = "runq",   [KL_SLEEPQ] = "sleepq",   [KL_CALLOUT] = "callout",
  [KL_INIT] = "init",   [KL_PMAP] = "pmap",       [KL_VM] = "vm",
  [KL_KMEM] = "kmem",   [KL_VMEM] = "vmem",       [KL_LOCK] = "lock",
  [KL_SCHED] = "sched", [KL_THREAD] = "thread",   [KL_INTR] = "intr",
  [KL_DEV] = "dev",     [KL_VFS] = "vfs",         [KL_VNODE] = "vnode",
  [KL_PROC] = "proc",   [KL_SYSCALL] = "syscall", [KL_USER] = "user",
  [KL_TEST] = "test",   [KL_SIGNAL] = "signal",   [KL_FILESYS] = "filesys",
  [KL_TIME] = "time",   [KL_FILE] = "file",       [KL_TTY] = "tty",
  [KL_UNDEF] = "???"};

void init_klog(void) {
  const char *mask = kenv_get("klog-mask");
  klog.mask = mask ? (unsigned)strtol(mask, NULL, 16) : KL_DEFAULT_MASK;
  klog.verbose = kenv_get("klog-quiet") ? 0 : 1;
  klog.first = 0;
  klog.last = 0;
  klog.repeated = 0;
  klog.prev = -1;
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
                 const char *format, uintptr_t arg1, uintptr_t arg2,
                 uintptr_t arg3, uintptr_t arg4, uintptr_t arg5,
                 uintptr_t arg6) {
  if (!(KL_MASK(origin) & klog.mask))
    return;

  klog_entry_t *entry;
  tid_t tid = thread_self()->td_tid;

  WITH_SPIN_LOCK (&klog_lock) {
    bintime_t now = binuptime();

    entry = (klog.prev >= 0) ? &klog.array[klog.prev] : NULL;

    /* Do not store repeating log messages, just count them. */
    if (entry) {
      bool repeats =
        (entry->kl_params[0] == arg1) && (entry->kl_params[1] == arg2) &&
        (entry->kl_params[2] == arg3) && (entry->kl_params[3] == arg4) &&
        (entry->kl_params[4] == arg5) && (entry->kl_params[5] == arg6) &&
        (entry->kl_origin == origin) && (entry->kl_file == file) &&
        (entry->kl_line == line) && (entry->kl_tid = tid);

      if (repeats) {
        if (!klog.repeated) {
          int old_prev = klog.prev;
          klog.prev = -1;
          klog_append(entry->kl_origin, entry->kl_file, entry->kl_line,
                      "Last message repeated %d times.", 0, 0, 0, 0, 0, 0);
          klog.prev = old_prev;
          klog.repeated = true;
        }

        entry = &klog.array[next(klog.prev)];
        entry->kl_timestamp = now;
        entry->kl_params[0]++;
        return;
      } else {
        klog.repeated = false;
      }
    }

    entry = &klog.array[klog.last];

    *entry = (klog_entry_t){.kl_timestamp = now,
                            .kl_tid = tid,
                            .kl_line = line,
                            .kl_file = file,
                            .kl_origin = origin,
                            .kl_format = format,
                            .kl_params = {arg1, arg2, arg3, arg4, arg5, arg6}};

    klog.prev = klog.last;
    klog.last = next(klog.last);
    if (klog.first == klog.last)
      klog.first = next(klog.first);
  }

  if (klog.verbose && !intr_disabled())
    klog_entry_dump(entry);
}

unsigned klog_setmask(unsigned newmask) {
  unsigned oldmask;

  WITH_SPIN_LOCK (&klog_lock) {
    oldmask = klog.mask;
    klog.mask = newmask;
  }
  return oldmask;
}

void klog_dump(void) {
  klog_entry_t entry;

  while (klog.first != klog.last) {
    WITH_SPIN_LOCK (&klog_lock) {
      entry = klog.array[klog.first];
      klog.first = next(klog.first);
    }
    klog_entry_dump(&entry);
  }
}

void klog_clear(void) {
  klog.first = 0;
  klog.last = 0;
}
