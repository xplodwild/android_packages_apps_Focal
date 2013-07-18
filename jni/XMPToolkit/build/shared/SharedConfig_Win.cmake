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
# Shared config for windows
# ==============================================================================

if(CMAKE_CL_64)
	set(COMMON_SHARED_COMPILE_FLAGS "-DWIN64 -D_WIN64=1")
	set(COMMON_WIN32_CXX_EXTRAFLAGS "/bigobj")
	set(COMMON_WIN32_LINK_EXTRAFLAGS "")
else(CMAKE_CL_64)
	set(COMMON_SHARED_COMPILE_FLAGS "-DWIN32")
	set(COMMON_WIN32_CXX_EXTRAFLAGS "/arch:SSE2")
	set(COMMON_WIN32_LINK_EXTRAFLAGS "/SAFESEH /NXCOMPAT /LARGEADDRESSAWARE")
endif(CMAKE_CL_64)

set(COMMON_SHARED_COMPILE_FLAGS "${${COMPONENT}_SHARED_COMPILE_FLAGS} ${COMMON_SHARED_COMPILE_FLAGS} -DNOMINMAX -DUNICODE -D_UNICODE ${COMMON_WIN32_CXX_EXTRAFLAGS} /MP /W3 /GF /GS /EHsc /nologo /Zi /TP /errorReport:prompt")
set(COMMON_SHARED_COMPILE_DEBUG_FLAGS "-DDEBUG /Od /RTC1 ${${COMPONENT}_SHARED_COMPILE_DEBUG_FLAGS}")
set(COMMON_SHARED_COMPILE_RELEASE_FLAGS "-DNDEBUG /Gy ${${COMPONENT}_SHARED_COMPILE_RELEASE_FLAGS}")

if(MSVC90)
	set(COMMON_SHARED_COMPILE_DEBUG_FLAGS "${COMMON_SHARED_COMPILE_DEBUG_FLAGS}   -D_SECURE_SCL=1 -D_HAS_ITERATOR_DEBUGGING=1")
	set(COMMON_SHARED_COMPILE_RELEASE_FLAGS "${COMMON_SHARED_COMPILE_RELEASE_FLAGS} -D_SECURE_SCL=0 -D_HAS_ITERATOR_DEBUGGING=0")
else()
	set(COMMON_SHARED_COMPILE_DEBUG_FLAGS "${COMMON_SHARED_COMPILE_DEBUG_FLAGS} -D_ITERATOR_DEBUG_LEVEL=2")
	set(COMMON_SHARED_COMPILE_RELEASE_FLAGS "${COMMON_SHARED_COMPILE_RELEASE_FLAGS}  -D_ITERATOR_DEBUG_LEVEL=0")
endif()

set(COMMON_PLATFORM_LINK_WIN "${COMMON_WIN32_LINK_EXTRAFLAGS} ${${COMPONENT}_PLATFORM_LINK_WIN} /INCREMENTAL:NO /DYNAMICBASE ")
set(CMAKE_C_FLAGS "${COMMON_SHARED_COMPILE_FLAGS} ${COMMON_EXTRA_C_COMPILE_FLAGS} ")
set(CMAKE_C_FLAGS_DEBUG "${COMMON_SHARED_COMPILE_DEBUG_FLAGS}")
set(CMAKE_C_FLAGS_RELEASE "${COMMON_SHARED_COMPILE_RELEASE_FLAGS}")
set(CMAKE_CXX_FLAGS "${COMMON_SHARED_COMPILE_FLAGS} ${COMMON_EXTRA_CXX_COMPILE_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${COMMON_SHARED_COMPILE_DEBUG_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${COMMON_SHARED_COMPILE_RELEASE_FLAGS}")
set(COMMON_PLATFORM_BEGIN_WHOLE_ARCHIVE "")
set(COMMON_PLATFORM_END_WHOLE_ARCHIVE "")
set(COMMON_DYLIBEXTENSION	"dll")

if(NOT DEFINED COMMON_TOOLSET)
	if(MSVC90)
		set(COMMON_TOOLSET "${COMMON_TOOLSET} -DCOMMON_TOOLSET_VC90")
	elseif(MSVC10)
		set(COMMON_TOOLSET "${COMMON_TOOLSET} -DCOMMON_TOOLSET_VC10")
	endif()
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
	# ignore, doesn't exits on windows
endfunction(AddMacFramework)

# ==============================================================================
# Function: Set platform specific link flags to target
# ==============================================================================
#
# TODO: make function work on all platforms. Unify with AddMacFramework.
function(SetPlatformLinkFlags target linkflags libname)
	if (CMAKE_CL_64 AND ${COMPONENT}_BUILD_STATIC)
		set_target_properties(${target} PROPERTIES STATIC_LIBRARY_FLAGS "/machine:x64")
	endif()
	set_target_properties(${target} PROPERTIES LINK_FLAGS "${COMMON_PLATFORM_LINK_WIN} ${linkflags}")
	set_target_properties(${target} PROPERTIES LINK_FLAGS_RELEASE "/DEBUG /OPT:REF /OPT:ICF /PDBALTPATH:${libname}.pdb")
endfunction(SetPlatformLinkFlags)

# ==============================================================================
# Function: Set the output path depending on isExecutable
#	The output path is corrected. XCode and VS automatically add the selected target (debug/release)
# ==============================================================================
#
function(SetOutputPath2 path targetName targetType)
	# remove last path item if not makefile
	if(NOT COMMON_IS_MAKEFILE_BUILD)
		get_filename_component(correctedPath ${path} PATH)
		#message("SetOutputPath: ${path} to ${correctedPath}")
	else()
		set(correctedPath ${path})
	endif()
	
	if(${targetType} MATCHES "DYNAMIC" OR ${targetType} MATCHES "EXECUTABLE")
		set_property(TARGET ${targetName} PROPERTY RUNTIME_OUTPUT_DIRECTORY ${correctedPath})
	else()
		set_property(TARGET ${targetName} PROPERTY ARCHIVE_OUTPUT_DIRECTORY ${correctedPath})
	endif()
endfunction(SetOutputPath2)

# ==============================================================================
# Function: Set precompiled headers
# ==============================================================================
#
function( SetPrecompiledHeaders target pchHeader pchSource )
	set_target_properties( ${target} PROPERTIES COMPILE_FLAGS "/Yu${pchHeader}" )
	set_source_files_properties( ${pchSource} PROPERTIES COMPILE_FLAGS "/Yc${pchHeader}" )
endfunction( SetPrecompiledHeaders )

# ==============================================================================
# Function: Add library in native OS name to variable resultName
# ==============================================================================
#
function(AddLibraryNameInNativeFormat libname libpath resultName)
	foreach(libname_item ${libname})
		set(result "${libpath}/${libname_item}.lib")
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
		set(result "${libpath}/${libname_item}.dll")
		set(resultList ${resultList} ${result})
	endforeach()	
	set(${resultName} ${${resultName}} ${resultList} PARENT_SCOPE)
	#message("AddSharedLibraryNameInNativeFormat: ${resultList} ")
endfunction(AddSharedLibraryNameInNativeFormat)

# ==============================================================================
# Function: Make an OSX bundle
# ==============================================================================
# 	Optional parameter (!!)
#	${ARGV3} = Name of Info.plist located in ${RESOURCE_ROOT}/${COMMON_PLATFORM_SHORT}
#	${ARGV4} = Name of header file to use to generate Info.plist
#
function(MakeBundle appname extension outputDir)
	# ignore, no bundle on windows
endfunction(MakeBundle)
