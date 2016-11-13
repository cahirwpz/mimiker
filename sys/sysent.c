#include <sysent.h>
#include <stdc.h>
#include <errno.h>
#include <thread.h>
#include <vm_map.h>
#include <vm_pager.h>
#include <sched.h>

int sys_nosys(thread_t *td, syscall_args_t *args) {
  log("No such system call: %ld", args->code);
  return -ENOSYS;
};

/* This is just a stub. A full implementation of this syscall will probably
   deserve a separate file. */
int sys_write(thread_t *td, syscall_args_t *args) {
  int fd = args->args[0];
  const char *buf = (const char *)(uintptr_t)args->args[1];
  size_t count = args->args[2];

  /* TODO: copyout string from userspace */
  if (fd == 1 || fd == 2) {
    kprintf("%.*s", (int)count, buf);

    return count;
  }

  return -EBADF;
}

/* This is just a stub. A full implementation of this syscall will probably
   deserve a separate file. */
/* Note that this sbrk implementation does not actually extend .data section,
   because we have no guarantee that there is any free space after .data in the
   memory map. But it does not matter much, because no application would assume
   that we are actually expanding .data, it will use the pointer returned by
   sbrk. */
int sys_sbrk(thread_t *td, syscall_args_t *args) {
  size_t increment = (size_t)args->args[0];

  kprintf("[syscall] sbrk(%zu)\n", increment);

  assert(td->td_uspace);

  if (!td->td_uspace->td_brk_entry) {
    /* sbrk was not used before by this thread. */
    size_t size = roundup(increment, PAGESIZE);
    vm_addr_t addr;
    if (vm_map_findspace(td->td_uspace, BRK_SEARCH_START, size, &addr) != 0)
      return -ENOMEM;
    vm_map_entry_t *entry = vm_map_add_entry(td->td_uspace, addr, addr + size,
                                             VM_PROT_READ | VM_PROT_WRITE);
    entry->object = default_pager->pgr_alloc();

    td->td_uspace->td_brk_entry = entry;
    td->td_uspace->td_brk = addr + increment;

    return addr;
  } else {
    /* There already is a brk segment in user space map. */
    vm_map_entry_t *brk_entry = td->td_uspace->td_brk_entry;
    vm_addr_t brk = td->td_uspace->td_brk;
    if (brk + increment <= brk_entry->end) {
      /* No need to expand the segment. */
      td->td_uspace->td_brk = brk + increment;
      return brk;
    }
    vm_addr_t new_end = roundup(brk + increment, PAGESIZE);
    if (vm_map_expand(td->td_uspace, brk_entry, new_end) != 0) {
      /* Map entry expansion failed. */
      return -ENOMEM;
    }
    td->td_uspace->td_brk += increment;
    return brk;
  }
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
  {sys_nosys}, {sys_nosys}, {sys_nosys}, {sys_nosys}, {sys_nosys}, {sys_sbrk},
};
