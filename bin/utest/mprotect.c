#include "utest.h"
#include "util.h"

#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define check_write_ok(addr)                                                   \
  { *((char *)(addr)) = 'x'; }

#define check_write_err(si, addr)                                              \
  {                                                                            \
    EXPECT_SIGNAL(SIGSEGV, &(si)) {                                            \
      *((char *)(addr)) = 'x';                                                 \
    }                                                                          \
    CLEANUP_SIGNAL();                                                          \
    CHECK_SIGSEGV(&(si), (addr), SEGV_ACCERR);                                 \
  }

#define check_read_ok(addr)                                                    \
  {                                                                            \
    char v = *((char *)(addr));                                                \
    if (v)                                                                     \
      printf("\n");                                                            \
  }

#define check_read_err(si, addr)                                               \
  {                                                                            \
    EXPECT_SIGNAL(SIGSEGV, &(si)) {                                            \
      char v = *((char *)(addr));                                              \
      if (v)                                                                   \
        printf("\n");                                                          \
    }                                                                          \
    CLEANUP_SIGNAL();                                                          \
    CHECK_SIGSEGV(&(si), (addr), SEGV_ACCERR);                                 \
  }

#define check_exec_ok(addr)                                                    \
  { ((void (*)(int))(addr))(1); }

#define check_exec_err(si, addr)                                               \
  {                                                                            \
    EXPECT_SIGNAL(SIGSEGV, &(si)) {                                            \
      ((void (*)(int))(addr))(1);                                              \
    }                                                                          \
    CLEANUP_SIGNAL();                                                          \
    CHECK_SIGSEGV(&(si), (addr), SEGV_ACCERR);                                 \
  }

#define check_none_prot(si, addr)                                              \
  {                                                                            \
    check_read_err(si, addr);                                                  \
    check_write_err(si, addr);                                                 \
    check_exec_err(si, addr);                                                  \
  }

/* XXX In theory compiler may reorder those function and indeed Clang does it
 * when they're marked with `static` quantifier. */

int __func_inc_mprotect(int x) {
  return x + 1;
}

int __func_dec_mprotect(int x) {
  return x - 1;
}

static int (*func_inc)(int) = __func_inc_mprotect;
static int (*func_dec)(int) = __func_dec_mprotect;

static void memcpy_fun(void *addr) {
  assert((uintptr_t)func_inc < (uintptr_t)func_dec);

  /* TODO: how to properly determine function length? */
  size_t len = (char *)func_dec - (char *)func_inc;
  memcpy(addr, func_inc, len);
}

TEST_ADD(mprotect_fail) {
  size_t pgsz = getpagesize();
  void *addr = mmap_anon_priv(NULL, pgsz, PROT_NONE);
  siginfo_t si;

  check_none_prot(si, addr);

  /* Is not valid prot. */
  syscall_fail(mprotect(addr, pgsz, 0xbeef), EINVAL);

  /* addr is not aligned */
  syscall_fail(mprotect(addr + 0x10, pgsz, PROT_READ), EINVAL);

  /* len is not aligned */
  syscall_fail(mprotect(addr, pgsz + 0x10, PROT_READ), EINVAL);

  /* trying to change prot of unmapped memory */
  syscall_fail(mprotect(addr, 3 * pgsz, PROT_READ), ENOMEM);
  syscall_fail(mprotect(addr - pgsz, 3 * pgsz, PROT_READ), ENOMEM);

  /* len must be nonzero */
  syscall_fail(mprotect(addr, 0, PROT_READ), EINVAL);

  syscall_ok(munmap(addr, pgsz));

  return 0;
}

