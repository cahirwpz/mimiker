###############################################################################
#
#  Copyright 2014-2015, Imagination Technologies Limited and/or its
#                       affiliated group companies.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#
#  1. Redistributions of source code must retain the above copyright notice,
#  this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright notice,
#  this list of conditions and the following disclaimer in the documentation
#  and/or other materials provided with the distribution.
#  3. Neither the name of the copyright holder nor the names of its
#  contributors may be used to endorse or promote products derived from this
#  software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.
#
###############################################################################

###############################################################################
#              file : $RCSfile: CMakeLists.txt,v $
#            author : $Author  Imagination Technologies Ltd
# date last revised :
#   current version :
###############################################################################

# Description   Leaf CMake file for Size-optimized C library

#cmake_minimum_required(VERSION 2.6)

cmake_minimum_required(VERSION 2.6)
enable_language(ASM)

set(PACKAGE ../mips_smallclib)

set(PRODUCT SmallCLib)

PROJECT(SmallCLib C)

set (sources_common
	argz/argz_add.c
	argz/argz_add_sep.c
	argz/argz_append.c
	argz/argz_count.c
	argz/argz_create.c
	argz/argz_create_sep.c
	argz/argz_delete.c
	argz/argz_extract.c
	argz/argz_insert.c
	argz/argz_next.c
	argz/argz_replace.c
	argz/argz_stringify.c
	argz/buf_findstr.c
	argz/envz_add.c
	argz/envz_entry.c
	argz/envz_get.c
	argz/envz_merge.c
	argz/envz_remove.c
	argz/envz_strip.c
 
	ctype/ctype_.c
	ctype/isalnum.c
	ctype/isalpha.c
	ctype/isascii.c
	ctype/isblank.c
	ctype/iscntrl.c
	ctype/isdigit.c
	ctype/isgraph.c
	ctype/islower.c
	ctype/isprint.c
	ctype/ispunct.c
	ctype/isspace.c
	ctype/isupper.c

	ctype/iswalnum.c
	ctype/iswalpha.c
	ctype/iswblank.c
	ctype/iswcntrl.c
	ctype/iswctype.c
	ctype/iswdigit.c
	ctype/iswgraph.c
	ctype/iswlower.c
	ctype/iswprint.c
	ctype/iswpunct.c
	ctype/iswspace.c
	ctype/iswupper.c
	ctype/iswxdigit.c
	ctype/isxdigit.c
	ctype/toascii.c
	ctype/tolower.c
	ctype/toupper.c
	ctype/towctrans.c
	ctype/towlower.c
	ctype/towupper.c
	ctype/wctrans.c
	ctype/wctype.c
	ctype/jp2uc.c

	errno/errno.c
#	errno/errno_var.c
#	errno/get_errno.c
#	errno/set_errno.c

	locale/fix_grouping.c
	locale/lctype.c
	locale/ldpart.c
	locale/lmessages.c
	locale/lmonetary.c
	locale/lnumeric.c
	locale/locale.c
	locale/localeconv.c
	locale/__locale_mbtowc.c
	locale/__locale_wctomb.c
	locale/nl_langinfo.c
	locale/timelocal.c

	misc/ffs.c
	misc/fini.c
	misc/setjmp.S

#	reent/gettimeofdayr.c
#	reent/reent_data.c
#	reent/timesr.c

#	reent/__epi.c
#	reent/__epi_f.c
#	reent/__epi_di.c
#	reent/__epi_si.c

#	reent/__epi_varg.c
#	reent/__get_atexit.c
#	reent/__get_sigfunc.c
#	reent/__get_signgam.c

	search/bsearch.c
	search/hash.c
	search/hash_bigkey.c
	search/hash_buf.c
	search/hash_func.c
	search/hash_log2.c
	search/hash_page.c
	search/hcreate.c
	search/qsort.c
	search/tdelete.c
	search/tdestroy.c
	search/tfind.c
	search/tsearch.c
	search/twalk.c

	signal/psignal.c
	signal/signal_data.c
	signal/sigtramp.c

	stdlib/__adjust.c
	stdlib/__exp10.c
	stdlib/__ten_mu.c
	stdlib/a64l.c
	stdlib/abs.c
	stdlib/abort.c
	stdlib/__atexit.c
	stdlib/atexit.c
	stdlib/atexit_data.c
	stdlib/atof.c
	stdlib/atoff.c
	stdlib/atoi.c
	stdlib/atol.c
	stdlib/atoll.c
	stdlib/bsearch.c
	stdlib/btowc.c
	stdlib/__call_atexit.c
	stdlib/calloc.c
	stdlib/cxa_atexit.c
	stdlib/cxa_finalize.c
	stdlib/div.c
	stdlib/drand48.c
	stdlib/dtoa.c
	stdlib/ecvtbuf.c
	stdlib/efgcvt.c
	stdlib/environ.c
	stdlib/envlock.c
	stdlib/erand48.c
	stdlib/exit.c
	stdlib/_Exit.c
	stdlib/findenv.c
	stdlib/free.c
	stdlib/gdtoa-gethex.c
	stdlib/gdtoa-hexnan.c
	stdlib/getenv.c
	stdlib/getopt.c
	stdlib/getsubopt.c
	stdlib/jrand48.c
	stdlib/ldtoa.c
	stdlib/l64a.c
	stdlib/labs.c
	stdlib/lcong48.c
	stdlib/ldiv.c
	stdlib/llabs.c
	stdlib/lldiv.c
#	stdlib/__locale_mbtowc.c
#	stdlib/__locale_wctomb.c
	stdlib/lrand48.c
	stdlib/malloc_util.c
	stdlib/malloc_util_coalesce.c
	stdlib/mblen.c
	stdlib/mbrlen.c
	stdlib/mbrtowc.c
	stdlib/mbsinit.c
	stdlib/mbstowcs.c
	stdlib/mbsrtowcs
	stdlib/mbsnrtowcs
	stdlib/mbtowc.c
	stdlib/mlock.c
	stdlib/mprec.c
	stdlib/mrand48.c
	stdlib/mstats.c
	stdlib/nrand48.c
	stdlib/on_exit.c
	stdlib/putenv.c
	stdlib/qsort.c
	stdlib/rand48.c
	stdlib/rand.c
	stdlib/rand_data.c
	stdlib/realloc.c
	stdlib/reallocf.c
#	stdlib/rand_r.c
	stdlib/sb_charsets.c
	stdlib/seed48.c
	stdlib/setenv.c
	stdlib/srand48.c
	stdlib/strtod.c
#	stdlib/strtof.c
	stdlib/strtol.c
	stdlib/strtold.c
	stdlib/strtoll.c
	stdlib/strtoul.c
	stdlib/strtoull.c
	stdlib/_strtoxl.c
	stdlib/_strtoxll.c
#	stdlib/system.c
	stdlib/wcrtomb.c
	stdlib/wcsrtombs.c
	stdlib/wcsnrtombs.c
	stdlib/wcstol.c
	stdlib/wcstold.c
	stdlib/wcstoll.c
	stdlib/wcstombs.c
	stdlib/wcstoul.c
	stdlib/wcstoull.c
	stdlib/_wcstoxl.c
	stdlib/_wcstoxll.c
	stdlib/wctob.c
	stdlib/wctomb.c
	stdlib/mbstate.c
	stdlib/wcstod.c
	stdlib/assert.c
	stdlib/system.c
#	stdlib/_mbtowc_r.c
#	stdlib/_wcstombs_r.c
#	stdlib/_wctomb_r.c
#	stdlib/__atexit_with_arg.c
#	stdlib/__register_exitproc.c
#	stdlib/__strtoxll_common_r.c
#	stdlib/__strtoxl_common_r.c
#	stdlib/__wstrtoxll_common_r.c
#	stdlib/__wstrtoxl_common_r.c
#	stdlib/__runatexitfns.c

	string/bcmp.c
	string/bcopy.c
	string/bzero.c
	string/index.c
	string/memccpy.c
	string/memchr.c
	string/memcmp.c
	string/memmove.c
	string/mempcpy.c
	string/memset.c
	string/rindex.c
	string/stpcpy.c
	string/stpncpy.c
	string/strcasecmp.c
	string/strcasestr.c
	string/strcat.c
	string/strchr.c
	string/strcmp.c
	string/strcoll.c
	string/strcpy.c
	string/strcspn.c
	string/strdup.c
	string/strerror.c
	string/strerror_r.c
	string/strlen.c
	string/strlwr.c
	string/strncasecmp.c
	string/strncat.c
	string/strncmp.c
	string/strncpy.c
	string/strnlen.c
	string/strndup.c
	string/strpbrk.c
	string/strrchr.c
	string/strsep.c
	string/strsignal.c
	string/strspn.c
	string/strstr.c
	string/strtok.c
#	string/strtok_r.c
	string/strupr.c
	string/strxfrm.c
	string/swab.c
	string/u_strerr.c
	string/wcpcpy.c
	string/wcpncpy.c
	string/wcscasecmp.c
	string/wcscat.c
	string/wcschr.c
	string/wcscmp.c
	string/wcscoll.c
	string/wcscpy.c
	string/wcscspn.c
	string/wcsdup.c
	string/wcslcat.c
	string/wcslcpy.c
	string/wcslen.c
	string/wcsncasecmp.c
	string/wcsncat.c
	string/wcsncmp.c
	string/wcsncpy.c
	string/wcsnlen.c
	string/wcspbrk.c
	string/wcsrchr.c
	string/wcsspn.c
	string/wcsstr.c
	string/wcstok.c
	string/wcsxfrm.c
	string/wcswidth.c
	string/wcwidth.c
	string/wmemchr.c
	string/wmemcmp.c
	string/wmemcpy.c
	string/wmemmove.c
	string/wmemset.c
#	string/__strtok_r.c
#	string/ffsll.c
	string/memmem.c
	string/memrchr.c
#	string/strchrnul.c
	string/strlcat.c
	string/strlcpy.c

	stdio/asiprintf.c
	stdio/asniprintf.c
	stdio/asnprintf.c
	stdio/asprintf.c
	stdio/clearerr.c
	stdio/diprintf.c
	stdio/dprintf.c
	stdio/fcloseall.c
	stdio/fclose.c
	stdio/fdopen.c
	stdio/feof.c
	stdio/ferror.c
	stdio/fflush.c
	stdio/refill.c
	stdio/fgetc.c
	stdio/fgetpos.c
	stdio/fgets.c
	stdio/fgetwc.c
	stdio/fgetws.c
	stdio/fileno.c
	stdio/findfp.c
	stdio/fiprintf.c
	stdio/fiscanf.c
	stdio/flags.c
	stdio/fmemopen.c
	stdio/fopen.c
	stdio/fopencookie.c
	stdio/__format_parser.c
	stdio/fprintf.c
	stdio/fpurge.c
	stdio/fputc.c
	stdio/fputs.c
	stdio/fputwc.c
	stdio/fputws.c
	stdio/fread.c
	stdio/freopen.c
	stdio/fscanf.c
	stdio/fseek.c
	stdio/fseeko.c
	stdio/fsetpos.c
	stdio/ftell.c
	stdio/ftello.c
	stdio/funopen.c
	stdio/fwide.c
	stdio/fwprintf.c
	stdio/fwrite.c
	stdio/fwscanf.c
	stdio/getc.c
	stdio/getchar.c
	stdio/getchar_u.c
	stdio/getc_u.c
	stdio/getdelim.c
	stdio/getline.c
	stdio/gets.c
	stdio/getw.c
	stdio/getwc.c
	stdio/getwchar.c
	stdio/iostreams.c
	stdio/ioutils.c
	stdio/ioutils_canread.c
	stdio/iprintf.c
	stdio/iscanf.c
	stdio/___low_asnprintf.c
	stdio/___low_asprintf.c
	stdio/low_close.c
	stdio/___low_fprintf.c
	stdio/___low_fscanf.c
	stdio/___low_fwprintf.c
	stdio/___low_fwscanf.c
	stdio/___low_printf.c
	stdio/low_read.c
	stdio/___low_scanf.c
	stdio/low_seek.c
	stdio/___low_snprintf.c
	stdio/___low_sprintf.c
	stdio/___low_sscanf.c
	stdio/___low_swprintf.c
	stdio/___low_swscanf.c
	stdio/___low_wprintf.c
	stdio/low_write.c
	stdio/___low_wscanf.c
	stdio/mktemp.c
	stdio/open_memstream.c
	stdio/perror.c
	stdio/printf.c
	stdio/putc.c
	stdio/putchar.c
	stdio/putchar_u.c
	stdio/putc_u.c
	stdio/puts.c
	stdio/putw.c
	stdio/putwc.c
	stdio/putwchar.c
	stdio/remove.c
	stdio/rename.c
	stdio/rewind.c
	stdio/scanf.c
	stdio/__scanf_core.c
	stdio/setbuf.c
	stdio/setbuffer.c
	stdio/setlinebuf.c
	stdio/setvbuf.c
	stdio/siprintf.c
	stdio/siscanf.c
	stdio/sniprintf.c
	stdio/snprintf.c
	stdio/sprintf.c
	stdio/sscanf.c
	stdio/swprintf.c
	stdio/swscanf.c
	stdio/tmpfile.c
	stdio/tmpnam.c
	stdio/ungetc.c
	stdio/ungetwc.c
#	stdio/unlink.c
	stdio/vasiprintf.c
	stdio/vasniprintf.c
	stdio/vasnprintf.c
	stdio/vasprintf.c
	stdio/vdiprintf.c
	stdio/vdprintf.c
	stdio/vfprintf.c
	stdio/vfscanf.c
	stdio/vfwprintf.c
	stdio/vfwscanf.c
	stdio/viprintf.c
	stdio/viscanf.c
	stdio/vprintf.c
	stdio/vscanf.c
	stdio/vsiprintf.c
	stdio/vsiscanf.c
	stdio/vsniprintf.c
	stdio/vsnprintf.c
	stdio/vsprintf.c
	stdio/vsscanf.c
	stdio/vswprintf.c
	stdio/vswscanf.c
	stdio/vwprintf.c
	stdio/vwscanf.c
	stdio/__wformat_parser.c
	stdio/wprintf.c
	stdio/wscanf.c
	stdio/__wscanf_core.c

# float int IO file list start
	stdio/__format_parser_float.c
	stdio/asnprintf_float.c
	stdio/asprintf_float.c
	stdio/dprintf_float.c
	stdio/fprintf_float.c
	stdio/printf_float.c
	stdio/snprintf_float.c
	stdio/sprintf_float.c
	stdio/vasnprintf_float.c
	stdio/vasprintf_float.c
	stdio/vdprintf_float.c
	stdio/vfprintf_float.c
	stdio/vprintf_float.c
	stdio/vsnprintf_float.c
	stdio/vsprintf_float.c

	stdio/__scanf_core_float.c
	stdio/fscanf_float.c
	stdio/scanf_float.c
	stdio/sscanf_float.c
	stdio/vfscanf_float.c
	stdio/vscanf_float.c
	stdio/vsscanf_float.c

	stdio/__format_parser_int.c
	stdio/asnprintf_int.c
	stdio/asprintf_int.c
	stdio/dprintf_int.c
	stdio/fprintf_int.c
	stdio/printf_int.c
	stdio/snprintf_int.c
	stdio/sprintf_int.c
	stdio/vasnprintf_int.c
	stdio/vasprintf_int.c
	stdio/vdprintf_int.c
	stdio/vfprintf_int.c
	stdio/vprintf_int.c
	stdio/vsnprintf_int.c
	stdio/vsprintf_int.c

	stdio/__scanf_core_int.c
	stdio/fscanf_int.c
	stdio/scanf_int.c
	stdio/sscanf_int.c
	stdio/vfscanf_int.c
	stdio/vscanf_int.c
	stdio/vsscanf_int.c

	stdio/__memcpy.c
# float int IO file list ends

	time/asctime.c
	time/clock.c
	time/ctime.c
	time/difftime.c
	time/gmtime.c
	time/langinfo.c
	time/localtime.c
	time/mktime.c
	time/strftime.c
	time/strptime.c
	time/time.c
	time/time_data.c
	time/tzset.c
	time/wcsftime.c
	time/__time_to_tm.c
	time/__tm_to_time.c

#	stdlib/calloc_szx.c
#	stdlib/free_szx.c
#	stdlib/realloc_szx.c
#	stdlib/malloc_szx.c
)

