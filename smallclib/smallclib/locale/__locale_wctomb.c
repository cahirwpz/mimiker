/*******************************************************************************
 *
 * Copyright 2014-2015, Imagination Technologies Limited and/or its
 *                      affiliated group companies.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

/******************************************************************************
*                 file : $RCSfile: __locale_wctomb.c,v $ 
*               author : $Author  Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/


#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>
#include "low/_stdlib.h"

#define __state __count

int __common_wctomb (char *s, wchar_t _wchar, const char *charset, mbstate_t *state)
{
  wint_t wchar = _wchar;
  unsigned char char2 = (unsigned char)wchar;
  unsigned char char1 = (unsigned char)(wchar >> 8);

  if (s == NULL)
    return __lc_wctomb != jis ? 0 : 1;

  switch (__lc_wctomb)
    {
#ifdef _MB_EXTENDED_CHARSETS_ISO
      case iso:
      
        /* wchars <= 0x9f translate to all ISO charsets directly. */
        if (wchar >= 0xa0)
          {
            int iso_idx = __iso_8859_index (charset + 9);
            if (iso_idx >= 0)
              {
                unsigned char mb;

                if (s == NULL)
                  return 0;

                for (mb = 0; mb < 0x60; ++mb)
                  if (__iso_8859_conv[iso_idx][mb] == wchar)
                    {
                      *s = (char) (mb + 0xa0);
                      return 1;
                    }

                errno = EILSEQ;
                return -1;
              }
          }
 
        if ((size_t)wchar >= 0x100)
          {
            errno = EILSEQ;
            return -1;
          }
          
        break; /* iso */
#endif

#ifdef _MB_EXTENDED_CHARSETS_WINDOWS
      case cp:
        if (wchar >= 0x80)
          {
            int cp_idx = __cp_index (charset + 2);

            if (cp_idx >= 0)
              {
                unsigned char mb;

                if (s == NULL)
                  return 0;

                for (mb = 0; mb < 0x80; ++mb)
                  if (__cp_conv[cp_idx][mb] == wchar)
                    {
                      *s = (char) (mb + 0x80);
                      return 1;
                    }
                    
                errno = EILSEQ;
                return -1;
              }
          }

        if ((size_t)wchar >= 0x100)
          {
            errno = EILSEQ;
            return -1;
          }
          
        break; /* cp */
