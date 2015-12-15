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
#              file : $RCSfile: MultiLibConfig.cmake,v $
#            author : $Author  Imagination Technologies Ltd
# date last revised :
#   current version :
###############################################################################

#.rst:
# MultiLibConfig
# --------------
#
# This module provides generic stubs for building libraries with a C
# compiler that supports multi-lib configurations with the
# command-line option '-print-multi-lib'. Output format is expected to
# match that of . For compilers that do not support this option,
# MultiLibConfig has no effect and the default configuration of the
# library is built.
#
# Fine grained control of multilib configurations is provided with the
# variables MULTILIB_SELECT and MULTILIB_SKIP.
#
# MULTILIB_SELECT=[ALL|<regex>]
# ALL: all configurations, except those skipped by MULTILIB_SKIP are
# 	built
# <regex>: only configs matching the pattern, excluding those skipped by
# 	MULTILIB_SKIP are built
# If MULTILIB_SELECT is not specified, only the default configuration
# is built.
#
# MULTILIB_SKIP=[NONE|<regex>]
# NONE: all configurations selected by MULTILIB_SELECT are built
# regex: only those configurations selected by MULTILIB_SELECT, which
# 	do NOT match the pattern are built
# If MULTILIB_SKIP is not specified, behaviour is same as
# MULTILIB_SKIP=NONE. If MULTILIB_SELECT is not specified, MULTILIB_SKIP
# is ignored.
#
# This module defines the following variables:
# 
# .. variable:: MULTILIB_PRE_TOP
# Flag to indicate top-level of multilib recursion, before creating
# actual multilib targets.
#
# .. variable:: MULTILIB_TOP
# Flag to indicate top-level of multilib recursion, after creating
# multilib targets.
#
# .. variable:: MULTILIB_FLAVOUR
# `Compiler options' part of a multilib descriptor, only visible in
# the recursive sub-context.
# 
# .. variable:: MULTILIB_DIR
# `Target directory' part of multilib descriptor, only visible in the
# recursive sub-context.
#
# .. variable:: MULTILIB_NAME
# Same as MULTILIB_FLAVOUR, or 'default' for the default multilib
# configuration, where MULTILIB_FLAVOUR is empty. Only visible in the
# recursive sub-context.
#
# .. variable:: MULTILIB_TARGET_<name>
# For each library added, a reference to the current multilib target.
# 
# Example Usage:
# .. code-block::
#   set(sources_math sin.c cos.s floor.c ...)
#
#   include(MultiLibConfig)
#
#   multilib_build_setup()
#
#   if (MULTILIB_TOP)
#     <Do everything you want to do only once, like installing headers,
#     top-level checks, etcetra>
#   else (MULTILIB_TOP)
#     ## build the library for each multilib configuration>
#     ## This step creates the target and sets up compiler flags for it based 
#     ## on the multilib configuration. It also creates a handle for the user 
#     ## to override target properties later.
#     multilib_add_library(m STATIC ${sources_math})
#     ## The handle MULTILIB_TARGET_<name> allows user to override target
#     ## properties or set install path
#     set_target_properties(${MULTILIB_TARGET_m} PROPERTIES OUTPUT_NAME mymath)
#     install(TARGETS ${MULTILIB_TARGET_m}
# 	 ARCHIVE DESTINATION lib/${MULTILIB_DIR})
#   endif (MULTILIB_TOP)
#

#=============================================================================
# Copyright 2014 Imagination Technologies Limited
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

cmake_minimum_required(VERSION 2.6)

#.rst:
# .. command:: multilib_pre_setup
# 
# Macro to flag the top-level multilib configuration step by setting or
# resetting the variable MULTILIB_PRE_TOP before defining the actual multilib
# targets with :command:`multilib_build_setup`. This is useful for defining
# top-level target dependencies which must be done only once.
#
# multilib_pre_setup()
#
macro (multilib_pre_setup)
  if (NOT (DEFINED MULTILIB_FLAVOUR))
    # Set a flag to indicate top-level invocation of CMake
    set(MULTILIB_PRE_TOP YES)
  else  () # not MULTILIB_FLAVOUR
    # Unset flag to indicate leaf-level invocation of cmake
    set(MULTILIB_PRE_TOP NO)
  endif () # MULTILIB_FLAVOUR
endmacro()

#.rst:
# .. command:: multilib_recurse_step
# 
# Do a recursive CMake invocation for the current project with a new output
# directory name determined by the mutilib _flavour string. If no
# MULTILIB_SELECT parameter is provided, this can be used to flash-build
# current project with a custom set of CFLAGS (separated by '@').
# 
#  multilib_recurse_step()
#
function (multilib_recurse_step _FLAVOUR _DIR )
  set(MULTILIB_DIR "${_DIR}")
  
  # make doesn't like = within target names, replace with ~
  string(REPLACE "=" "~" MULTILIB_FLAVOUR "${_FLAVOUR}")
  
  if ("${MULTILIB_FLAVOUR}" STREQUAL "")
    # Specify a non-empty build directory for default configuration
    set(MULTILIB_NAME "default")
  else () # MULTILIB_FLAVOUR != ""
    # else derive build-directory from configuration string
    set(MULTILIB_NAME ${MULTILIB_FLAVOUR})      
  endif() # ${MULTILIB_FLAVOUR} == ""
  
  # Recurse to build this multilib configuration
  add_subdirectory(. "${MULTILIB_NAME}")
