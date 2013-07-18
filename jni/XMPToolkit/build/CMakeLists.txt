# =================================================================================================
# ADOBE SYSTEMS INCORPORATED
# Copyright 2013 Adobe Systems Incorporated
# All Rights Reserved
#
# NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
# of the Adobe license agreement accompanying it.
# =================================================================================================
  
# ==============================================================================
# define minimum cmake version
cmake_minimum_required(VERSION 2.8.6)


# Enable folder grouping of projects in IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ==============================================================================
# This project
option(CMAKE_CL_64 "64bit target?" OFF)
set(TARGET_NAME XMPToolkitSDK)
SET(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Configurations" FORCE)
if(CMAKE_CL_64)
	project(${TARGET_NAME}64)
else(CMAKE_CL_64)
	project(${TARGET_NAME})
endif(CMAKE_CL_64)

if(NOT DEFINED XMP_TOPLEVELPROJECT)
	set(XMP_TOPLEVELPROJECT "XMPBuild" CACHE INTERNAL "Identify top level project responsible for build XMP libs")
endif()

# ==============================================================================
# Globals
# ==============================================================================

set(XMP_ROOT ${PROJECT_SOURCE_DIR}/..)

# ==============================================================================
# SET XMP and Plugin Version Numbers defined in an external file
# ==============================================================================

# Get Version numbers from an external file XMP_BuildNumber.txt
#file(STRINGS "XMP_BuildNumber.txt" XMP_BUILD_STRING) 

# Parse List for Minor Major and Branchnumber
#string(REGEX MATCHALL "[0-9]" XMP_BUILD_VERSION_LIST "${XMP_BUILD_STRING}")
#list(GET XMP_BUILD_VERSION_LIST 0 XMP_VERSION_MAJOR)
#list(GET XMP_BUILD_VERSION_LIST 1 XMP_VERSION_MINOR)
#list(GET XMP_BUILD_VERSION_LIST 2 XMP_BRANCH_NUMBER)
	
#set(XMP_VERSION ${XMP_VERSION_MAJOR}.${XMP_VERSION_MINOR}.${XMP_BRANCH_NUMBER})

# Write the Version infos to a header file
#configure_file (
#  "${XMP_ROOT}/../toolkit/build/XMP_VersionNumber.h.in"
#  "${XMP_ROOT}/../toolkit/public/include/XMP_VersionNumber.h"
#  )
  
# extract BUILDID from XMP_BuildInfo.h to be used by external CMake files, e.g. FaceDetector DLL
#file(STRINGS "XMP_BuildInfo.h" XMP_BUILDID_LINE REGEX "#define BUILDID ")
#string(REGEX MATCHALL "[0-9]+" XMP_BUILDID ${XMP_BUILDID_LINE})

# ==============================================================================
# Shared config
# ==============================================================================
# setup some values before calling shared config
set(XMP_THIS_PROJECT_RELATIVEPATH "..")
include(${CMAKE_CURRENT_SOURCE_DIR}/${XMP_THIS_PROJECT_RELATIVEPATH}/build/XMP_Config.cmake)

# ==============================================================================
# Sub projects
# ==============================================================================
add_subdirectory(${XMP_ROOT}/XMPCore/build ${XMP_ROOT}/XMPCore/build/${XMP_CMAKEFOLDER_NAME})
if (NOT APPLE_IOS)
	add_subdirectory(${XMP_ROOT}/XMPFiles/build ${XMP_ROOT}/XMPFiles/build/${XMP_CMAKEFOLDER_NAME})
endif()

# ==============================================================================
# configuration feedback
# ==============================================================================

message (STATUS "===========================================================================")
message (STATUS " ${TARGET_NAME} ")
#message (STATUS " Version = ${XMP_VERSION}")
message (STATUS "===========================================================================")
#message (STATUS " ${XMP_EXTRA_BUILDMACHINE}")
message (STATUS " 64 bit build is: ${CMAKE_CL_64}")
message (STATUS " Static build is: ${XMP_BUILD_STATIC}")
message (STATUS " ${TARGET_NAME} ")
message (STATUS " ${XMP_PLATFORM_LINK} ")
message (STATUS " ${XMP_GLOBALS}")
message (STATUS " Compiler: ")
message (STATUS " CC=${CMAKE_C_COMPILER}")
message (STATUS " CXX=${CMAKE_CXX_COMPILER}")
message (STATUS "===========================================================================")
