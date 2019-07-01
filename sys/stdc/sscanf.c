#include <stdarg.h>
#include <low/_stdio.h>

int __low_sscanf(ReadWriteInfo *rw)
{
    char *t = rw->m_handle;
    
    if (t[rw->m_size] == 0)
      return -1;

    return t[rw->m_size++];
}

int sscanf ( const char *str, const char * fmt, ...)
{
    int ret;
    ReadWriteInfo rw;
    va_list ap;
    va_start(ap, fmt);

    rw.m_handle = (void *)str;
    rw.m_fnptr = __low_sscanf;
    rw.m_size = 0;

    ret = __scanf_core_int (&rw, fmt, ap, 0);
    va_end(ap);
    return ret;
}
