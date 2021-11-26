/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2019 Mitchell Horne <mhorne@FreeBSD.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* MODIFIED */

#include <sys/klog.h>
#include <sys/mimiker.h>
#include <riscv/sbi.h>

typedef struct {
  long error;
  long value;
} sbi_ret_t;

/* SBI implementation info. */
static unsigned long sbi_spec_version;
static unsigned long sbi_impl_id;
static unsigned long sbi_impl_version;

static bool has_console_putchar_extension = false;
static bool has_console_getchar_extension = false;
static bool has_shutdown_extension = false;
static bool has_time_extension = false;
static bool has_ipi_extension = false;
static bool has_rfnc_extension = false;

/* Hardware implementation info. */
static unsigned long mvendorid; /* CPU's JEDEC vendor ID */
static unsigned long marchid;
static unsigned long mimpid;

static sbi_ret_t sbi_call(unsigned long ext, unsigned long func,
                          unsigned long arg0, unsigned long arg1,
                          unsigned long arg2, unsigned long arg3,
                          unsigned long arg4) {
  register register_t a0 __asm("a0") = (register_t)(arg0);
  register register_t a1 __asm("a1") = (register_t)(arg1);
  register register_t a2 __asm("a2") = (register_t)(arg2);
  register register_t a3 __asm("a3") = (register_t)(arg3);
  register register_t a4 __asm("a4") = (register_t)(arg4);
  register register_t a6 __asm("a6") = (register_t)(func);
  register register_t a7 __asm("a7") = (register_t)(ext);

  __asm __volatile("ecall"
                   : "+r"(a0), "+r"(a1)
                   : "r"(a2), "r"(a3), "r"(a4), "r"(a6), "r"(a7)
                   : "memory");

  return (sbi_ret_t){
    .error = a0,
    .value = a1,
  };
}

#define SBI_CALL0(e, f) SBI_CALL5(e, f, 0, 0, 0, 0, 0)
#define SBI_CALL1(e, f, a1) SBI_CALL5(e, f, a1, 0, 0, 0, 0)
#define SBI_CALL2(e, f, a1, a2) SBI_CALL5(e, f, a1, a2, 0, 0, 0)
#define SBI_CALL3(e, f, a1, a2, a3) SBI_CALL5(e, f, a1, a2, a3, 0, 0)
#define SBI_CALL4(e, f, a1, a2, a3, a4) SBI_CALL5(e, f, a1, a2, a3, a4, 0)
#define SBI_CALL5(e, f, a1, a2, a3, a4, a5) sbi_call(e, f, a1, a2, a3, a4, a5)

/*
 * Base extension.
 */

static sbi_ret_t sbi_get_spec_version(void) {
  return (SBI_CALL0(SBI_EXT_ID_BASE, SBI_BASE_GET_SPEC_VERSION));
}

static sbi_ret_t sbi_get_impl_id(void) {
  return (SBI_CALL0(SBI_EXT_ID_BASE, SBI_BASE_GET_IMPL_ID));
}

static sbi_ret_t sbi_get_impl_version(void) {
  return (SBI_CALL0(SBI_EXT_ID_BASE, SBI_BASE_GET_IMPL_VERSION));
}

long sbi_probe_extension(unsigned long id) {
  return (SBI_CALL1(SBI_EXT_ID_BASE, SBI_BASE_PROBE_EXTENSION, id).value);
}

static sbi_ret_t sbi_get_mvendorid(void) {
  return (SBI_CALL0(SBI_EXT_ID_BASE, SBI_BASE_GET_MVENDORID));
}

static sbi_ret_t sbi_get_marchid(void) {
  return (SBI_CALL0(SBI_EXT_ID_BASE, SBI_BASE_GET_MARCHID));
}

static sbi_ret_t sbi_get_mimpid(void) {
  return (SBI_CALL0(SBI_EXT_ID_BASE, SBI_BASE_GET_MIMPID));
}

/*
 * Legacy extensions.
 */

void sbi_console_putchar(int ch) {
  if (has_console_putchar_extension)
    (void)SBI_CALL1(SBI_CONSOLE_PUTCHAR, 0, ch);
}