# sources imported from math for libc
set (sources_math_import
	math/sf_copysign.c
	math/sf_finite.c
	math/sf_fpclassify.c
	math/sf_frexp.c
	math/sf_isinff.c
	math/sf_isnanf.c  
	math/sf_ldexp.c
	math/sf_modf.c
	math/sf_nan.c
	math/sf_scalbn.c
	math/s_copysign.c
	math/s_finite.c
	math/s_fpclassify.c
	math/s_frexp.c
	math/s_isinfd.c
	math/s_isnand.c
	math/s_ldexp.c
	math/s_modf.c
	math/s_nan.c
	math/s_scalbn.c
)

# set sources, headers are added auto-magically by dependency generation
set (sources_sz
	string/memcpy.S
	stdlib/malloc.c
	signal/signal.c
)

set (sources_szx
	string/memcpy.c
	stdlib/malloc_tiny.c
	stdlib/getenv_nofio.c
	signal/signal_tiny.c
	stdio/ungetc_nofio.c
	stdio/perror_nofio.c
	stdio/clearerr_nofio.c
	stdio/fclose_nofio.c
	stdio/feof_nofio.c
	stdio/ferror_nofio.c
	stdio/fflush_nofio.c
	stdio/fgetc_nofio.c
	stdio/fgetpos_nofio.c
	stdio/fgets_nofio.c
	stdio/fopen_nofio.c
	stdio/fputc_nofio.c
	stdio/fputs_nofio.c
	stdio/fread_nofio.c
	stdio/freopen_nofio.c
	stdio/fseek_nofio.c
	stdio/fsetpos_nofio.c
	stdio/ftell_nofio.c
	stdio/fwrite_nofio.c
	stdio/getc_nofio.c
	stdio/getchar_nofio.c
	stdio/gets_nofio.c
	stdio/putc_nofio.c
	stdio/putchar_nofio.c
	stdio/puts_nofio.c
	stdio/remove_nofio.c
	stdio/rewind_nofio.c
	stdio/setbuf_nofio.c
	stdio/setvbuf_nofio.c
	stdio/tmpfile_nofio.c
	stdio/tmpnam_nofio.c
)

