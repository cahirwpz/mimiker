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
*              file : $RCSfile: erff_data.c,v $
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

#include "low/_math.h"
#include "low/_fpuinst.h"

const float __coeff1f[]={
 -2.3763017452e-05, /*pp4 = 0xb7c756b1 */
 -3.9602282413e-06, /*qq5 = 0xb684e21a */
 -5.7702702470e-03, /*pp3 = 0xbbbd1489 */
 -2.8481749818e-02, /*pp2 = 0xbce9528f */
 -3.2504209876e-01, /*pp1 = 0xbea66beb */
  1.2837916613e-01, /*pp0 = 0x3e0375d4 */
  1.3249473704e-04, /*qq4 = 0x390aee49 */
  5.0813062117e-03, /*qq3 = 0x3ba68116 */
  6.5022252500e-02, /*qq2 = 0x3d852a63 */
  3.9791721106e-01 /*qq1 = 0x3ecbbbce */
};
/*
 * Coefficients for approximation to  erf  in [0.84375,1.25]
 */
const float __coeff2f[]={
 -2.1663755178e-03, /*pa6  = 0xbb0df9c0 */
  1.1984500103e-02, /*qa6  = 0x3c445aa3 */
  3.5478305072e-02, /*pa5  = 0x3d1151b3 */
 -1.1089469492e-01, /*pa4  = 0xbde31cc2 */
  3.1834661961e-01, /*pa3  = 0x3ea2fe54 */
 -3.7220788002e-01, /*pa2  = 0xbebe9208 */
  4.1485610604e-01, /*pa1  = 0x3ed46805 */
 -2.3621185683e-03, /*pa0  = 0xbb1acdc6 */
  1.3637083583e-02, /*qa5  = 0x3c5f6e13 */
  1.2617121637e-01, /*qa4  = 0x3e013307 */
  7.1828655899e-02, /*qa3  = 0x3d931ae7 */
  5.4039794207e-01, /*qa2  = 0x3f0a5785 */
  1.0642088205e-01 /*qa1  = 0x3dd9f331 */
};

/*
 * Coefficients for approximation to  erfc in [1.25,1/0.35]
 */
const float __coeff3f[] = {
 -9.8143291473e+00, /*ra7 = 0xc11d077e */
 -6.0424413532e-02, /*sa8 = 0xbd777f97 */
 -8.1287437439e+01, /*ra6 = 0xc2a2932b */
 -1.8460508728e+02, /*ra5 = 0xc3389ae7 */
 -1.6239666748e+02, /*ra4 = 0xc322658c */
 -6.2375331879e+01, /*ra3 = 0xc2798057 */
 -1.0558626175e+01, /*ra2 = 0xc128f022 */
 -6.9385856390e-01, /*ra1 = 0xbf31a0b7 */
 -9.8649440333e-03, /*ra0 = 0xbc21a093 */
  6.5702495575e+00, /*sa7 = 0x40d23f7c */
  1.0863500214e+02, /*sa6 = 0x42d9451f */
  4.2900814819e+02, /*sa5 = 0x43d6810b */
  6.4538726807e+02, /*sa4 = 0x442158c9 */
  4.3456588745e+02, /*sa3 = 0x43d9486f */
  1.3765776062e+02, /*sa2 = 0x4309a863 */
  1.9651271820e+01  /*sa1 = 0x419d35ce */
};

/*
 * Coefficients for approximation to  erfc in [1/.35,28]
 */
const float __coeff4f[] = {
 -4.8351919556e+02, /*rb6 = 0xc3f1c275 */
 -2.2440952301e+01, /*sb7 = 0xc1b38712 */
 -1.0250950928e+03, /*rb5 = 0xc480230b */
 -6.3756646729e+02, /*rb4 = 0xc41f6441 */
 -1.6063638306e+02, /*rb3 = 0xc320a2ea */
 -1.7757955551e+01, /*rb2 = 0xc18e104b */
 -7.9928326607e-01, /*rb1 = 0xbf4c9dd4 */
 -9.8649431020e-03, /*rb0 = 0xbc21a092 */
  4.7452853394e+02, /*sb6 = 0x43ed43a7 */
  2.5530502930e+03, /*sb5 = 0x451f90ce */
  3.1998581543e+03, /*sb4 = 0x4547fdbb */
  1.5367296143e+03, /*sb3 = 0x44c01759 */
  3.2579251099e+02, /*sb2 = 0x43a2e571 */
  3.0338060379e+01  /*sb1 = 0x41f2b459 */
};