int sbi_console_getchar(void) {
  if (has_console_getchar_extension) {
    /*
     * XXX: The "error" is returned here because legacy SBI functions
     * continue to return their value in a0.
     */
    return (SBI_CALL0(SBI_CONSOLE_GETCHAR, 0).error);
  }
  return -1;
}

void sbi_shutdown(void) {
  if (has_shutdown_extension)
    (void)SBI_CALL0(SBI_SHUTDOWN, 0);
}

/*
 * Timer extension.
 */

void sbi_set_timer(uint64_t val) {
  sbi_ret_t ret;

  /* Use the TIME legacy replacement extension, if available. */
  if (has_time_extension) {
    ret = SBI_CALL1(SBI_EXT_ID_TIME, SBI_TIME_SET_TIMER, val);
    assert(ret.error == SBI_SUCCESS);
  } else {
    (void)SBI_CALL1(SBI_SET_TIMER, 0, val);
  }
}

/*
 * IPI extension.
 */

void sbi_send_ipi(const unsigned long *hart_mask) {
  sbi_ret_t ret;

  /* Use the IPI legacy replacement extension, if available. */
  if (has_ipi_extension) {
    ret = SBI_CALL2(SBI_EXT_ID_IPI, SBI_IPI_SEND_IPI, *hart_mask, 0);
    assert(ret.error == SBI_SUCCESS);
  } else {
    (void)SBI_CALL1(SBI_SEND_IPI, 0, (unsigned long)hart_mask);
  }
}

/*
 * RFENCE extension.
 */

void sbi_remote_fence_i(const unsigned long *hart_mask) {
  sbi_ret_t ret;

  /* Use the RFENCE legacy replacement extension, if available. */
  if (has_rfnc_extension) {
    ret = SBI_CALL2(SBI_EXT_ID_RFNC, SBI_RFNC_REMOTE_FENCE_I, *hart_mask, 0);
    assert(ret.error == SBI_SUCCESS);
  } else {
    (void)SBI_CALL1(SBI_REMOTE_FENCE_I, 0, (unsigned long)hart_mask);
  }
}

void sbi_remote_sfence_vma(const unsigned long *hart_mask, unsigned long start,
                           unsigned long size) {
  sbi_ret_t ret;

  /* Use the RFENCE legacy replacement extension, if available. */
  if (has_rfnc_extension) {
    ret = SBI_CALL4(SBI_EXT_ID_RFNC, SBI_RFNC_REMOTE_SFENCE_VMA, *hart_mask, 0,
                    start, size);
    assert(ret.error == SBI_SUCCESS);
  } else {
    (void)SBI_CALL3(SBI_REMOTE_SFENCE_VMA, 0, (unsigned long)hart_mask, start,
                    size);
  }
}

void sbi_remote_sfence_vma_asid(const unsigned long *hart_mask,
                                unsigned long start, unsigned long size,
                                unsigned long asid) {
  sbi_ret_t ret;

  /* Use the RFENCE legacy replacement extension, if available. */
  if (has_rfnc_extension) {
    ret = SBI_CALL5(SBI_EXT_ID_RFNC, SBI_RFNC_REMOTE_SFENCE_VMA_ASID,
                    *hart_mask, 0, start, size, asid);
    assert(ret.error == SBI_SUCCESS);
  } else {
    (void)SBI_CALL4(SBI_REMOTE_SFENCE_VMA_ASID, 0, (unsigned long)hart_mask,
                    start, size, asid);
  }
}

/*
 * Hart state management extension.
 */

int sbi_hsm_hart_start(unsigned long hart, unsigned long start_addr,
                       unsigned long priv) {
  sbi_ret_t ret;

  ret = SBI_CALL3(SBI_EXT_ID_HSM, SBI_HSM_HART_START, hart, start_addr, priv);
  return (ret.error != 0 ? (int)ret.error : 0);
}

void sbi_hsm_hart_stop(void) {
  (void)SBI_CALL0(SBI_EXT_ID_HSM, SBI_HSM_HART_STOP);
}

int sbi_hsm_hart_status(unsigned long hart) {
  sbi_ret_t ret;

  ret = SBI_CALL1(SBI_EXT_ID_HSM, SBI_HSM_HART_STATUS, hart);

  return (ret.error != 0 ? (int)ret.error : (int)ret.value);
}

/*
 * SBI module functions.
 */

