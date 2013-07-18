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

# ==============================================================================
# Shared config
# ==============================================================================

include(${COMMON_BUILD_SHARED_DIR}/SharedConfig_Common.cmake)

# ============================================================================
# Load platform specified configurations
# ============================================================================

SetupTargetArchitecture()
SetupInternalBuildDirectory()

if (UNIX)
	if (APPLE)
		if (APPLE_IOS)
			include(${COMMON_BUILD_SHARED_DIR}/SharedConfig_Ios.cmake)
		else ()
			include(${COMMON_BUILD_SHARED_DIR}/SharedConfig_Mac.cmake)
		endif ()
	else ()
        execute_process(COMMAND "uname" OUTPUT_VARIABLE OSNAME)
        string(TOUPPER "${OSNAME}" OSNAME)

        if ( ${OSNAME} MATCHES SUNOS)
            execute_process(COMMAND "uname" "-p" OUTPUT_VARIABLE PLATFORM_SUNOS_ARCH)
            string(TOUPPER "${PLATFORM_SUNOS_ARCH}" PLATFORM_SUNOS_ARCH)

            if ( ${PLATFORM_SUNOS_ARCH} MATCHES SPARC)
		        include(${COMMON_BUILD_SHARED_DIR}/SharedConfig_sunos_sparc.cmake)
            else()
		        include(${COMMON_BUILD_SHARED_DIR}/SharedConfig_sunos_intel.cmake)
            endif()
        else()
		    include(${COMMON_BUILD_SHARED_DIR}/SharedConfig_Linux.cmake)
        endif()
	endif ()
else ()
	if (WIN32)
		include(${COMMON_BUILD_SHARED_DIR}/SharedConfig_Win.cmake)
	endif ()
endif ()

# ==============================================================================
# Debug feedback
# ==============================================================================

#message(STATUS " ${OUTPUT_DIR}")
#message(STATUS " ${${COMPONENT}_PLATFORM_FOLDER}")
#message(STATUS " ${${COMPONENT}_BUILDMODE_DIR}")
#message(STATUS " ${CMAKE_CXX_FLAGS}")
