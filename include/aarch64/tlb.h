#ifndef _AARCH64_TLB_H_
#define _AARCH64_TLB_H_

#ifndef _MACHDEP
#error "Do not use this header file outside kernel machine dependent code!"
#endif

#include <aarch64/pte.h>

void tlb_invalidate(vaddr_t va, asid_t asid);
void tlb_invalidate_asid(asid_t asid);

#endif /* !_AARCH64_TLB_H_ */
