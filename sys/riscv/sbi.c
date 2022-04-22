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

/*
 * MODIFIED
 *
 * We make the following assumptions based on OpenSBI v1.0-44-g9cd95e1:
 *  - the TIMER, IPI, RFENCE, HSM, and LEGACY extensions are always supported,
 *  - the SRST extension support is platform-dependent.
 *
 * If the newest OpenSBI version introduces any changes to the aforementioned
 * assumptions, the code should be appropriately modified.
 */

#include <sys/klog.h>
#include <sys/mimiker.h>
#include <riscv/sbi.h>

/* SBI Implementation-Specific Definitions */
#define OPENSBI_VERSION_MAJOR_OFFSET 16
#define OPENSBI_VERSION_MINOR_MASK 0xFFFF

typedef struct sbi_ret {
  long error;
  long value;
} sbi_ret_t;

/* SBI implementation info. */
static u_long sbi_spec_version;
static u_long sbi_impl_id;
static u_long sbi_impl_version;

static bool has_srst_extension = false;

/* Hardware implementation info. */
static u_long mvendorid; /* CPU's JEDEC vendor ID */
static u_long marchid;
static u_long mimpid;

static inline sbi_ret_t sbi_call(u_long ext, u_long func, u_long arg0,
                                 u_long arg1, u_long arg2, u_long arg3,
                                 u_long arg4) {
  u_long error, value;
  __asm __volatile("mv a0, %2\n\t"
                   "mv a1, %3\n\t"
                   "mv a2, %4\n\t"
                   "mv a3, %5\n\t"
                   "mv a4, %6\n\t"
                   "mv a6, %7\n\t"
                   "mv a7, %8\n\t"
                   "ecall\n\t"
                   "mv %0, a0\n\t"
                   "mv %1, a1"
                   : "=r"(error), "=r"(value)
                   : "r"(arg0), "r"(arg1), "r"(arg2), "r"(arg3), "r"(arg4),
                     "r"(func), "r"(ext)
                   : "a0", "a1", "a2", "a3", "a4", "a6", "a7", "memory");
  return (sbi_ret_t){
    .error = error,
    .value = value,
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

long sbi_probe_extension(long id) {
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
 * Timer extension.
 */

int sbi_set_timer(uint64_t val) {
  sbi_ret_t ret = SBI_CALL1(SBI_EXT_ID_TIME, SBI_TIME_SET_TIMER, val);
  return (int)ret.error;
}

/*
 * IPI extension.
 */

/* NOTE: this request cannot fail. */
void sbi_send_ipi(const u_long *hart_mask) {
  (void)SBI_CALL2(SBI_EXT_ID_IPI, SBI_IPI_SEND_IPI, *hart_mask, 0);
}

/*
 * RFENCE extension.
 */

/* NOTE: this request cannot fail. */
void sbi_remote_fence_i(const u_long *hart_mask) {
  (void)SBI_CALL2(SBI_EXT_ID_RFNC, SBI_RFNC_REMOTE_FENCE_I, *hart_mask, 0);
}

int sbi_remote_sfence_vma(const u_long *hart_mask, u_long start, u_long size) {
  sbi_ret_t ret = SBI_CALL4(SBI_EXT_ID_RFNC, SBI_RFNC_REMOTE_SFENCE_VMA,
                            *hart_mask, 0, start, size);
  return (int)ret.error;
}

int sbi_remote_sfence_vma_asid(const u_long *hart_mask, u_long start,
                               u_long size, u_long asid) {
  sbi_ret_t ret = SBI_CALL5(SBI_EXT_ID_RFNC, SBI_RFNC_REMOTE_SFENCE_VMA_ASID,
                            *hart_mask, 0, start, size, asid);
  return (int)ret.error;
}

/*
 * Hart state management extension.
 */

int sbi_hsm_hart_start(u_long hart, u_long start_addr, u_long priv) {
  sbi_ret_t ret =
    SBI_CALL3(SBI_EXT_ID_HSM, SBI_HSM_HART_START, hart, start_addr, priv);
  return (int)ret.error;
}

int sbi_hsm_hart_stop(void) {
  sbi_ret_t ret = SBI_CALL0(SBI_EXT_ID_HSM, SBI_HSM_HART_STOP);
  return (int)ret.error;
}

int sbi_hsm_hart_status(u_long hart) {
  sbi_ret_t ret = SBI_CALL1(SBI_EXT_ID_HSM, SBI_HSM_HART_STATUS, hart);
  return (ret.error != SBI_SUCCESS ? (int)ret.error : (int)ret.value);
}

/*
 * System Reset extension.
 */

void sbi_system_reset(u_long reset_type, u_long reset_reason) {
  /* Use the SRST extension, if available. */
  if (has_srst_extension) {
    (void)SBI_CALL2(SBI_EXT_ID_SRST, SBI_SRST_SYSTEM_RESET, reset_type,
                    reset_reason);
  }
  (void)SBI_CALL0(SBI_SHUTDOWN, 0);
}

/*
 * Console legacy extensions.
 */

void sbi_console_putchar(int ch) {
  (void)SBI_CALL1(SBI_CONSOLE_PUTCHAR, 0, ch);
}

int sbi_console_getchar(void) {
  /*
   * XXX: The "error" is returned here because legacy SBI functions
   * continue to return their value in a0.
   */
  return (int)(SBI_CALL0(SBI_CONSOLE_GETCHAR, 0).error);
}

/*
 * SBI module functions.
 */

static void sbi_print_version(void) {
  u_int major;
  u_int minor;

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

void init_sbi(void) {
  sbi_ret_t sret;

  /*
   * Get the spec version. For legacy SBI implementations this will
   * return an error, otherwise it is guaranteed to succeed.
   */
  sret = sbi_get_spec_version();
  if (sret.error != SBI_SUCCESS)
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

  /* Probe for the SRST extension. */
  if (sbi_probe_extension(SBI_EXT_ID_SRST) != 0)
    has_srst_extension = true;

  /*
   * Ensure the required extensions are implemented.
   */
  if (!sbi_probe_extension(SBI_EXT_ID_TIME))
    panic("SBI doesn't implement the TIME extension");
  if (!sbi_probe_extension(SBI_EXT_ID_IPI))
    panic("SBI doesn't implement the IPI extension");
  if (!sbi_probe_extension(SBI_EXT_ID_RFNC))
    panic("SBI doesn't implement the RFNC extension");
  if (!sbi_probe_extension(SBI_EXT_ID_HSM))
    panic("SBI doesn't implement the HSM extension");
  if (!sbi_probe_extension(SBI_EXT_ID_SRST) &&
      !sbi_probe_extension(SBI_SHUTDOWN))
    panic("SBI doesn't implement a shutdown or reset extension");

  sbi_print_version();
}
