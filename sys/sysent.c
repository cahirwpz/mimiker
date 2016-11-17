#include <sysent.h>
#include <stdc.h>
#include <errno.h>
#include <thread.h>
#include <sched.h>

int sys_nosys(thread_t *td, syscall_args_t *args) {
  kprintf("[syscall] unimplemented system call %ld\n", args->code);
  return -ENOSYS;
};

/* This is just a stub. A full implementation of this syscall will probably
   deserve a separate file. */
int sys_write(thread_t *td, syscall_args_t *args) {
  int fd = args->args[0];
  const char *buf = (const char *)(uintptr_t)args->args[1];
  size_t count = args->args[2];

  kprintf("[syscall] write(%d, %p, %zu)\n", fd, buf, count);

  /* TODO: copyout string from userspace */
  if (fd == 1 || fd == 2) {
    kprintf("%.*s", (int)count, buf);

    return count;
  }

  return -EBADF;
}

/* This is just a stub. A full implementation of this syscall will probably
   deserve a separate file. */
int sys_exit(thread_t *td, syscall_args_t *args) {
  int status = args->args[0];

  kprintf("[syscall] exit(%d)\n", status);

  /* Temporary implementation. */
  td->td_state = TDS_INACTIVE;
  sched_yield();
  __builtin_unreachable();
}

/* clang-format hates long arrays. */
sysent_t sysent[] = {
  {sys_nosys}, {sys_exit},  {sys_nosys}, {sys_nosys}, {sys_nosys}, {sys_write},
  {sys_nosys}, {sys_nosys}, {sys_nosys}, {sys_nosys}, {sys_nosys},
};
