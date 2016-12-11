#include <sysent.h>
#include <stdc.h>
#include <errno.h>
#include <thread.h>
#include <vm_map.h>
#include <vm_pager.h>
#include <sched.h>
#include <file.h>
#include <basic_dev.h>
#include <filedesc.h>
#include <vnode.h>
#include <mount.h>
#include <systm.h>

int sys_nosys(thread_t *td, syscall_args_t *args) {
  kprintf("[syscall] unimplemented system call %ld\n", args->code);
  return -ENOSYS;
};

int do_write(int fd, uio_t *uio) {
  thread_t *td = thread_self();
  file_t *f;
  int res = file_get_write(td, fd, &f);
  if (res)
    return res;
  res = f->f_ops->fo_write(f, td, uio);
  file_drop(f);
  return res;
}

int do_read(int fd, uio_t *uio) {
  thread_t *td = thread_self();
  file_t *f;
  int res = file_get_read(td, fd, &f);
  if (res)
    return res;
  res = f->f_ops->fo_read(f, td, uio);
  file_drop(f);
  return res;
}

int sys_write(thread_t *td, syscall_args_t *args) {
  int fd = args->args[0];
  char *ubuf = (char *)(uintptr_t)args->args[1];
  size_t count = args->args[2];

  log("sys_write(%d, %p, %zu)", fd, ubuf, count);

  /* Copyin buf */
  char buf[256];
  count = min(count, 256);
  int error = copyin(ubuf, buf, sizeof(buf));
  if (error < 0)
    return error;

  uio_t uio;
  iovec_t iov;
  uio.uio_op = UIO_WRITE;
  uio.uio_vmspace = get_user_vm_map();
  iov.iov_base = buf;
  iov.iov_len = count;
  uio.uio_iovcnt = 1;
  uio.uio_iov = &iov;
  uio.uio_resid = count;
  uio.uio_offset = 0;

  error = do_write(fd, &uio);
  if (error)
    return -error;
  return count - uio.uio_resid;
}

int sys_read(thread_t *td, syscall_args_t *args) {
  int fd = args->args[0];
  char *ubuf = (char *)(uintptr_t)args->args[1];
  size_t count = args->args[2];

  char buf[256];
  count = min(count, 256);

  kprintf("[syscall] read(%d, %p, %zu)\n", fd, ubuf, count);

  uio_t uio;
  iovec_t iov;
  uio.uio_op = UIO_WRITE;
  uio.uio_vmspace = get_user_vm_map();
  iov.iov_base = buf;
  iov.iov_len = count;
  uio.uio_iovcnt = 1;
  uio.uio_iov = &iov;
  uio.uio_resid = count;
  uio.uio_offset = 0;

  int error = do_read(fd, &uio);
  if (error)
    return -error;
  int read = count - uio.uio_resid;
  error = copyout(buf, ubuf, read);
  if (error < 0)
    return error;
  return read;
}

int sys_close(thread_t *td, syscall_args_t *args) {
  int fd = args->args[0];

  kprintf("[syscall] close(%d)\n", fd);

  return -file_desc_close(td->td_fdt, fd);
}

int do_open(file_t *f, char *pathname, int flags, int mode) {
  vnode_t *v;
  int error;
  error = vfs_lookup(pathname, &v);
  if (error)
    return error;
  error = VOP_OPEN(v, mode, f);
  if (error)
    return error;
  return 0;
}

int sys_open(thread_t *td, syscall_args_t *args) {
  char *user_pathname = (char *)args->args[0];
  int flags = args->args[1];
  int mode = args->args[2];

  int error = 0;
  char pathname[256];
  size_t n = 0;

  /* Copyout pathname. */
  error = copyinstr(user_pathname, pathname, sizeof(pathname), &n);
  if (error < 0)
    return error;

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
  return -error;
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
