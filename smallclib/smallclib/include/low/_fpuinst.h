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
*                 file : $RCSfile: _fpuinst.h,v $ 
*               author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* Absolute value of double */
#if defined(__MATH_IEEE754_2008__) && ! defined(__MATH_SOFT_FLOAT__)
#define __inst_abs_D(o,i)   __asm__ ("abs.D %0, %1" : "=f" (o) : "f" (i));
#else /* __MATH_IEEE754_2008__ and !__MATH_SOFT_FLOAT__ */
/* Legacy soft-float version */
#define __inst_abs_D(o,i)   			\
  do {						\
    __uint32_t hi; (o)=(i);			\
    GET_HIGH_WORD (hi, (i));			\
    hi = hi & 0x7fffffff;			\
    SET_HIGH_WORD ((o), hi);			\
  } while (0);
#endif  /* __MATH_IEEE754_2008__ and !__MATH_SOFT_FLOAT__ */

/* Absolute value of float */
#if defined(__MATH_IEEE754_2008__) && ! defined(__MATH_SOFT_FLOAT__)
#define __inst_abs_S(o,i)   __asm__ ("abs.S %0, %1" : "=f" (o) : "f" (i));
#else  /* __MATH_IEEE754_2008__ and !__MATH_SOFT_FLOAT__ */
/* Legacy/soft-float version */
#define __inst_abs_S(o,i)   	\
  do {				\
  __uint32_t hi;		\
  GET_FLOAT_WORD (hi, i);	\
  hi = hi & 0x7fffffff;		\
  SET_FLOAT_WORD (o, hi);	\
  } while (0);
#endif /* __MATH_IEEE754_2008__ and !__MATH_SOFT_FLOAT__ */

/* Set flag if value is infinity */
#define __inst_check_nan(flag,value)	__asm__("c.un.D %1,%1\n"	\
						"bc1t end\n"		\
						"li %0,1\n end:\n" :	\
						"=r" (flag):		\
						"f" (value));

/* Instructions to load upper-16 bit hex pattern to float register, 
remaining bits set to 0 */
#if defined(__mips16) && defined(__mips_soft_float) 
#define __inst_ldi_S_H(o,bits)	SET_FLOAT_WORD(o,bits<<16);
#else
#define __inst_ldi_S_H(o,bits)	{__uint32_t __tmp;			\
		__asm__ ("lui %0, %1\n": "=r"(__tmp): "K" (bits));	\
		SET_FLOAT_WORD(o,__tmp);				}
#endif


/* Instructions to load upper-16 bit hex pattern to double register, 
remaining bits set to 0 */
#if !defined(__mips16)
#define __inst_ldi_D_H(o,bits)	__asm__ ("mtc1 $0, %0\n\t"		\
					 "mthc1 %1, %0" :		\
					 "=f" (o): "r" (bits));
#else /* MIPS32R2 or MIPS64R2 */
#define __inst_ldi_D_H(o,bits)	INSERT_WORDS(o,bits,0);
#endif  /* MIPS32R2 or MIPS64R2 */

/* Instructions to load integer-values < 2^16 in to float/double register */
#define __inst_ldi_D_W(o,i)	__asm__ ("cvt.D.W %0, %1" :	\
					 "=f" (o): "f"(i));

#define __inst_ldi_S_W(o,i)	__asm__ ("cvt.S.W %0, %1" : 	\
					 "=f" (o) : 		\
					 "f" (i));

/* Square-root */
#define __inst_sqrt_S(o,i)	__asm__ ("sqrt.S %0, %1" : 	\
					 "=f" (o) : 		\
					 "f" (i));

#define __inst_sqrt_D(o,i)	__asm__ ("sqrt.D %0, %1" : 	\
					 "=f" (o) : 		\
					 "f" (i));


#define __inst_fenr_save_reset(save) __asm__ ("cfc1 %0, $28\n"		\
					      "ctc1 $0, $28\n":	\
					      "=r" (save));

#define __inst_fpstatus_read(save) __asm__ ("cfc1 %0, $31\n":	\
					  "=r" (save));

#define __inst_fenr_restore(saved) __asm__ ("ctc1 %0, $28\n":	\
					    :		\
					    "r" (saved));


#define __inst_fexr_save_reset(save) __asm__ ("cfc1 %0, $26\n"		\
					      "ctc1 $0, $26\n" :	\
					      "=r" (save));


#define __inst_fexr_save(save) __asm__ ("cfc1 %0, $26\n":	\
					      "=r" (save));


