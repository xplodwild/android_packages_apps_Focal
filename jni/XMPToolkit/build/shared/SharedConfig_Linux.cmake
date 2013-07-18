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
# Shared config for linux
# ==============================================================================

add_definitions(-DUNIX_ENV=1)

# Linux -------------------------------------------

set(COMMON_PLATFORM_LINK " ${${COMPONENT}_PLATFORM_LINK}")
set(COMMON_SHARED_COMPILE_FLAGS "-fPIC  ${${COMPONENT}_SHARED_COMPILE_FLAGS} -fexceptions -Wformat -Wformat-security ")
set(COMMON_SHARED_COMPILE_DEBUG_FLAGS " ${${COMPONENT}_SHARED_COMPILE_DEBUG_FLAGS} -g -O0 -DDEBUG=1 -D_DEBUG=1")
set(COMMON_SHARED_COMPILE_RELEASE_FLAGS "-Os ${${COMPONENT}_SHARED_COMPILE_RELEASE_FLAGS} -DNDEBUG=1 -D_NDEBUG=1")

if(NOT ${COMPONENT}_DISABLE_ALL_WARNINGS)
	set(COMMON_SHARED_COMPILE_FLAGS "${COMMON_SHARED_COMPILE_FLAGS} -Wall")
endif()

if(${COMPONENT}_ENABLE_SECURE_SETTINGS)
	set(COMMON_SHARED_COMPILE_FLAGS "${COMMON_SHARED_COMPILE_FLAGS} -fstack-protector -D_FORTIFY_SOURCE=2")
	set(COMMON_PLATFORM_LINK "${COMMON_PLATFORM_LINK} -Wl,-z,relro -Wl,-z,now")
endif()

set(CMAKE_C_FLAGS "${COMMON_SHARED_COMPILE_FLAGS} ${${COMPONENT}_EXTRA_C_COMPILE_FLAGS} ")
set(CMAKE_C_FLAGS_DEBUG "${COMMON_SHARED_COMPILE_DEBUG_FLAGS}")
set(CMAKE_C_FLAGS_RELEASE "${COMMON_SHARED_COMPILE_RELEASE_FLAGS}")
set(CMAKE_CXX_FLAGS "${COMMON_SHARED_COMPILE_FLAGS} ${COMMON_EXTRA_CXX_COMPILE_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${COMMON_SHARED_COMPILE_DEBUG_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${COMMON_SHARED_COMPILE_RELEASE_FLAGS}")
set(COMMON_PLATFORM_BEGIN_WHOLE_ARCHIVE "-Wl,--whole-archive")
set(COMMON_PLATFORM_END_WHOLE_ARCHIVE "-Wl,--no-whole-archive")
set(COMMON_DYLIBEXTENSION	"so")

if(NOT CMAKE_BUILD_WITH_INSTALL_RPATH)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    set(CMAKE_INSTALL_RPATH $ORIGIN)
endif()

SetupCompilerFlags()
SetupGeneralDirectories()


# ==============================================================================
# Functions
# ==============================================================================

# ==============================================================================
# Function: Add Mac framework to list of libraries to link to
# ==============================================================================
#
function(AddMacFramework appname fwname)
	# ignore on linux
endfunction(AddMacFramework)

# ==============================================================================
# Function: Set platform specific link flags to target
# ==============================================================================
#
# TODO: make function work on all platforms. Unify with AddMacFramework.
function(SetPlatformLinkFlags target linkflags libname)
	set_target_properties(${target} PROPERTIES LINK_FLAGS "${XMP_PLATFORM_LINK} ${linkflags}")

endfunction(SetPlatformLinkFlags)

# ==============================================================================
# Function: Set the output path depending on isExecutable
#	The output path is corrected. 
# ==============================================================================
#
function(SetOutputPath2 path targetName targetType)
	# remove last path item if not makefile
	if(NOT XMP_IS_MAKEFILE_BUILD)
		get_filename_component(correctedPath ${path} PATH)
		#message("SetOutputPath: ${path} to ${correctedPath}")
	else()
		set(correctedPath ${path})
	endif()

	if(${targetType} MATCHES "EXECUTABLE")
		set_property(TARGET ${targetName} PROPERTY RUNTIME_OUTPUT_DIRECTORY ${correctedPath})
	elseif(${targetType} MATCHES "DYNAMIC")
		set_property(TARGET ${targetName} PROPERTY LIBRARY_OUTPUT_DIRECTORY ${correctedPath})
	else()
		set_property(TARGET ${targetName} PROPERTY ARCHIVE_OUTPUT_DIRECTORY ${correctedPath})
	endif()
endfunction(SetOutputPath2)

# ==============================================================================
# Function: Set precompiled headers
# ==============================================================================
#
function( SetPrecompiledHeaders target pchHeader pchSource )
	# Linux
	# not supported
endfunction( SetPrecompiledHeaders )

# ==============================================================================
# Function: Add library in native OS name to variable resultName
# ==============================================================================
#
function(AddLibraryNameInNativeFormat libname libpath resultName)
	foreach(libname_item ${libname})
		set(result "${libpath}/lib${libname_item}.a")
		set(resultList ${resultList} ${result})
	endforeach()	
	set(${resultName} ${${resultName}} ${resultList} PARENT_SCOPE)
	#message("AddLibraryNameInNativeFormat: ${resultList} ")
endfunction(AddLibraryNameInNativeFormat)

# ==============================================================================
# Function: Add shared library in native OS name to variable resultName
# ==============================================================================
#
function(AddSharedLibraryNameInNativeFormat libname libpath resultName)
	foreach(libname_item ${libname})
		set(result "${libpath}/lib${libname_item}.so")
		set(resultList ${resultList} ${result})
	endforeach()	
	set(${resultName} ${${resultName}} ${resultList} PARENT_SCOPE)
	#message("AddSharedLibraryNameInNativeFormat: ${resultList} ")
endfunction(AddSharedLibraryNameInNativeFormat)

# ==============================================================================
# Function: Add tbb library to variables (resultName and resultNameDebug)
# ==============================================================================
#
function(AddTbbLib resultName resultNameDebug)
    # not supported under linux
endfunction(AddTbbLib)

# ==============================================================================
# Function: Make an OSX bundle
# ==============================================================================
# 	Optional parameter (!!)
#	${ARGV3} = Name of Info.plist located in ${RESOURCE_ROOT}/${XMP_PLATFORM_SHORT}
#	${ARGV4} = Name of header file to use to generate Info.plist
#
function(MakeBundle appname extension outputDir)
	# not supported under linux
endfunction(MakeBundle)
