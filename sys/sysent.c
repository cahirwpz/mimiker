#include <sysent.h>
#include <stdc.h>
#include <errno.h>

int sys_nosys(thread_t *td, syscall_args_t *args, int *error) {
  log("No such system call: %ld", args->code);
  *error = ENOSYS;
  return -1;
};

/* This is just a stub. A full implementation of this syscall will probably
   deserve a separate file. */
int sys_write(thread_t *td, syscall_args_t *args, int *error) {
  int fd = args->args[0];
  const char *buf = (const char *)(uintptr_t)args->args[1];
  size_t count = args->args[2];

  /* TODO: copyout string from userspace */
  if (fd == 1 || fd == 2) {
    /* We need a local copy of the string to null-terminate it after @count
       bytes, because the only printing facility we have is kprintf. */
    char buf2[count + 1];
    memcpy(buf2, buf, count);
    kprintf("%.*s", (int)count, buf);

    return count;
  }

  *error = EBADF;
  return -1;
}

/* clang-format hates long arrays. */
sysent_t sysent[] = {
  {sys_nosys}, {sys_nosys}, {sys_nosys}, {sys_nosys}, {sys_nosys}, {sys_write},
  {sys_nosys}, {sys_nosys}, {sys_nosys}, {sys_nosys}, {sys_nosys},
};