/* Execute specified double-precision floating point instruction with 
exceptions disabled and return the flag state for post-op analysis.
Steps:
	1. Save existing exception-enabled state
	2. Disable all exceptions
	3. Execute instruction
	4. Get cause & flag stage
	5. Must reset flags, else exception will trigger on restore 
	6. Restore old exception-enabled state	
	7. Convert output to double float */
#define __inst_conv_D(inst,op,in,flags) {				\
		__uint32_t __state;					\
		__asm__ ("cfc1 %2, $28\n\t"				\
			 "ctc1 $0, $28\n\t"				\
			 inst ".L.D %0, %3\n\t"				\
			 "cfc1 %1, $26\n\t"				\
			 "ctc1 $0, $26\n\t"				\
			 "ctc1 %2, $28" :				\
			 "=f" (op), "=r" (flags), "=r" (__state) :	\
			 "f" (in));					\
		__inst_conv_D_L(op,op);					\
	}

#define __inst_floor_D(op,in,flags) 	__inst_conv_D("floor",op,in,flags)
#define __inst_ceil_D(op,in,flags) 	__inst_conv_D("ceil",op,in,flags)
#define __inst_round_D(op,in,flags) 	__inst_conv_D("round",op,in,flags)
#define __inst_trunc_D(op,in,flags) 	__inst_conv_D("trunc",op,in,flags)

/* Convert double to nearest integer in current rounding mode, preserve
   in-exact exception. Target format in 'from', intermediate format in 
'thru'. Steps:
	1. Save existing exception-enabled state
	2. Disable all exceptions except inexact, preserve rounding mode
	3. Set new state with inexact and rounding-mode 'as-they-were' 
	4. Execute instruction
	5. Get cause & flag bits
	6. Reset all cause bits, except inexact
	7. Save cause field so that no other exceptions can be triggered
	8. Restore old exception-enabled state	
	9. Convert output to target format */
#define __inst_rint_fmt(from,thru,op,in,flags) {__uint32_t __state, __tmp; \
    __asm__ ("cfc1 %2, $28\n\t"						\
	     "andi %3, %2, 0x83\n\t"					\
	     "ctc1 %3, $28\n\t"						\
	     "cvt" thru from " %0, %4\n\t"				\
	     "cfc1 %1, $26\n\t"						\
	     "andi %3, %1, 0x1000\n\t"					\
	     "ctc1 %3, $26\n\t"						\
	     "ctc1 %2, $28" :						\
	     "=f" (op), "=r" (flags), "=r" (__state), "=r" (__tmp) :	\
	     "f" (in));							\
    __inst_conv_gen(from thru, op,op);			       		}

#define __inst_rint_D(op,in,flags) __inst_rint_fmt(".D",".L",op,in,flags);
#define __inst_rint_S(op,in,flags) __inst_rint_fmt(".S",".W",op,in,flags);


/* Convert to nearest integer in current rounding mode, no exceptions
   Target format in 'from', intermediate format in 'thru'
Steps:
	1. Save existing exception-enabled & rounding state
	2. Disable all exceptions, preserve rounding mode
	3. Set new state with old rounding-mode
	4. Execute instruction
	5. Get cause & flag bits
	6. Reset all cause and flag bits so that no exception can be triggered
	7. Restore old exception-enabled state	
	8. Convert output to target format */
#define __inst_nearint_fmt(from,thru,op,in,flags) {			\
    __uint32_t __state, __tmp;						\
    __asm__ ("cfc1 %2, $28\n\t"						\
	     "andi %3, %2, 0x3\n\t"					\
	     "ctc1 %3, $28\n\t"						\
	     "cvt" thru from " %0, %4\n\t"				\
	     "cfc1 %1, $26\n\t"						\
	     "ctc1 $0, $26\n\t"						\
	     "ctc1 %2, $28" :						\
	     "=f" (op), "=r" (flags), "=r" (__state), "=r"(__tmp) :	\
	     "f" (in));							\
    __inst_conv_gen(from thru, op,op);  				}

#define __inst_nearint_D(op,in,flags) __inst_nearint_fmt(".D",".L",op,in,flags);
#define __inst_nearint_S(op,in,flags) __inst_nearint_fmt(".S",".W",op,in,flags);

/* Execute specified single-precision floating point instruction with 
exceptions disabled and return the flag state for post-op analysis
Steps:
	1. Save existing exception-enabled state
	2. Disable all exceptions
	3. Execute instruction
	4. Get cause & flag stage
	5. Must reset flags, else exception will trigger on restore 
	6. Restore old exception-enabled state	
	7. Convert output to double float */