set (sources_math_common
	math/sf_acos.c
	math/sf_acosh.c
	math/sf_asin.c
	math/sf_atan2.c
	math/sf_atanh.c
	math/sf_cosh.c
	math/ef_exp.c
	math/sf_fmod.c
	math/sf_hypot.c
	math/ef_j0.c
	math/ef_j1.c
	math/ef_jn.c
	math/ef_log.c
	math/sf_pow.c
	math/sf_remainder.c
	math/ef_rem_pio2.c
	math/sf_sinh.c
	math/ef_sqrt.c
	math/erf_gamma.c
	math/erf_lgamma.c
	math/er_gamma.c
	math/er_lgamma.c
	math/s_acos.c
	math/s_acosh.c
	math/s_asin.c
	math/s_atan2.c
	math/s_atanh.c
	math/s_cosh.c
	math/e_exp.c
	math/s_fmod.c
	math/s_hypot.c
	math/e_j0.c
	math/e_j1.c
	math/e_jn.c
	math/e_log.c
	math/s_pow.c
	math/s_remainder.c
	math/e_rem_pio2.c
	math/s_sinh.c	
	math/e_sqrt.c
	math/s_fma.c
	math/sf_fma.c
	math/kf_cos.c
	math/kf_sin.c
	math/kf_tan.c
	math/k_cos.c
	math/k_sin.c
	math/k_tan.c
	math/sf_asinh.c
	math/sf_atan.c
	math/sf_ceil.c
	math/sf_cos.c
	math/sf_erf.c
	math/sf_erfcf.c
	math/erff_data.c
	math/sf_exp10.c
	math/sf_expm1.c
	math/sf_fabs.c
	math/sf_floor.c
	math/sf_infinity.c
	math/sf_log1p.c
	math/sf_log2.c
	math/sf_lround.c
	math/sf_llround.c
	math/sf_nearbyint.c
	math/sf_pow10.c
	math/sf_remquo.c
	math/sf_round.c
	math/sf_signif.c
	math/sf_sin.c
	math/sf_tan.c
	math/sf_tanh.c
	math/sf_trunc.c
	math/s_asinh.c
	math/s_atan.c
	math/s_ceil.c
	math/s_cos.c
	math/s_erf.c
	math/s_erfc.c
	math/erf_data.c
	math/s_exp10.c
	math/s_expm1.c
	math/s_fabs.c
	math/s_floor.c
	math/s_infinity.c
	math/s_log1p.c
	math/s_log2.c
	math/s_lrint.c
	math/s_llrint.c
	math/s_lround.c
	math/s_llround.c
	math/s_nearbyint.c
	math/s_pow10.c
	math/s_remquo.c
	math/s_round.c
	math/s_signbit.c
	math/s_signif.c
	math/s_sin.c
	math/s_tan.c
	math/s_tanh.c
	math/s_trunc.c
	math/wf_exp.c
	math/wf_exp2.c
	math/wf_gamma.c
	math/wf_j0.c
	math/wf_j1.c
	math/wf_jn.c
	math/wf_lgamma.c
	math/wf_log.c
	math/sf_log10.c
	math/wf_sqrt.c
	math/wf_tgamma.c
	math/wrf_gamma.c
	math/wrf_lgamma.c
	math/wr_gamma.c
	math/wr_lgamma.c
	math/w_exp.c
	math/w_exp2.c
	math/w_gamma.c
	math/w_j0.c
	math/w_j1.c
	math/w_jn.c
	math/w_lgamma.c
	math/w_log.c
	math/s_log10.c
	math/w_sqrt.c
	math/w_tgamma.c
	math/asincos_consts.c
	math/___dbl_poly2.c
	math/___flt_poly2.c

	math/sf_cbrt.c
	math/sf_fdim.c
	math/sf_fmax.c
	math/sf_fmin.c
	math/sf_ilogb.c
	math/sf_logb.c
	math/sf_lrint.c
	math/sf_llrint.c
	math/sf_nextafter.c
	math/sf_rint.c
	math/sf_scalbln.c
	math/s_cbrt.c
	math/s_fdim.c
	math/s_fmax.c
	math/s_fmin.c
	math/s_ilogb.c
	math/s_logb.c
	math/s_nextafter.c
	math/s_rint.c
	math/s_scalbln.c

	math/k_rem_pio2.c
	math/kf_rem_pio2.c
)

