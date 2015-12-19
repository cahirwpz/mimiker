#include <stdio.h>
#include <stdarg.h>
#include <low/_stdio.h>

int __low_snprintf (ReadWriteInfo *rw, const void *src, size_t len)
{
  int ret, maxbytes = 0;
  char *dst;
  const char *src0 = (const char *) src;

  if (len && rw->m_handle)
    {
      maxbytes = MIN ((rw->m_limit - rw->m_size)-1, len);
      ret = maxbytes;
      dst = (char*)(rw->m_handle + rw->m_size);
      while (maxbytes)
      {
        *dst++ = *src0++;
        --maxbytes;
      }
      //memcpy (rw->m_handle + rw->m_size, src, maxbytes);
      rw->m_size += ret;
    }

  return len;
}

/* This custom snprintf does not use any floating point calculations,
   and can only format integer numbers. */
int snprintf (char *str, size_t size, const char *fmt, ...)
{
    int ret;
    ReadWriteInfo rw;
    va_list ap;
    va_start(ap, fmt);
    rw.m_handle = str;
    rw.m_fnptr = __low_snprintf;
    rw.m_size = 0;
    rw.m_limit = size;
    ret = _format_parser_int(&rw, fmt, &ap);
    va_end(ap);

    /* terminate the string */
    if (rw.m_handle)
      *((char *)rw.m_handle + rw.m_size) = '\0';
    return ret;
}