TEST_ADD(mprotect1) {
  size_t pgsz = getpagesize();
  void *addr = mmap_anon_priv(NULL, pgsz, PROT_NONE);
  siginfo_t si;

  check_none_prot(si, addr);

  syscall_ok(mprotect(addr, pgsz, PROT_READ | PROT_WRITE | PROT_EXEC));

  /* Copy function to address that won't be overritten by checks. */
  void *fun_addr = addr + 4;
  memcpy_fun(fun_addr);

  check_read_ok(addr);
  check_write_ok(addr);
  check_exec_ok(fun_addr);

  syscall_ok(mprotect(addr, pgsz, PROT_READ | PROT_WRITE));

  check_read_ok(addr);
  check_write_ok(addr);
  check_exec_err(si, fun_addr);

  syscall_ok(mprotect(addr, pgsz, PROT_READ | PROT_EXEC));

  check_read_ok(addr);
  check_write_err(si, addr);
  check_exec_ok(fun_addr);

  syscall_ok(mprotect(addr, pgsz, PROT_READ));

  check_read_ok(addr);
  check_write_err(si, addr);
  check_exec_err(si, fun_addr);

  syscall_ok(mprotect(addr, pgsz, PROT_NONE));

  check_none_prot(si, addr);

  return 0;
}

static void *prepare_none_layout(size_t pgsz) {
  void *addr = prepare_layout(pgsz, PROT_NONE);
  if (addr == NULL)
    return NULL;

  siginfo_t si;

  check_none_prot(si, addr);
  check_none_prot(si, addr + pgsz);
  check_none_prot(si, addr + 3 * pgsz);
  check_none_prot(si, addr + 5 * pgsz);
  check_none_prot(si, addr + 6 * pgsz);

  return addr;
}

/* Change only some entries from prepared layout and check if protection
 * was changed correctly.
 */
TEST_ADD(mprotect2) {
  size_t pgsz = getpagesize();
  void *addr = prepare_none_layout(pgsz);
  siginfo_t si;

  syscall_fail(mprotect(addr + pgsz, 5 * pgsz, PROT_READ | PROT_WRITE), ENOMEM);

  syscall_ok(mprotect(addr + pgsz, pgsz, PROT_READ | PROT_WRITE));
  syscall_ok(mprotect(addr + 3 * pgsz, pgsz, PROT_READ | PROT_WRITE));
  syscall_ok(mprotect(addr + 5 * pgsz, pgsz, PROT_READ | PROT_WRITE));

  check_none_prot(si, addr);

  check_read_ok(addr + 1 * pgsz);
  check_write_ok(addr + 1 * pgsz);
  check_exec_err(si, addr + 1 * pgsz);

  check_read_ok(addr + 3 * pgsz);
  check_write_ok(addr + 3 * pgsz);
  check_exec_err(si, addr + 3 * pgsz);

  check_read_ok(addr + 5 * pgsz);
  check_write_ok(addr + 5 * pgsz);
  check_exec_err(si, addr + 5 * pgsz);

  check_none_prot(si, addr + 6 * pgsz);

  /* Copy function to address on 6th page that won't be overritten by checks. */
  void *fun_addr = addr + 5 * pgsz + 4;
  memcpy_fun(fun_addr);

  syscall_fail(mprotect(addr + pgsz, 3 * pgsz, PROT_READ), ENOMEM);

  syscall_ok(mprotect(addr + pgsz, pgsz, PROT_READ));
  syscall_ok(mprotect(addr + 3 * pgsz, pgsz, PROT_READ));

  check_none_prot(si, addr);

  check_read_ok(addr + 1 * pgsz);
  check_write_err(si, addr + 1 * pgsz);
  check_exec_err(si, addr + 1 * pgsz);

  check_read_ok(addr + 3 * pgsz);
  check_write_err(si, addr + 3 * pgsz);
  check_exec_err(si, addr + 3 * pgsz);

  check_read_ok(addr + 5 * pgsz);
  check_write_ok(addr + 5 * pgsz);
  check_exec_err(si, addr + 5 * pgsz);

  check_none_prot(si, addr + 6 * pgsz);

  /* TODO: change to READ | WRITE */
  syscall_ok(
    mprotect(addr + 5 * pgsz, pgsz, PROT_READ | PROT_WRITE | PROT_EXEC));

  check_none_prot(si, addr);

  check_read_ok(addr + 1 * pgsz);
  check_write_err(si, addr + 1 * pgsz);
  check_exec_err(si, addr + 1 * pgsz);

  check_read_ok(addr + 3 * pgsz);
  check_write_err(si, addr + 3 * pgsz);
  check_exec_err(si, addr + 3 * pgsz);

  check_read_ok(addr + 5 * pgsz);
  check_write_ok(addr + 5 * pgsz);
  check_exec_ok(fun_addr);

  check_none_prot(si, addr + 6 * pgsz);

  syscall_ok(mprotect(addr + pgsz, pgsz, PROT_READ | PROT_WRITE));
  syscall_ok(mprotect(addr + 3 * pgsz, pgsz, PROT_READ | PROT_WRITE));

  check_none_prot(si, addr);

  check_read_ok(addr + 1 * pgsz);
  check_write_ok(addr + 1 * pgsz);
  check_exec_err(si, addr + 1 * pgsz);

  check_read_ok(addr + 3 * pgsz);
  check_write_ok(addr + 3 * pgsz);
  check_exec_err(si, addr + 3 * pgsz);

  check_read_ok(addr + 5 * pgsz);
  check_write_ok(addr + 5 * pgsz);
  check_exec_ok(fun_addr);

  check_none_prot(si, addr + 6 * pgsz);

  syscall_fail(mprotect(addr + pgsz, 5 * pgsz, PROT_NONE), ENOMEM);

  syscall_ok(mprotect(addr + pgsz, pgsz, PROT_NONE));
  syscall_ok(mprotect(addr + 3 * pgsz, pgsz, PROT_NONE));
  syscall_ok(mprotect(addr + 5 * pgsz, pgsz, PROT_NONE));

  check_none_prot(si, addr);
  check_none_prot(si, addr + pgsz);
  check_none_prot(si, addr + 3 * pgsz);
  check_none_prot(si, addr + 5 * pgsz);
  check_none_prot(si, addr + 6 * pgsz);

  return 0;
}