set (sources_mathl_common
	mathl/acoshl.c
	mathl/acosl.c
	mathl/asinhl.c
	mathl/asinl.c
	mathl/atan2l.c
	mathl/atanhl.c
	mathl/atanl.c
	mathl/cbrtl.c
	mathl/ceill.c
	mathl/copysignl.c
	mathl/coshl.c
	mathl/cosl.c
	mathl/erfcl.c
	mathl/erfl.c
	mathl/exp2l.c
	mathl/expl.c
	mathl/expm1l.c
	mathl/fabsl.c
	mathl/fdiml.c
	mathl/floorl.c
	mathl/fmal.c
	mathl/fmaxl.c
	mathl/fminl.c
	mathl/fmodl.c
	mathl/frexpl.c
	mathl/hypotl.c
	mathl/ilogbl.c
	mathl/ldexpl.c
	mathl/lgammal.c
	mathl/llrintl.c
	mathl/llroundl.c
	mathl/log10l.c
	mathl/log1pl.c
	mathl/logl.c
	mathl/lrintl.c
	mathl/lroundl.c
	mathl/modfl.c
	mathl/nanl.c
	mathl/nearbyintl.c
	mathl/nextafterl.c
	mathl/powl.c
	mathl/remainderl.c
	mathl/remquol.c
	mathl/rintl.c
	mathl/roundl.c
	mathl/scalblnl.c
	mathl/scalbnl.c
	mathl/sinhl.c
	mathl/sinl.c
	mathl/sqrtl.c
	mathl/tanhl.c
	mathl/tanl.c
	mathl/tgammal.c
	mathl/truncl.c
)

