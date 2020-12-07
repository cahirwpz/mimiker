#include <sys/param.h>
#include <sys/kmem.h>
#include <sys/gmon.h>
#include <machine/vm_param.h>

gmonparam_t _gmonparam = { .state = GMON_PROF_OFF };

void init_prof(void) {
  void *profptr;
  gmonparam_t *p = &_gmonparam;

  p->lowpc = rounddown(KERNEL_SPACE_BEGIN, HISTFRACTION * sizeof(HISTFRACTION));
  /* TODO: Get the compiled kernel space end (kernel text end) */
  p->highpc = roundup(KERNEL_SPACE_END, HISTFRACTION * sizeof(HISTFRACTION));
  p->textsize = p->highpc - p->lowpc;
  p->hashfraction = HASHFRACTION;
  p->fromsize = p->textsize / HASHFRACTION;
  p->tolimit = p->textsize * ARCDENSITY / 100;
  if(p->tolimit < MINARCS)
    p->tolimit = MINARCS;
  else if(p->tolimit > MAXARCS)
    p->tolimit = MAXARCS;
  p->tossize = p->tolimit * sizeof(tostruct_t);
  
  profptr = kmem_alloc(p->tossize + p->fromssize, M_NOWAIT | M_ZERO);
  if(profptr == NULL) {
    kprintf("Not enough memory for profiling!\n")
    return;
  }
  p->tos = (tostruct_t *)profptr;
  profptr += p->tossize;
  p->froms = (u_short *)profptr;
}
