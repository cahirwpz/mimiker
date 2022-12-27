#ifndef _MIPS_CPUINFO_H_
#define _MIPS_CPUINFO_H_

#ifndef _MACHDEP
#error "Do not use this header file outside kernel machine dependent code!"
#endif

typedef struct cpuinfo {
  int tlb_entries;
  int ic_size;
  int ic_linesize;
  int ic_nways;
  int ic_nsets;
  int dc_size;
  int dc_linesize;
  int dc_nways;
  int dc_nsets;
} cpuinfo_t;

extern cpuinfo_t cpuinfo;

void init_mips_cpu(void);

#endif /* !_MIPS_CPUINFO_H_ */
