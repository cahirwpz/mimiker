#define KLOG KL_INIT
#include <interrupt.h>
#include <physmem.h>
#include <malloc.h>
#include <mips/cpuinfo.h>
#include <mips/malta.h>
#include <mips/exc.h>
#include <mips/intr.h>
#include <mips/timer.h>
#include <mips/tlb.h>
#include <klog.h>
#include <kbss.h>
#include <console.h>
#include <pcpu.h>
#include <pmap.h>
#include <pool.h>
#include <stdc.h>
#include <sleepq.h>
#include <rman.h>
#include <thread.h>
#include <turnstile.h>
#include <vm_map.h>
#include <syslimits.h>
#include <kenv.h>

extern int kernel_init(int argc, char **argv);
extern uint8_t __kernel_start[];

static void pm_bootstrap(unsigned memsize) {
  pm_init();

  pm_seg_t *seg = kbss_grow(pm_seg_space_needed(memsize));

  /*
   * Let's fix size of kernel bss section. We need to tell physical memory
   * allocator not to manage memory used by the kernel image along with all
   * memory allocated using \a kbss_grow.
   */
  void *__kernel_end = kbss_fix();

  /* create Malta physical memory segment */
  pm_seg_init(seg, MALTA_PHYS_SDRAM_BASE, MALTA_PHYS_SDRAM_BASE + memsize,
              MIPS_KSEG0_START);

  /* reserve kernel image and physical memory description space */
  pm_seg_reserve(seg, MIPS_KSEG0_TO_PHYS(__kernel_start),
                 MIPS_KSEG0_TO_PHYS(__kernel_end));

  pm_add_segment(seg);
}

static void thread_bootstrap(void) {
  /* Create main kernel thread */
  thread_t *td =
    thread_create("kernel-main", (void *)kernel_init, NULL, prio_uthread(255));

  exc_frame_t *kframe = td->td_kframe;
  kframe->sr |= SR_IE; /* the thread will run with interrupts enabled */
  td->td_state = TDS_RUNNING;
  PCPU_SET(curthread, td);
}

extern const uint8_t __malta_dtb_start[];

void platform_init(int argc, char **argv, char **envp, unsigned memsize) {

  kbss_init();
  setup_kenv(argc, argv, envp);
  cn_init();
  klog_init();
  pcpu_init();
  cpu_init();
  tlb_init();
  mips_intr_init();
  mips_timer_init();
  pm_bootstrap(memsize);
  pmap_init();
  pool_bootstrap();
  vm_map_init();
  kmem_bootstrap();
  sleepq_init();
  turnstile_init();
  thread_bootstrap();

  print_kenv();

  klog("Switching to 'kernel-main' thread...");
}