static void sbi_print_version(void) {
  unsigned major;
  unsigned minor;

  switch (sbi_impl_id) {
    case (SBI_IMPL_ID_BBL):
      klog("SBI: Berkely Boot Loader %lu", sbi_impl_version);
      break;
    case (SBI_IMPL_ID_XVISOR):
      klog("SBI: eXtensible Versatile hypervISOR %lu", sbi_impl_version);
      break;
    case (SBI_IMPL_ID_KVM):
      klog("SBI: Kernel-based Virtual Machine %lu", sbi_impl_version);
      break;
    case (SBI_IMPL_ID_RUSTSBI):
      klog("SBI: RustSBI %lu", sbi_impl_version);
      break;
    case (SBI_IMPL_ID_DIOSIX):
      klog("SBI: Diosix %lu", sbi_impl_version);
      break;
    case (SBI_IMPL_ID_OPENSBI):
      major = sbi_impl_version >> OPENSBI_VERSION_MAJOR_OFFSET;
      minor = sbi_impl_version & OPENSBI_VERSION_MINOR_MASK;
      klog("SBI: OpenSBI v%u.%u", major, minor);
      break;
    default:
      klog("SBI: Unrecognized Implementation: %lu", sbi_impl_id);
      break;
  }

  major =
    (sbi_spec_version & SBI_SPEC_VERS_MAJOR_MASK) >> SBI_SPEC_VERS_MAJOR_OFFSET;
  minor = (sbi_spec_version & SBI_SPEC_VERS_MINOR_MASK);

  klog("SBI Specification Version: %u.%u", major, minor);
}

void sbi_init(void) {
  sbi_ret_t sret;

  /*
   * Get the spec version. For legacy SBI implementations this will
   * return an error, otherwise it is guaranteed to succeed.
   */
  sret = sbi_get_spec_version();
  if (sret.error != 0)
    panic("Legacy SBI implementation");

  /* Set the SBI implementation info. */
  sbi_spec_version = sret.value;
  sbi_impl_id = sbi_get_impl_id().value;
  sbi_impl_version = sbi_get_impl_version().value;

  /* Set the hardware implementation info. */
  mvendorid = sbi_get_mvendorid().value;
  marchid = sbi_get_marchid().value;
  mimpid = sbi_get_mimpid().value;

  klog("Machine vendor ID: 0x%lx", mvendorid);
  klog("Machine architecture ID: 0x%lx", marchid);
  klog("Machine implementation ID: 0x%lx", mimpid);

  /* Probe for legacy replacement extensions. */
  if (sbi_probe_extension(SBI_CONSOLE_PUTCHAR) != 0)
    has_console_putchar_extension = true;
  if (sbi_probe_extension(SBI_CONSOLE_GETCHAR) != 0)
    has_console_getchar_extension = true;
  if (sbi_probe_extension(SBI_SHUTDOWN) != 0)
    has_shutdown_extension = true;
  if (sbi_probe_extension(SBI_EXT_ID_TIME) != 0)
    has_time_extension = true;
  if (sbi_probe_extension(SBI_EXT_ID_IPI) != 0)
    has_ipi_extension = true;
  if (sbi_probe_extension(SBI_EXT_ID_RFNC) != 0)
    has_rfnc_extension = true;

  /*
   * Ensure the required extensions are implemented.
   */
  if (!has_time_extension && !sbi_probe_extension(SBI_SET_TIMER))
    panic("SBI doesn't implement sbi_set_timer()");
  if (!has_ipi_extension && !sbi_probe_extension(SBI_SEND_IPI))
    panic("SBI doesn't implement sbi_send_ipi()");
  if (!has_rfnc_extension && !sbi_probe_extension(SBI_REMOTE_FENCE_I))
    panic("SBI doesn't implement sbi_remote_fence_i()");
  if (!has_rfnc_extension && !sbi_probe_extension(SBI_REMOTE_SFENCE_VMA))
    panic("SBI doesn't implement sbi_remote_sfence_vma()");
  if (!has_rfnc_extension && sbi_probe_extension(SBI_REMOTE_SFENCE_VMA_ASID))
    panic("SBI doesn't implement sbi_remote_sfence_vma_asid()");
  if (!sbi_probe_extension(SBI_SHUTDOWN))
    panic("SBI doesn't implement sbi_shutdown()");

  sbi_print_version();
}
