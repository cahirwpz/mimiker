#include <sys/param.h>
#include <sys/prof.h>
#include <sys/interrupt.h>
#include <sys/kmem.h>
#include <sys/gmon.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/mimiker.h>
#include <machine/vm_param.h>

gmonparam_t _gmonparam = {.state = GMON_PROF_OFF};
void __cyg_profile_func_exit(void *this_fn, void *call_site) {
}
void init_prof(void) {
  void *profptr;
  gmonparam_t *p = &_gmonparam;

  p->lowpc = rounddown((unsigned long)__kernel_start,
                       HISTFRACTION * sizeof(HISTFRACTION));
  /* TODO: Get the compiled kernel space end (kernel text end) */
  p->highpc =
    roundup((unsigned long)__etext, HISTFRACTION * sizeof(HISTFRACTION));
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
    kprintf("Not enough memory for profiling!\n");
    return;
  }
  p->tos = (tostruct_t *)profptr;
  profptr += p->tossize;
  p->kcount = (u_short *)profptr;
  profptr += p->kcountsize;
  p->froms = (u_short *)profptr;
  p->state = GMON_PROF_ON;
}

_MCOUNT_DECL(void *from, void *self) {
  u_long frompc = (u_long)from, selfpc = (u_long)self;
  u_short *frompcindex;
  tostruct_t *top, *prevtop;
  gmonparam_t *p = &_gmonparam;
  long toindex;

  // compare and swap?
  if (p->state != GMON_PROF_ON)
    return;

  p->state = GMON_PROF_BUSY;
  intr_disable();

  /* TODO: Handle SMP */

  /* Checking if frompc is in range of kernel space
     - signal catchers get called from the stack */
  frompc -= p->lowpc;
  if (frompc > p->textsize)
    goto done;

  size_t index = (frompc / (p->hashfraction * sizeof(*p->froms)));
  frompcindex = &p->froms[index];
  toindex = *frompcindex;
  if (toindex == 0) {
    /*
     *	first time traversing this arc
     */
    toindex = ++p->tos[0].link;
    if (toindex >= p->tolimit) {
      /* We can not profile it any more */
      p->state = GMON_PROF_ERROR;
      goto done;
    }

    *frompcindex = (u_short)toindex;
    top = &p->tos[(size_t)toindex];
    top->selfpc = selfpc;
    top->count = 1;
    top->link = 0;
    goto done;
  }
  top = &p->tos[(size_t)toindex];
  if (top->selfpc == selfpc) {
    /*
     * arc at front of chain; usual case.
     */
    top->count++;
    goto done;
  }
  /*
   * have to go looking down chain for it.
   * top points to what we are looking at,
   * prevtop points to previous top.
   * we know it is not at the head of the chain.
   */
  while (true) {
    if (top->link == 0) {
      /*
       * top is end of the chain and none of the chain
       * had top->selfpc == selfpc.
       * so we allocate a new tostruct
       * and link it to the head of the chain.
       */
      toindex = ++p->tos[0].link;
      if (toindex >= p->tolimit) {
        p->state = GMON_PROF_ERROR;
        goto done;
      }

      top = &p->tos[(size_t)toindex];
      top->selfpc = selfpc;
      top->count = 1;
      top->link = *frompcindex;
      *frompcindex = (u_short)toindex;
      goto done;
    }
    /*
     * otherwise, check the next arc on the chain.
     */
    prevtop = top;
    top = &p->tos[top->link];
    if (top->selfpc == selfpc) {
      /*
       * there it is.
       * increment its count
       * move it to the head of the chain.
       */
      top->count++;
      toindex = prevtop->link;
      prevtop->link = top->link;
      top->link = *frompcindex;
      *frompcindex = (u_short)toindex;
      goto done;
    }
  }
done:
  if (p->state != GMON_PROF_ERROR)
    p->state = GMON_PROF_ON;
  /* TODO: End handling SMP */
  intr_enable();
  return;
}
