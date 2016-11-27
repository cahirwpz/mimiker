#include <sysent.h>
#include <stdc.h>
#include <errno.h>
#include <thread.h>
#include <vm_map.h>
#include <vm_pager.h>
#include <sched.h>
#include <file.h>
#include <basic_dev.h>

int sys_nosys(thread_t *td, syscall_args_t *args) {
  kprintf("[syscall] unimplemented system call %ld\n", args->code);
  return -ENOSYS;
};

/* These are just stubs. A full implementation of some syscalls will probably
   deserve a separate file. */
int sys_write(thread_t *td, syscall_args_t *args) {
  int fd = args->args[0];
  char *buf = (char *)(uintptr_t)args->args[1];
  size_t count = args->args[2];

  kprintf("[syscall] write(%d, %p, %zu)\n", fd, buf, count);

  file_t *f;
  int res = file_get_write(td, fd, &f);
  if (res)
    return res;
  res = f->f_ops->fo_write(f, td, buf, count);
  file_drop(f);
  return res;
}

int sys_read(thread_t *td, syscall_args_t *args) {
  int fd = args->args[0];
  char *buf = (char *)(uintptr_t)args->args[1];
  size_t count = args->args[2];

  kprintf("[syscall] read(%d, %p, %zu)\n", fd, buf, count);

  file_t *f;
  int res = file_get_read(td, fd, &f);
  if (res)
    return res;
  res = f->f_ops->fo_read(f, td, buf, count);
  file_drop(f);
  return res;
}

int sys_close(thread_t *td, syscall_args_t *args) {
  int fd = args->args[0];

  kprintf("[syscall] close(%d)\n", fd);

  return file_desc_close(td->td_fdt, fd);
}

int do_open(file_t *f, char *pathname, int flags, int mode) {
  /* Note: We lack file system, so this implementation is very silly. */
  if (strcmp(pathname, "/dev/null") == 0) {
    return dev_null_open(f, flags, mode);
  }
  return -ENOENT;
}

int sys_open(thread_t *td, syscall_args_t *args) {
  char *pathname = (char *)args->args[0];
  int flags = args->args[1];
  int mode = args->args[2];

  int error = 0;

  /* TODO: Copyout pathname! */
  kprintf("[syscall] open(%s, %d, %d)\n", pathname, flags, mode);

  /* Allocate a file structure, but do not install descriptor yet. */
  file_t *f = file_alloc_noinstall();
  /* Try opening file. Fill the file structure. */
  error = do_open(f, pathname, flags, mode);
  if (error)
    goto fail;
  /* Now install the file in descriptor table. */
  int fd;
  error = file_install_desc(td->td_fdt, f, &fd);
  if (error)
    goto fail;

  /* The file is stored in the descriptor table, but we got our own reference
     when we asked to allocate the file. Thus we need to release that initial
     ref. */
  file_drop(f);
  return fd;

fail:
  file_drop(f);
  return error;
}

/* This is just a stub. A full implementation of this syscall will probably
   deserve a separate file. */
/* Note that this sbrk implementation does not actually extend .data section,
   because we have no guarantee that there is any free space after .data in the
   memory map. But it does not matter much, because no application would assume
   that we are actually expanding .data, it will use the pointer returned by
   sbrk. */
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

  if (!td->td_uspace->sbrk_entry) {
    /* sbrk was not used before by this thread. */
    size_t size = roundup(increment, PAGESIZE);
    vm_addr_t addr;
    if (vm_map_findspace(td->td_uspace, BRK_SEARCH_START, size, &addr) != 0)
      return -ENOMEM;
    vm_map_entry_t *entry = vm_map_add_entry(td->td_uspace, addr, addr + size,
                                             VM_PROT_READ | VM_PROT_WRITE);
    entry->object = default_pager->pgr_alloc();

    td->td_uspace->sbrk_entry = entry;
    td->td_uspace->sbrk_end = addr + increment;

    return addr;
  } else {
    /* There already is a brk segment in user space map. */
    vm_map_entry_t *brk_entry = td->td_uspace->sbrk_entry;
    vm_addr_t brk = td->td_uspace->sbrk_end;
    if (brk + increment == brk_entry->end) {
      /* No need to resize the segment. */
      td->td_uspace->sbrk_end = brk + increment;
      return brk;
    }
    /* Shrink or expand the vm_map_entry */
    vm_addr_t new_end = roundup(brk + increment, PAGESIZE);
    if (vm_map_resize(td->td_uspace, brk_entry, new_end) != 0) {
      /* Map entry expansion failed. */
      return -ENOMEM;
    }
    td->td_uspace->sbrk_end += increment;
    return brk;
  }
}

/* This is just a stub. A full implementation of this syscall will probably
   deserve a separate file. */
int sys_exit(thread_t *td, syscall_args_t *args) {
  int status = args->args[0];

  kprintf("[syscall] exit(%d)\n", status);

  thread_exit();
  __builtin_unreachable();
}

/* clang-format hates long arrays. */
sysent_t sysent[] = {
  {sys_nosys}, {sys_exit},  {sys_open},  {sys_close}, {sys_read},  {sys_write},
  {sys_nosys}, {sys_nosys}, {sys_nosys}, {sys_nosys}, {sys_nosys}, {sys_sbrk},
};
