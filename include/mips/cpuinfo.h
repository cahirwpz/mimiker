#ifndef __MIPS_CPUINFO_H__
#define __MIPS_CPUINFO_H__

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

void cpu_init();

#endif