#endif
      case utf:
        {
          int ret = 0;

          if (sizeof (wchar_t) == 2 && state->__count == -4
              && (wchar < 0xdc00 || wchar >= 0xdfff))
            {
              /* There's a leftover lone high surrogate.  Write out the CESU-8 value
              of the surrogate and proceed to convert the given character.  Note
              to return extra 3 bytes. */
              wchar_t tmp;
              tmp = (state->__value.__wchb[0] << 16 | state->__value.__wchb[1] << 8)
                - (0x10000 >> 10 | 0xd80d);
              *s++ = 0xe0 | ((tmp & 0xf000) >> 12);
              *s++ = 0x80 | ((tmp &  0xfc0) >> 6);
              *s++ = 0x80 |  (tmp &   0x3f);
              state->__count = 0;
              ret = 3;
            }

          if (wchar <= 0x7f)
            {
              *s = wchar;
              return ret + 1;
            }

          if (wchar >= 0x80 && wchar <= 0x7ff)
            {
              *s++ = 0xc0 | ((wchar & 0x7c0) >> 6);
              *s   = 0x80 |  (wchar &  0x3f);
              return ret + 2;
            }

          if (wchar >= 0x800 && wchar <= 0xffff)
            {
              /* No UTF-16 surrogate handling in UCS-4 */
              if (sizeof (wchar_t) == 2 && wchar >= 0xd800 && wchar <= 0xdfff)
                {
                  wint_t tmp;

                  if (wchar <= 0xdbff)
                    {
                      /* First half of a surrogate pair.  Store the state and
                      return ret + 0. */
                      tmp = ((wchar & 0x3ff) << 10) + 0x10000;
                      state->__value.__wchb[0] = (tmp >> 16) & 0xff;
                      state->__value.__wchb[1] = (tmp >> 8) & 0xff;
                      state->__count = -4;
                      *s = (0xf0 | ((tmp & 0x1c0000) >> 18));
                      return ret;
                    }
                  if (state->__count == -4)
                    {
                      /* Second half of a surrogate pair.  Reconstruct the full
                      Unicode value and return the trailing three bytes of the
                      UTF-8 character. */
                      tmp = (state->__value.__wchb[0] << 16)
                            | (state->__value.__wchb[1] << 8)
                            | (wchar & 0x3ff);
                      state->__count = 0;
                      *s++ = 0xf0 | ((tmp & 0x1c0000) >> 18);
                      *s++ = 0x80 | ((tmp &  0x3f000) >> 12);
                      *s++ = 0x80 | ((tmp &    0xfc0) >> 6);
                      *s   = 0x80 |  (tmp &     0x3f);
                      return 4;
                    }
                  /* Otherwise translate into CESU-8 value. */
                }
              *s++ = 0xe0 | ((wchar & 0xf000) >> 12);
              *s++ = 0x80 | ((wchar &  0xfc0) >> 6);
              *s   = 0x80 |  (wchar &   0x3f);
              return ret + 3;
            }

          if (wchar >= 0x10000 && wchar <= 0x10ffff)
            {
              *s++ = 0xf0 | ((wchar & 0x1c0000) >> 18);
              *s++ = 0x80 | ((wchar &  0x3f000) >> 12);
              *s++ = 0x80 | ((wchar &    0xfc0) >> 6);
              *s   = 0x80 |  (wchar &     0x3f);
              return 4;
            }

          errno = EILSEQ;
          return -1;
        }/* utf */
        
      case sjis:
        {
          if (char1 != 0x00)
            {
              /* first byte is non-zero..validate multi-byte char */
              if (_issjis1(char1) && _issjis2(char2)) 
                {
                  *s++ = (char)char1;
                  *s = (char)char2;
                  return 2;
                }
              else
                {
                  errno = EILSEQ;
                  return -1;
                }
            }
          break;
        }/* sjis */
        
      case eucjp:
        if (char1 != 0x00)
          {
            /* first byte is non-zero..validate multi-byte char */
            if (_iseucjp1 (char1) && _iseucjp2 (char2)) 
              {
                *s++ = (char)char1;
                *s = (char)char2;
                return 2;
              }
            else if (_iseucjp2 (char1) && _iseucjp2 (char2 | 0x80))
              {
                *s++ = (char)0x8f;
                *s++ = (char)char1;
                *s = (char)(char2 | 0x80);
                return 3;
              }
            else
              {
                errno = EILSEQ;
                return -1;
              }
          }
          break; /* eucjp */
          
      case jis:
        {
          int cnt = 0; 

          if (char1 != 0x00)
            {
              /* first byte is non-zero..validate multi-byte char */
              if (_isjis (char1) && _isjis (char2)) 
                {
                  if (state->__state == 0)
                    {
                      /* must switch from ASCII to JIS state */
                      state->__state = 1;
                      *s++ = ESC_CHAR;
                      *s++ = '$';
                      *s++ = 'B';
                      cnt = 3;
                    }
                  *s++ = (char)char1;
                  *s = (char)char2;
                  return cnt + 2;
                }
              errno = EILSEQ;
              return -1;
            }

          if (state->__state != 0)
            {
              /* must switch from JIS to ASCII state */
              state->__state = 0;
              *s++ = ESC_CHAR;
              *s++ = '(';
              *s++ = 'B';
              cnt = 3;
            }

          *s = (char)char2;
          return cnt + 1;
        }/* jis */
        
      case ascii: 
        if ((size_t)wchar >= 0x100)
          {
            errno = EILSEQ;
            return -1;
          }
        break;
        
      default:
        break;
    }/* __lc_wctomb */

  *s = (char) wchar;
  return 1;
}/* __common_wctomb */

