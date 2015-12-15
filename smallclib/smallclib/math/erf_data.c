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
*              file : $RCSfile: s_erf.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice 
 * is preserved.
 * ====================================================
 */

/*
 * Constants required by erf() and erfc()
*/

#include "low/_math.h"

/* pp0-pp4, qq1- qq5 */
const double __coeff1[] = {
 -2.37630166566501626084e-05, /*pp4 = 0xBEF8EAD6, 0x120016AC */
 -3.96022827877536812320e-06, /*qq5 = 0xBED09C43, 0x42A26120 */
 -5.77027029648944159157e-03, /*pp3 = 0xBF77A291, 0x236668E4 */
 -2.84817495755985104766e-02, /*pp2 = 0xBF9D2A51, 0xDBD7194F */
 -3.25042107247001499370e-01, /*pp1 = 0xBFD4CD7D, 0x691CB913 */
  1.28379167095512558561e-01, /*pp0 = 0x3FC06EBA, 0x8214DB68 */
  1.32494738004321644526e-04, /*qq4 = 0x3F215DC9, 0x221C1A10 */
  5.08130628187576562776e-03, /*qq3 = 0x3F74D022, 0xC4D36B0F */
  6.50222499887672944485e-02, /*qq2 = 0x3FB0A54C, 0x5536CEBA */
  3.97917223959155352819e-01 /*qq1 = 0x3FD97779, 0xCDDADC09 */
  };

  /*
   * Coefficients for approximation to  erf  in [0.84375,1.25]
   */
/* pa0 - pa6 ,qa1-qa6 */
const double __coeff2[] = {
 -2.16637559486879084300e-03, /*pa6 = 0xBF61BF38, 0x0A96073F */
  1.19844998467991074170e-02, /*qa6 = 0x3F888B54, 0x5735151D */
  3.54783043256182359371e-02, /*pa5 = 0x3FA22A36, 0x599795EB */
 -1.10894694282396677476e-01, /*pa4 = 0xBFBC6398, 0x3D3E28EC */
  3.18346619901161753674e-01, /*pa3 = 0x3FD45FCA, 0x805120E4 */
 -3.72207876035701323847e-01, /*pa2 = 0xBFD7D240, 0xFBB8C3F1 */
  4.14856118683748331666e-01, /*pa1 = 0x3FDA8D00, 0xAD92B34D */
 -2.36211856075265944077e-03, /*pa0 = 0xBF6359B8, 0xBEF77538 */
  1.36370839120290507362e-02, /*qa5 = 0x3F8BEDC2, 0x6B51DD1C */
  1.26171219808761642112e-01, /*qa4 = 0x3FC02660, 0xE763351F */
  7.18286544141962662868e-02, /*qa3 = 0x3FB2635C, 0xD99FE9A7 */
  5.40397917702171048937e-01, /*qa2 = 0x3FE14AF0, 0x92EB6F33 */
  1.06420880400844228286e-01 /*qa1 = 0x3FBB3E66, 0x18EEE323 */
};
  /*
   * Coefficients for approximation to  erfc in [1.25,1/0.35]
   */
/* ra0-ra7, sa1-sa8 */
const double __coeff3[] = {
 -9.81432934416914548592e+00, /*ra7 = 0xC023A0EF, 0xC69AC25C */
 -6.04244152148580987438e-02, /*sa8 = 0xBFAEEFF2, 0xEE749A62 */
 -8.12874355063065934246e+01, /*ra6 = 0xC0545265, 0x57E4D2F2 */
 -1.84605092906711035994e+02, /*ra5 = 0xC067135C, 0xEBCCABB2 */
 -1.62396669462573470355e+02, /*ra4 = 0xC0644CB1, 0x84282266 */
 -6.23753324503260060396e+01, /*ra3 = 0xC04F300A, 0xE4CBA38D */
 -1.05586262253232909814e+01, /*ra2 = 0xC0251E04, 0x41B0E726 */
 -6.93858572707181764372e-01, /*ra1 = 0xBFE63416, 0xE4BA7360 */
 -9.86494403484714822705e-03, /*ra0 = 0xBF843412, 0x600D6435 */
  6.57024977031928170135e+00, /*sa7 = 0x401A47EF, 0x8E484A93 */
  1.08635005541779435134e+02, /*sa6 = 0x405B28A3, 0xEE48AE2C */
  4.29008140027567833386e+02, /*sa5 = 0x407AD021, 0x57700314 */
  6.45387271733267880336e+02, /*sa4 = 0x40842B19, 0x21EC2868 */
  4.34565877475229228821e+02, /*sa3 = 0x407B290D, 0xD58A1A71 */
  1.37657754143519042600e+02, /*sa2 = 0x4061350C, 0x526AE721 */
  1.96512716674392571292e+01 /*sa1= 0x4033A6B9, 0xBD707687 */
};
  /*
   * Coefficients for approximation to  erfc in [1/.35,28]
   */
/* rb0-rb6, sb1-sb7 */
const double __coeff4[] = {
 -4.83519191608651397019e+02, /*rb6 = 0xC07E384E, 0x9BDC383F */
 -2.24409524465858183362e+01, /*sb7 = 0xC03670E2, 0x42712D62 */
 -1.02509513161107724954e+03, /*rb5 = 0xC0900461, 0x6A2E5992 */
 -6.37566443368389627722e+02, /*rb4 = 0xC083EC88, 0x1375F228 */
 -1.60636384855821916062e+02, /*rb3 = 0xC064145D, 0x43C5ED98 */
 -1.77579549177547519889e+01, /*rb2 = 0xC031C209, 0x555F995A */
 -7.99283237680523006574e-01, /*rb1 = 0xBFE993BA, 0x70C285DE */
 -9.86494292470009928597e-03, /*rb0 = 0xBF843412, 0x39E86F4A */
  4.74528541206955367215e+02, /*sb6 = 0x407DA874, 0xE79FE763 */
  2.55305040643316442583e+03, /*sb5 = 0x40A3F219, 0xCEDF3BE6 */
  3.19985821950859553908e+03, /*sb4 = 0x40A8FFB7, 0x688C246A */
  1.53672958608443695994e+03, /*sb3 = 0x409802EB, 0x189D5118 */
  3.25792512996573918826e+02, /*sb2 = 0x40745CAE, 0x221B9F0A */
  3.03380607434824582924e+01 /*sb1 = 0x403E568B, 0x261D5190 */
};
