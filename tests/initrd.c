#include <stdc.h>
#include <initrd.h>

int main()
{
    cpio_file_stat_t stat;
    char *tape = (char*)get_rd_start();
    do {
        memset(&stat, 0, sizeof(stat));
        fill_header(&tape, &stat);
        dump_cpio_stat(&stat);
    } while(strcmp(stat.c_name ,"TRAILER!!!") != 0);
    return 0;
}

