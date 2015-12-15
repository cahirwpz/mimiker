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
* 		  file : $RCSfile: asincos_consts.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

const double
  one =  1.00000000000000000000e+00, /* 0x3FF00000, 0x00000000 */
  huge =  1.000e+300,
  pi =  3.14159265358979311600e+00, /* 0x400921FB, 0x54442D18 */
  pio2_hi =  1.57079632679489655800e+00, /* 0x3FF921FB, 0x54442D18 */
  pio2_lo =  6.12323399573676603587e-17, /* 0x3C91A626, 0x33145C07 */
  pio4_hi =  7.85398163397448278999e-01; /* 0x3FE921FB, 0x54442D18 */

/* coefficient for R(x^2) */
const double coeff[]=
{
  3.47933107596021167570e-05, /* ps5 = 0x3F023DE1, 0x0DFDF709 */
  7.70381505559019352791e-02, /* qs4 = 0x3FB3B8C5, 0xB12E9282 */
  7.91534994289814532176e-04, /* ps4= 0x3F49EFE0, 0x7501B288 */
  -4.00555345006794114027e-02, /* ps3 = 0xBFA48228, 0xB5688F3B */
  2.01212532134862925881e-01, /* ps2 = 0x3FC9C155, 0x0E884455 */
  -3.25565818622400915405e-01, /* ps1 = 0xBFD4D612, 0x03EB6F7D */
  1.66666666666666657415e-01, /* ps0 = 0x3FC55555, 0x55555555 */
  -6.88283971605453293030e-01, /* qs3 = 0xBFE6066C, 0x1B8D0159 */
  2.02094576023350569471e+00, /* qs2 = 0x40002AE5, 0x9C598AC8 */
  -2.40339491173441421878e+00 /* qs1 = 0xC0033A27, 0x1C8A2D4B */
};

