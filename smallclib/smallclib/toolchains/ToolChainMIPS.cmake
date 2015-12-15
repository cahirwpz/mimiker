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
#              file : $RCSfile: ToolChainMIPS.txt,v $
#            author : $Author  Imagination Technologies Ltd
# date last revised :
#   current version :
###############################################################################

# Toolchain descriptor for building with Bare Metal MIPS gcc toolkit

IF (NOT MIPS_INST_ROOT)
	SET(MIPS_INST_ROOT $ENV{MIPS_INST_ROOT})
	IF ("x$MIPS_INST_ROOT" STREQUAL "x")
		message (FATAL_ERROR "No MIPS_INST_ROOT defined!!")
	ENDIF ("x$MIPS_INST_ROOT" STREQUAL "x")
ENDIF (NOT MIPS_INST_ROOT)

# this one is important
IF (NOT CMAKE_SYSTEM_NAME)
SET(CMAKE_SYSTEM_NAME mips-mti-elf)
ENDIF (NOT CMAKE_SYSTEM_NAME)

IF (NOT CMAKE_INSTALL_PREFIX)
SET(CMAKE_INSTALL_PREFIX ${MIPS_INST_ROOT}
  CACHE PATH "Install within MIPS toolchain if no alternative is specified" FORCE)
ENDIF (NOT CMAKE_INSTALL_PREFIX)

# specify the cross compiler

SET(CCNAME ${CMAKE_SYSTEM_NAME}-gcc)

IF(WIN32)
SET(CMAKE_C_COMPILER ${MIPS_INST_ROOT}/bin/$$CCNAME}.exe)
ELSE(WIN32)
SET(CMAKE_C_COMPILER ${CCNAME})
ENDIF(WIN32)

#enable_language(ASM)
SET(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})

SET(CMAKE_FIND_ROOT_PATH ${MIPS_INST_ROOT})

SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)