#define __inst_conv_S(inst,op,in,flags) {				\
		__uint32_t __state;					\
		__asm__ ("cfc1 %2, $28\n\t"				\
			 "ctc1 $0, $28\n\t"				\
			 inst ".W.S %0, %3\n\t"				\
			 "cfc1 %1, $26\n\t"				\
			 "ctc1 $0, $26\n\t"				\
			 "ctc1 %2, $28\n\t"	:			\
			 "=f" (op), "=r" (flags), "=r" (__state) :	\
			 "f" (in));					\
		__inst_conv_S_W(op,op);					\
	}

#define __inst_floor_S(op,in,flags) 	__inst_conv_S("floor",op,in,flags)
#define __inst_ceil_S(op,in,flags) 	__inst_conv_S("ceil",op,in,flags)
#define __inst_round_S(op,in,flags) 	__inst_conv_S("round",op,in,flags)
#define __inst_trunc_S(op,in,flags) 	__inst_conv_S("trunc",op,in,flags)

/* Convert float/double to long/word fixed point value */
#define __inst_conv_gen(trans,op,in)	__asm__ ("cvt" trans " %0, %1" :	\
					 "=f" (op):		\
					 "f" (in));

#define __inst_conv_W_D(op,in)	__inst_conv_gen(".W.D",op,in)
#define __inst_conv_W_S(op,in)	__inst_conv_gen(".W.S",op,in)
#define __inst_conv_L_D(op,in)	__inst_conv_gen(".L.D",op,in)
#define __inst_conv_L_S(op,in)	__inst_conv_gen(".L.S",op,in)
#define __inst_conv_D_W(op,in)	__inst_conv_gen(".D.W",op,in)
#define __inst_conv_S_W(op,in)	__inst_conv_gen(".S.W",op,in)
#define __inst_conv_D_L(op,in)	__inst_conv_gen(".D.L",op,in)
#define __inst_conv_S_L(op,in)	__inst_conv_gen(".S.L",op,in)


/* Convert float/double to long/word fixed point value and save
   a snapshot of FPU state to analyze for exceptions */
#define __inst_conv_W_D_state(op,in,state)	__asm__ ("cvt.W.D %0, %2\n\t" \
							 "cfc1 %1, $31": \
							 "=f" (op), "=r" (state) : \
							 "f" (in));
#define __inst_conv_W_S_state(op,in,state)	__asm__ ("cvt.W.S %0, %2\n\t" \
							 "cfc1 %1, $31": \
							 "=f" (op), "=r" (state) : \
							 "f" (in));
#define __inst_conv_L_D_state(op,in,state)	__asm__ ("cvt.L.D %0, %2\n\t" \
							 "cfc1 %1, $31": \
							 "=f" (op), "=r" (state) : \
							 "f" (in));
#define __inst_conv_L_S_state(op,in,state)	__asm__ ("cvt.L.S %0, %2\n\t" \
							 "cfc1 %1, $31": \
							 "=f" (op), "=r" (state) : \
							 "f" (in));


/* Round float/double to long/word fixed point value */
#define __inst_round_W_D(op,in)	__asm__ ("round.W.D %0, %1" :	\
					 "=f" (op):		\
					 "f" (in));

#define __inst_round_W_S(op,in)	__asm__ ("round.W.S %0, %1" :	\
					 "=f" (op) :		\
					 "f" (in));

#define __inst_round_L_D(op,in)	__asm__ ("round.L.D %0, %1":	\
					 "=f" (op):		\
					 "f" (in));

#define __inst_round_L_S(op,in)	__asm__ ("round.L.S %0, %1":	\
					 "=f" (op):		\
					 "f" (in));

/* Round float/double to long/word fixed point value and save
   a snapshot of FPU state to analyze for exceptions */
#define __inst_round_W_D_state(op,in,state)	__asm__ ("round.W.D %0, %2\n\t" \
							 "cfc1 %1, $31": \
							 "=f" (op), "=r" (state) : \
							 "f" (in));
#define __inst_round_W_S_state(op,in,state)	__asm__ ("round.W.S %0, %2\n\t" \
							 "cfc1 %1, $31": \
							 "=f" (op), "=r" (state) : \
							 "f" (in));
#define __inst_round_L_D_state(op,in,state)	__asm__ ("round.L.D %0, %2\n\t" \
							 "cfc1 %1, $31": \
							 "=f" (op), "=r" (state) : \
							 "f" (in));
