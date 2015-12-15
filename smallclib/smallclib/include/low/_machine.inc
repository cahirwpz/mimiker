/*****************************************************************************
* Copyright: 2013-2014 by Imagination Technologies Limited. 
*          All rights reserved. No part of this software, either material or
*          conceptual may be copied or distributed, transmitted, transcribed,
*          stored in a retrieval system or translated into any human or
*          computer language in any form by any means, electronic, mechanical,
*          manual or other-wise, or disclosed to third parties without the
*          express written permission of:
*               Imagination Technologies Limited, HomePark Industrial Estate,
*               King's Langley, Hertfordshire, WD4 8LZ, U.K.
******************************************************************************/

/******************************************************************************
*                 file : $RCSfile: _machine.inc,v $ 
*               author : $Author: faraz.shahbazker $ 
*    date last revised : $Date: 2014/04/28 $ 
*      current version : $Revision: #3 $
******************************************************************************/

/* Bit descriptors for floating point status/control registers */

#define FCSR_ENABLE_BITS		0x000007C0
#define FCSR_ENABLE_SHIFT 	7

#define FCSR_ENABLE_INEXACT_BIT		0x00000080
#define FCSR_ENABLE_UNDERFLOW_BIT 	0x00000100
#define FCSR_ENABLE_OVERFLOW_BIT 	0x00000200
#define FCSR_ENABLE_DIV0_BIT 		0x00000400
#define FCSR_ENABLE_INVALIDOP_BIT 	0x00000800

#define FCSR_FLAG_BITS 	      0x0000007C
#define FCSR_FLAG_SHIFT       2

#define FCSR_FLAG_INEXACT_BIT	0x00000004
#define FCSR_FLAG_UNDERFLOW_BIT 0x00000008
#define FCSR_FLAG_OVERFLOW_BIT 	0x00000010
#define FCSR_FLAG_DIV0_BIT 	0x00000020
#define FCSR_FLAG_INVALIDOP_BIT 0x00000040


#define FCSR_CAUSE_BITS		0x0003F000
#define FCSR_CAUSE_SHIFT 	12

#define FCSR_CAUSE_INEXACT_BIT		0x00001000
#define FCSR_CAUSE_UNDERFLOW_BIT 	0x00002000
#define FCSR_CAUSE_OVERFLOW_BIT 	0x00004000
#define FCSR_CAUSE_DIV0_BIT 		0x00008000
#define FCSR_CAUSE_INVALIDOP_BIT 	0x00010000
#define FCSR_CAUSE_UNIMPL_BIT 		0x00020000

#define FCSR_ROUND_BITS		0x00000003
#define FCSR_ROUND_SHIFT 	0

#define FCSR_ROUND_TO_NEAREST	0x0
#define FCSR_ROUND_TO_ZERO	0x1
#define FCSR_ROUND_TO_PLUSINF	0x2
#define FCSR_ROUND_TO_MINUSINF	0x3
