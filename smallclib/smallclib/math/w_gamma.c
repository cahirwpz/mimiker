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
*              file : $RCSfile: w_gamma.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* @(#)w_gamma.c 5.1 93/09/24 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice 
 * is preserved.
 * ====================================================
 *
 */

/* BUG:  FIXME?
     According to Linux man pages for tgamma, lgamma, and gamma, the gamma
function was originally defined in BSD as implemented here--the log of the gamma
function.  BSD 4.3 changed the name to lgamma, apparently removing gamma.  BSD
4.4 re-introduced the gamma name with the more intuitive, without logarithm,
plain gamma function.  The C99 standard apparently wanted to avoid a problem
with the poorly-named earlier gamma and used tgamma when adding a plain
gamma function.
     So the current gamma is matching an old, bad definition, and not
matching a newer, better definition.  */
/*
FUNCTION
        <<gamma>>, <<gammaf>>, <<lgamma>>, <<lgammaf>>, <<gamma_r>>, <<gammaf_r>>, <<lgamma_r>>, <<lgammaf_r>>, <<tgamma>>, and <<tgammaf>>--logarithmic and plain gamma functions

INDEX
gamma
INDEX
gammaf
INDEX
lgamma
INDEX
lgammaf
INDEX
gamma_r
INDEX
gammaf_r
INDEX
lgamma_r
INDEX
lgammaf_r
INDEX
tgamma
INDEX
tgammaf

ANSI_SYNOPSIS
#include <math.h>
double gamma(double <[x]>);
float gammaf(float <[x]>);
double lgamma(double <[x]>);
float lgammaf(float <[x]>);
double gamma_r(double <[x]>, int *<[signgamp]>);
float gammaf_r(float <[x]>, int *<[signgamp]>);
double lgamma_r(double <[x]>, int *<[signgamp]>);
float lgammaf_r(float <[x]>, int *<[signgamp]>);
double tgamma(double <[x]>);
float tgammaf(float <[x]>);

TRAD_SYNOPSIS
#include <math.h>
double gamma(<[x]>)
double <[x]>;
float gammaf(<[x]>)
float <[x]>;
double lgamma(<[x]>)
double <[x]>;
float lgammaf(<[x]>)
float <[x]>;
double gamma_r(<[x]>, <[signgamp]>)
double <[x]>;
int <[signgamp]>;
float gammaf_r(<[x]>, <[signgamp]>)
float <[x]>;
int <[signgamp]>;
double lgamma_r(<[x]>, <[signgamp]>)
double <[x]>;
int <[signgamp]>;
float lgammaf_r(<[x]>, <[signgamp]>)
float <[x]>;
int <[signgamp]>;
double tgamma(<[x]>)
double <[x]>;
float tgammaf(<[x]>)
float <[x]>;

DESCRIPTION
<<gamma>> calculates
@tex
$\mit ln\bigl(\Gamma(x)\bigr)$,
@end tex
the natural logarithm of the gamma function of <[x]>.  The gamma function
(<<exp(gamma(<[x]>))>>) is a generalization of factorial, and retains
the property that
@ifnottex
<<exp(gamma(N))>> is equivalent to <<N*exp(gamma(N-1))>>.
@end ifnottex
@tex
$\mit \Gamma(N)\equiv N\times\Gamma(N-1)$.
@end tex
Accordingly, the results of the gamma function itself grow very
quickly.  <<gamma>> is defined as
@tex
$\mit ln\bigl(\Gamma(x)\bigr)$ rather than simply $\mit \Gamma(x)$
@end tex
@ifnottex
the natural log of the gamma function, rather than the gamma function
itself,
@end ifnottex
to extend the useful range of results representable.

The sign of the result is returned in the global variable <<signgam>>,
which is declared in math.h.

<<gammaf>> performs the same calculation as <<gamma>>, but uses and
returns <<float>> values.

<<lgamma>> and <<lgammaf>> are alternate names for <<gamma>> and
<<gammaf>>.  The use of <<lgamma>> instead of <<gamma>> is a reminder
that these functions compute the log of the gamma function, rather
than the gamma function itself.

The functions <<gamma_r>>, <<gammaf_r>>, <<lgamma_r>>, and
<<lgammaf_r>> are just like <<gamma>>, <<gammaf>>, <<lgamma>>, and
<<lgammaf>>, respectively, but take an additional argument.  This
additional argument is a pointer to an integer.  This additional
argument is used to return the sign of the result, and the global
variable <<signgam>> is not used.  These functions may be used for
reentrant calls (but they will still set the global variable <<errno>>
if an error occurs).

<<tgamma>> and <<tgammaf>> are the "true gamma" functions, returning
@tex
$\mit \Gamma(x)$,
@end tex
the gamma function of <[x]>--without a logarithm.
(They are apparently so named because of the prior existence of the old,
poorly-named <<gamma>> functions which returned the log of gamma up
through BSD 4.2.)

RETURNS
Normally, the computed result is returned.

When <[x]> is a nonpositive integer, <<gamma>> returns <<HUGE_VAL>>
and <<errno>> is set to <<EDOM>>.  If the result overflows, <<gamma>>
returns <<HUGE_VAL>> and <<errno>> is set to <<ERANGE>>.

You can modify this error treatment using <<matherr>>.

PORTABILITY
Neither <<gamma>> nor <<gammaf>> is ANSI C.  It is better not to use either
of these; use <<lgamma>> or <<tgamma>> instead.@*
<<lgamma>>, <<lgammaf>>, <<tgamma>>, and <<tgammaf>> are nominally C standard
in terms of the base return values, although the <<matherr>> error-handling
is not standard, nor is the <[signgam]> global for <<lgamma>>.
*/

/* double gamma(double x)
 * Return the logarithm of the Gamma function of x.
 *
 * Method: call gamma_r
 */

#include "low/_math.h"
#include <reent.h>
#include <errno.h>

#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
	double gamma(double x)
#else
	double gamma(x)
	double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_gamma_r(x,&(_REENT_SIGNGAM(_REENT)));
#else
        double y;
        y = __ieee754_gamma_r(x,&(_REENT_SIGNGAM(_REENT)));
        if(!finite(y)&&finite(x)) {
#ifndef HUGE_VAL 
#define HUGE_VAL inf
	    double inf = 0.0;
	    SET_HIGH_WORD(inf,0x7ff00000);	/* set inf to infinite */
#endif
 	    if(floor(x)==x&&x<=0.0) {
		/* gamma(-integer) or gamma(0) */
		  errno = EDOM;
            } else {
		/* gamma(finite) overflow */
		  errno = ERANGE;
            }
	    return HUGE_VAL;
        } else
            return y;
#endif
}             

#endif /* defined(_DOUBLE_IS_32BITS) */