#define __inst_round_L_S_state(op,in,state)	__asm__ ("round.L.S %0, %2\n\t" \
							 "cfc1 %1, $31": \
							 "=f" (op), "=r" (state) : \
							 "f" (in));

#ifdef __HW_SET_BITS__
#define __inst_merge_signbit(op,in)  __asm ("ins %0, %1, 0, 31" :	\
					    "+r" (op): "r" (in));
#else /* __HW_SET_BITS__ */
#define __inst_merge_signbit(op,in)  op=(op>>31)|(in<<1); \
	op=(op>>1)|(op<<31);
#endif /* __HW_SET_BITS__ */

#ifdef  __HW_SET_BITS__
#define __inst_extract_signbit(op)  __asm ("ins %0, $0, 0, 31" :	\
					    "+r" (op));
#else /* __HW_SET_BITS__ */
#define __inst_extract_signbit(op)  op=(op>>31)<<31;
#endif /* __HW_SET_BITS__ */



/* Copy sign of double to double 
Injects non-sign bits of wo into wi */
#define __inst_copysign_D(op,in) { __uint32_t __wo,__wi;		\
		GET_HIGH_WORD (__wo,op); GET_HIGH_WORD (__wi,in);	\
		__inst_merge_signbit(__wi,__wo);			\
		SET_HIGH_WORD (op,__wi); 				}

/* Copy sign of float to float */
#define __inst_copysign_S(op,in) { __uint32_t __wo,__wi;		\
		GET_FLOAT_WORD (__wo,op); GET_FLOAT_WORD (__wi,in);	\
		__inst_merge_signbit(__wi,__wo);			\
		SET_FLOAT_WORD (op,__wi); 				}

/* Generate double zero with sign of input */	
#define __inst_copysign_zero_D(op,in) { __uint32_t __wi;	\
    GET_HIGH_WORD (__wi,in);					\
    __inst_extract_signbit(__wi);				\
    INSERT_WORDS (op,__wi,0);					}

#define __inst_copysign_zero_S(op,in) { __uint32_t __wi;	\
    GET_FLOAT_WORD (__wi,in);					\
    __inst_extract_signbit(__wi);				\
    SET_FLOAT_WORD (op,__wi);					}

/* Set sign of double based on 32-bit input, clobber the input register
   in the process */
#define __inst_copysign_Dx(op,win) {__uint32_t __wo;	\
		GET_HIGH_WORD (__wo,op);		\
		__inst_merge_signbit(win,__wo);		\
		SET_HIGH_WORD (op,win);			}

/* Set sign of float based on 32-bit input, clobber the input register
   in the process */
#define __inst_copysign_Sx(op,win) {__uint32_t __wo;	\
		GET_FLOAT_WORD (__wo,op);		\
		__inst_merge_signbit(win,__wo);		\
		SET_FLOAT_WORD (op,win);		}

/* Provide primitive rounding operation without correct range checks
   or exception masking. The rounded number is returned as unsigned
   integer and float */
#define __inst_round_S_prim(op,fop,in) {			\
	__asm__("round.W.S %0, %1\n\t" :			\
		"=f" (fop) : "f" (in));				\
	GET_FLOAT_WORD (op,fop);				\
	__inst_conv_S_W (fop,fop); 				}

/* Provide primitive rounding operation without correct range checks
   or exception masking. The rounded number is returned as long
   unsigned integer and double */
#define __inst_round_D_prim(op,fop,in) {			\
	__asm__("round.L.D %0, %1\n\t" :			\
		"=f" (fop) : "f" (in));				\
	EXTRACT_LLWORD (op,fop);				\
	__inst_conv_D_L (fop,fop); 				}

/* Toggle sign of double based on sign-bit input. Lower 31-bits of
input must be 0 */   
#define __inst_togglesign_D(op,win) {__uint32_t __wo;	\
		GET_HIGH_WORD (__wo,op);		\
		__wo=win^__wo;				\
		SET_HIGH_WORD (op,__wo);		}

/* Toggle sign of float based on sign-bit input. Lower 31-bits of
input must be 0 */   
#define __inst_togglesign_S(op,win) {__uint32_t __wo;	\
		GET_FLOAT_WORD (__wo,op);		\
		__wo=win^__wo;				\
		SET_FLOAT_WORD (op,__wo); 		}

/* Single-instruction equivalent of SET_LOW_WORD(fr,0) */
#define __inst_reset_low_D(op)  \
	__asm__ ("mtc1 $0, %1": "+f" (op)) ;
