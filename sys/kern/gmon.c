#include <sys/param.h>
#include <sys/interrupt.h>
#include <sys/klog.h>
#include <sys/kmem.h>
#include <sys/gmon.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mimiker.h>
#include <machine/vm_param.h>
#include <sys/types.h>

gmonparam_t _gmonparam = {.state = GMON_PROF_NOT_INIT};
gmonhdr_t _gmonhdr = {.profrate = CLK_TCK};
/* The macros description are provided in gmon.h */
void init_prof(void) {
#if KGPROF == 0
  return;
#endif
  void *profptr;
  gmonparam_t *p = &_gmonparam;

  p->lowpc = rounddown((unsigned long)__kernel_start, INSTR_GRANULARITY);
  p->highpc = roundup((unsigned long)__etext, INSTR_GRANULARITY);
  p->textsize = p->highpc - p->lowpc;
  p->kcountsize = p->textsize / HISTFRACTION;
  p->hashfraction = HASHFRACTION;
  p->fromssize = p->textsize / HASHFRACTION;
  p->tolimit = (p->textsize * ARCDENSITY) / 100;
  if (p->tolimit < MINARCS)
    p->tolimit = MINARCS;
  else if (p->tolimit > MAXARCS)
    p->tolimit = MAXARCS;
  p->tossize = p->tolimit * sizeof(tostruct_t);

  int size = p->kcountsize + p->tossize + p->fromssize;
  int aligned_size = align(size, PAGESIZE);
  profptr = kmem_alloc(aligned_size, M_NOWAIT | M_ZERO);
  if (profptr == NULL) {
    klog("Not enough memory for profiling!\n");
    return;
  }
  assert(is_aligned(profptr, alignof(tostruct_t)));
  p->tos = (tostruct_t *)profptr;
  profptr += p->tossize;
  assert(is_aligned(profptr, alignof(u_short)));
  p->kcount = (u_short *)profptr;
  profptr += p->kcountsize;
  assert(is_aligned(profptr, alignof(u_short)));
  p->froms = (u_short *)profptr;
  p->state = GMON_PROF_ON;

  gmonhdr_t *hdr = &_gmonhdr;
  hdr->lpc = p->lowpc;
  hdr->hpc = p->highpc;
  hdr->ncnt = p->kcountsize + sizeof(gmonhdr_t);
  hdr->version = GMONVERSION;
  hdr->spare[0] = hdr->spare[1] = hdr->spare[2] = 0;
}