/* Change protection for page inside given memory range. This will trigger
 * vm_map_entry splitting.
 */
TEST_ADD(mprotect3) {
  size_t pgsz = getpagesize();
  void *addr = mmap_anon_priv(NULL, 4 * pgsz, PROT_NONE);
  siginfo_t si;

  check_none_prot(si, addr);
  check_none_prot(si, addr + pgsz);
  check_none_prot(si, addr + 2 * pgsz);
  check_none_prot(si, addr + 3 * pgsz);

  syscall_ok(mprotect(addr + pgsz, 2 * pgsz, PROT_READ | PROT_WRITE));

  check_none_prot(si, addr);

  check_read_ok(addr + 1 * pgsz);
  check_write_ok(addr + 1 * pgsz);
  check_exec_err(si, addr + 1 * pgsz);

  check_read_ok(addr + 2 * pgsz);
  check_write_ok(addr + 2 * pgsz);
  check_exec_err(si, addr + 2 * pgsz);

  check_none_prot(si, addr + 3 * pgsz);

  syscall_ok(mprotect(addr + pgsz, pgsz, PROT_READ));

  check_none_prot(si, addr);

  check_read_ok(addr + 1 * pgsz);
  check_write_err(si, addr + 1 * pgsz);
  check_exec_err(si, addr + 1 * pgsz);

  check_read_ok(addr + 2 * pgsz);
  check_write_ok(addr + 2 * pgsz);
  check_exec_err(si, addr + 2 * pgsz);

  check_none_prot(si, addr + 3 * pgsz);

  syscall_ok(mprotect(addr + pgsz, 2 * pgsz, PROT_NONE));

  check_none_prot(si, addr);
  check_none_prot(si, addr + pgsz);
  check_none_prot(si, addr + 2 * pgsz);
  check_none_prot(si, addr + 3 * pgsz);

  return 0;
}
