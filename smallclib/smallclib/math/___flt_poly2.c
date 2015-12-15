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
* 		  file : $RCSfile: ___flt_poly2.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/*****************************************************************************//**
Common subroutine for polynomial expansion
Polynomials of the form
	      p = x*(P0 + x* (P1 + x*.......(P(n-1)+ x*Pn)))
	      q = x*(Q0 + x* (Q1 + x*.......(Q(n-1)+ x*Qn)))
	      r = p/(1.0+q)
	      Returns r.

	      Coefficients in calling function should be stored in an array in
	      the following sequence

	      coeff = {Pn,Qn, P(n-1), .... , P1, P0, Q(n-1), ....., Q1, Q0}
********************************************************************************/

float ___poly2f (float x, int num_coeff1, int num_coefftotal, 
		 const float *coeff, int flag)
{
    int i;
    float p = coeff[0], q = coeff[1];

    for (i = 2; i < num_coefftotal; i++) {
	if (i < num_coeff1)
	    p = p*x + coeff[i];
	else
	    q = q*x + coeff[i];
    }

    if (flag)
	p*=x;

    q = q*x + 1.0f;

    return p/q;
}