endfunction()

#.rst:
# .. command:: multilib_build_setup
# 
# Recursive magic for building multi-lib targets. This sets the flag
# MULTILIB_TOP to indicate the top-level invocation. For sub-invocations, it
# calls multilib_recurse_step.
#
#  multilib_build_setup()
#
macro (multilib_build_setup)

  if ((DEFINED MULTILIB_SELECT) AND NOT (DEFINED MULTILIB_FLAVOUR))

    execute_process(COMMAND ${CMAKE_C_COMPILER} "-print-multi-lib"
      OUTPUT_VARIABLE _C_MULTILIBS)

    # Default setup for compilers that don't support -print-multi-lib
    if (NOT _C_MULTILIBS)
      set(_C_MULTILIBS ".;")
    endif () # _C_MULTILIBS

    # Mangle multi-lib output so we can tokenize easily with foreach loop
    string(REPLACE ";" "\\;" _C_MULTILIBS "${_C_MULTILIBS}")
    string(REPLACE "\n" ";" _C_MULTILIBS "${_C_MULTILIBS}")

    if (MULTILIB_SELECT AND (NOT "${MULTILIB_SELECT}" STREQUAL "ALL"))
      # To filter the multilib configurations that we want to build
      set(_SELECT_REGEX ${MULTILIB_SELECT})
      # make doesn't like = within target names, replace with ~
      string(REPLACE "=" "~" MULTILIB_SELECT "${MULTILIB_SELECT}")
    else () # (${MULTILIB_SELECT} == ALL)
      # Match everything so that all configurations get built
      set(_SELECT_REGEX ".*")
    endif () # MULTILIB_SELECT && (${MULTILIB_SELECT} != ALL)

    if (MULTILIB_SKIP AND (NOT "${MULTILIB_SKIP}" STREQUAL "NONE"))
      # To filter the multilib configurations that we don't want to build
      set(_SKIP_REGEX ${MULTILIB_SKIP})
    else () # ${MULTILIB_SKIP} != NONE
      # Match nothing so that no configurations are skipped over
      set(_SKIP_REGEX "^$")
    endif () # MULTILIB_SKIP  && ${MULTILIB_SKIP} != NONE

    foreach (mlib_config ${_C_MULTILIBS})
      string(REGEX MATCH "@.*" _FLAVOUR "${mlib_config}")
      string(REGEX MATCH "[^;]*" _DIR "${mlib_config}")

      if (_FLAVOUR)
	string(REGEX MATCH "${_SELECT_REGEX}" _SELECT_MATCH "${_FLAVOUR}")
	string(REGEX MATCH "${_SKIP_REGEX}" _SKIP_MATCH "${_FLAVOUR}")
      endif() # _FLAVOUR

      if (_SELECT_MATCH AND NOT _SKIP_MATCH)
	multilib_recurse_step(${_FLAVOUR} ${_DIR})
      endif () # MATCH && !SKIP
    endforeach () # mlib_config

    # Set a flag to indicate top-level invocation of CMake
    set(MULTILIB_TOP YES)

    # Clean-up intermediates
    unset(_SELECT_REGEX)
    unset(_SKIP_REGEX)
    unset(_C_MULTILIBS)

  else () # !MULTILIB_SELECT || MULTILIB_FLAVOUR
    # The vanilla 'run'-mode to build actual targets

    # Break in to individual multi-lib compiler options
    string(REPLACE "@" " -" MULTILIB_CFLAGS "${MULTILIB_FLAVOUR}")
    # Revert ~ back to = for the compiler command-line 
    string(REPLACE "~" "=" MULTILIB_CFLAGS "${MULTILIB_CFLAGS}")
    # Unset flag to indicate leaf-level invocation of cmake
    set(MULTILIB_TOP NO)

  endif () # MULTILIB_SELECT && !MULTILIB_FLAVOUR
endmacro ()

#.rst:
# .. command:: multilib_add_library
#
# Create a library target for current multilib configuration and generate a
# variable MULTILIB_TARGET_<name>, based on the first input parameter to
# provide a handle to the new target.
#
#  multilib_add_library(_NAME ... ARGN)
#
macro (multilib_add_library _name)
  # Add target - _name prefixed with multilib config string
  add_library (${MULTILIB_FLAVOUR}${_name} ${ARGN})

  set_target_properties (${MULTILIB_FLAVOUR}${_name} PROPERTIES 
    # Reset library output-name to user-specified name
    OUTPUT_NAME ${_name}
    # Add multi-lib build options to compiler command-line for this target
    COMPILE_FLAGS "${MULTILIB_CFLAGS}")

  # Set up a handle for user to access/modify properties for this target
  set(MULTILIB_TARGET_${_name} ${MULTILIB_FLAVOUR}${_name})
endmacro ()

#
# End of multilib.cmake
#
