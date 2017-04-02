#include <sysent.h>
#include <stdc.h>
#include <errno.h>
#include <thread.h>
#include <vm_map.h>
#include <vm_pager.h>
#include <vfs_syscalls.h>
#include <sbrk.h>

int sys_nosys(thread_t *td, syscall_args_t *args) {
  kprintf("[syscall] unimplemented system call %ld\n", args->code);
  return -ENOSYS;
};

int sys_sbrk(thread_t *td, syscall_args_t *args) {
  intptr_t increment = (size_t)args->args[0];

  kprintf("[syscall] sbrk(%zu)\n", increment);

  /* TODO: Shrinking sbrk is impossible, because it requires unmapping pages,
   * which is not yet implemented! */
  if (increment < 0) {
    log("WARNING: sbrk called with a negative argument!");
    return -ENOMEM;
  }

  assert(td->td_uspace);

  return sbrk_resize(td->td_uspace, increment);
}

/* This is just a stub. A full implementation of this syscall will probably
   deserve a separate file. */
int sys_exit(thread_t *td, syscall_args_t *args) {
  int status = args->args[0];

  kprintf("[syscall] exit(%d)\n", status);

  thread_exit(status);
  __builtin_unreachable();
}

/* clang-format hates long arrays. */
sysent_t sysent[] = {
  {sys_nosys}, {sys_exit},  {sys_open},  {sys_close}, {sys_read},  {sys_write},
  {sys_lseek}, {sys_nosys}, {sys_nosys}, {sys_nosys}, {sys_fstat}, {sys_sbrk},
};