# top-level common stuff for embedded
include_directories(include)
SET(CMAKE_C_FLAGS_RELEASE "-Wall -Werror -Wno-char-subscripts")

# Prepend the default set of compiler flags
SET(CMAKE_C_FLAGS "-mno-gpopt -Os -fsection-anchors -D_COMPILING_NEWLIB -mabs=2008 ${CMAKE_C_FLAGS}")
SET(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -c")

include(toolchains/MultiLibConfig.cmake)

MULTILIB_BUILD_SETUP()

if (MULTILIB_TOP)
# Process this block only once per run, irrespective of recursive invocations
message (WARNING ": Parts of this software are derived/copied from
Newlib v2.0.0 which released under a collection of BSD-type licenses
and Musl v0.9.11 release under the MIT license. Any source/binary
distribution must include license/COPYING.NEWLIB &
license/COPYING.MUSL.  Releasing source code is not mandatory, but if
released, any copyright statement/license text in the original source
file must be preserved!  Files that are written from scratch may carry
just the Imagination Technologies Copyright statement.")
install (FILES license/COPYING.NEWLIB	license/COPYING.MUSL license/COPYING.CLIB
  DESTINATION share/copying)

else (MULTILIB_TOP)
# Process this block for each multilib invocation
STRING(REGEX REPLACE ".*mclib~(small|tiny).*" "\\1" SMALLCLIB_FLAVOUR "${MULTILIB_FLAVOUR}")
STRING(REGEX MATCH "@mips16" MIPS16_MODE "${MULTILIB_FLAVOUR}")
STRING(REGEX MATCH "@msoft-float" SOFT_FLOAT_MODE "${MULTILIB_FLAVOUR}")

if (MIPS16_MODE AND NOT SOFT_FLOAT_MODE)
set_source_files_properties(${sources_math_common} ${sources_mathl_common}
  ${sources_math_import}
  PROPERTIES 
  COMPILE_FLAGS "-mno-mips16")
endif ()

if("${SMALLCLIB_FLAVOUR}" STREQUAL "tiny")
  multilib_add_library (c STATIC ${sources_common} ${sources_szx} ${sources_math_import})
  multilib_add_library (g STATIC ${sources_common} ${sources_szx} ${sources_math_import})
else("${SMALLCLIB_FLAVOUR}" STREQUAL "tiny")
  multilib_add_library (c STATIC ${sources_common} ${sources_sz} ${sources_math_import})
  multilib_add_library (g STATIC ${sources_common} ${sources_sz} ${sources_math_import})
endif("${SMALLCLIB_FLAVOUR}" STREQUAL "tiny")

multilib_add_library (m STATIC ${sources_math_common}
  ${sources_mathl_common})

install (TARGETS ${MULTILIB_TARGET_c} ${MULTILIB_TARGET_g} ${MULTILIB_TARGET_m}
	ARCHIVE DESTINATION ${CMAKE_SYSTEM_NAME}/lib/${MULTILIB_DIR})
endif (MULTILIB_TOP)

#
# End of CMakeLists.txt
#

