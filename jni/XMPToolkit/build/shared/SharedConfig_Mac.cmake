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
# Shared config for mac
# ==============================================================================
DetectXCodeVersion()

if(${CMAKE_INSTALLED_XCODE_VERSION} LESS 4.3.0)
	if(NOT DEFINED ${COMPONENT}_DEVELOPER_ROOT)
		set(${COMPONENT}_DEVELOPER_ROOT "/Developer")
	endif()
	
	set(CMAKE_OSX_DEPLOYMENT_TARGET 	${${COMPONENT}_OSX_TARGET})
else()
	# get xcode installation path
	execute_process(
			  COMMAND xcode-select -print-path
			  OUTPUT_VARIABLE ${COMPONENT}_DEVELOPER_ROOT
			  OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	set(CMAKE_OSX_DEPLOYMENT_TARGET 	${${COMPONENT}_OSX_TARGET})
   	set(${COMPONENT}_DEVELOPER_ROOT "${${COMPONENT}_DEVELOPER_ROOT}/Platforms/MacOSX.platform/Developer")
endif()

set(CMAKE_OSX_SYSROOT "${${COMPONENT}_DEVELOPER_ROOT}/SDKs/MacOSX${${COMPONENT}_OSX_SDK}.sdk")



#
# shared flags
#
set(COMMON_SHARED_COMPILE_FLAGS "${${COMPONENT}_SHARED_COMPILE_FLAGS} -Wno-unused-variable -Wno-unused-function -Wno-unused-parameter")
set(COMMON_SHARED_COMPILE_DEBUG_FLAGS "-O0 -DDEBUG=1 -D_DEBUG=1")
set(COMMON_SHARED_COMPILE_RELEASE_FLAGS "-O3 -DNDEBUG=1 -D_NDEBUG=1")

set(CMAKE_C_FLAGS "${COMMON_SHARED_COMPILE_FLAGS} ${COMMON_EXTRA_C_COMPILE_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG "${COMMON_SHARED_COMPILE_DEBUG_FLAGS}")
set(CMAKE_C_FLAGS_RELEASE "${COMMON_SHARED_COMPILE_RELEASE_FLAGS}")

set(COMMON_SHARED_CXX_COMPILE_FLAGS "${${COMPONENT}_SHARED_CXX_COMPILE_FLAGS} -Wnon-virtual-dtor -Woverloaded-virtual -Wno-unused-variable -Wno-unused-function -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS "-funsigned-char -fshort-enums -fno-common ${COMMON_SHARED_CXX_COMPILE_FLAGS} ${COMMON_EXTRA_CXX_COMPILE_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${COMMON_SHARED_COMPILE_DEBUG_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${COMMON_SHARED_COMPILE_RELEASE_FLAGS}")

if(${COMPONENT}_ENABLE_SECURE_SETTINGS)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector -D_FORTIFY_SOURCE=2")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector")
endif()
set(COMMON_PLATFORM_BEGIN_WHOLE_ARCHIVE "")
set(COMMON_PLATFORM_END_WHOLE_ARCHIVE "")
set(COMMON_DYLIBEXTENSION	"dylib")


find_program(GCCTOOL gcc HINTS "/usr/bin" "${OSX_DEVELOPER_ROOT}/usr/bin")
if (${GCCTOOL} STREQUAL "GCCTOOL-NOTFOUND")
	message(SEND_ERROR "Can't find gcc in ${OSX_DEVELOPER_ROOT}/usr/bin")
endif()

SetupCompilerFlags()
SetupGeneralDirectories()

# ==============================================================================
# Functions
# ==============================================================================

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
# Function: Add Mac framework to list of libraries to link to
# ==============================================================================
#
function(AddMacFramework targetName frameworkList)
	#message("AddMacFramework ${targetName}: ${${frameworkList}}")	
	foreach(fwPair ${${frameworkList}})
		string(REGEX MATCH "(ALL|ios|Mac):(.+)" matches ${fwPair})
		
		set(scope ${CMAKE_MATCH_1})
		set(framework "-framework ${CMAKE_MATCH_2}")
		
		if(${scope} MATCHES "ALL")
			set(collectedFrameworks "${collectedFrameworks} ${framework}")
		elseif(${scope} MATCHES "ios")
			if(APPLE_IOS)
				set(collectedFrameworks "${collectedFrameworks} ${framework}")
			endif()
		elseif(${scope} MATCHES "Mac")
			if(NOT APPLE_IOS)
				set(collectedFrameworks "${collectedFrameworks} ${framework}")
			endif()
		endif()
	endforeach()
	
	if(collectedFrameworks)
		#message("-- collectedFrameworks: ${collectedFrameworks}")
		set_property(TARGET ${targetName} PROPERTY LINK_FLAGS ${collectedFrameworks})
	endif()
endfunction(AddMacFramework)

# ==============================================================================
# Function: Add shared library in native OS name to variable resultName
# ==============================================================================
#
function(AddSharedLibraryNameInNativeFormat libname libpath resultName)
	foreach(libname_item ${libname})
		set(result "${libpath}/lib${libname_item}.dylib")
		set(resultList ${resultList} ${result})
	endforeach()	
	set(${resultName} ${${resultName}} ${resultList} PARENT_SCOPE)
	#message("AddSharedLibraryNameInNativeFormat: ${resultList} ")
endfunction(AddSharedLibraryNameInNativeFormat)

# ==============================================================================
# Function: Make an OSX bundle
# ==============================================================================
# 	Optional parameter (!!)
#	${ARGV3} = Name of Info.plist located in ${RESOURCE_ROOT}/${XMP_PLATFORM_SHORT}
#	${ARGV4} = Name of header file to use to generate Info.plist
#
function(MakeBundle appname extension outputDir)
	# force no extension to make below bundle creation work
	set(CMAKE_SHARED_MODULE_PREFIX "" PARENT_SCOPE)
	set(CMAKE_SHARED_MODULE_SUFFIX "" PARENT_SCOPE)

	# are optional parameter set?
	if(ARGV3 AND ARGV4)
		message("Setup prebuild to compile Info.plist for ${appname}")
		set(infoPlist ${ARGV3})
		set(infoPlistHeader ${ARGV4})

		# skip generation of this project if gcc not found
		find_program(GCCTOOL gcc HINTS "/usr/bin" "${OSX_DEVELOPER_ROOT}/usr/bin")
		if (${GCCTOOL} STREQUAL "GCCTOOL-NOTFOUND")
			message(SEND_ERROR "Can't find gcc in ${OSX_DEVELOPER_ROOT}/usr/bin")
		endif()

		# preprocess Info.plist
		# env OTHER_CFLAGS contains proper debug/release preprocessor defines
		#add_custom_command (TARGET ${appname} PRE_BUILD 
		#						COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
		#						COMMAND ${GCCTOOL} -E -P -x c ${RESOURCE_ROOT}/${XMP_PLATFORM_SHORT}/${infoPlist}
		#						-F${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/ -DPRODUCT_NAME=${appname} "$(OTHER_CFLAGS)"
		#						-include ${RESOURCE_ROOT}/${XMP_PLATFORM_SHORT}/${infoPlistHeader} -o ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/Info.plist
		#						COMMENT "Preprocessing Info-plist")
	
		# define extra command
		#set(extraCmd cp -f ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/Info.plist ${outputDir}/${appname}.${extension}/Contents/Info.plist)
	endif()
	
	# make bundle
	add_custom_command (TARGET ${appname} POST_BUILD 
						COMMAND mkdir -p ${outputDir}/${appname}.${extension}
						COMMAND rm -rf ${outputDir}/${appname}.${extension}/*
						#COMMAND cp -f ${PROJECT_SOURCE_DIR}/${XMP_THIS_PROJECT_RELATIVEPATH}/toolkit/build/mac/PkgInfo ${outputDir}/${appname}.${extension}/Contents/PkgInfo
						#COMMAND ${extraCmd}
						COMMAND cp -Rf ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${appname}.framework/* ${outputDir}/${appname}.${extension}/
						COMMAND cp -rf ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${appname}.framework.dSYM ${outputDir}/${appname}.${extension}.dSYM
						COMMENT "Make bundle")
endfunction(MakeBundle)

# ==============================================================================
# Function: Set the output path depending on isExecutable
#	The output path is corrected. 
# ==============================================================================
#
function(SetOutputPath2 path targetName targetType)
	# remove last path item if not makefile
	if(NOT ${COMPONENT}_IS_MAKEFILE_BUILD)
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
# Function: Set platform specific link flags to target
# ==============================================================================
#
# TODO: make function work on all platforms. Unify with AddMacFramework.
function(SetPlatformLinkFlags target linkflags libname)
	# set additional XCode attribute for Mac
	if (CMAKE_INSTALLED_XCODE_VERSION GREATER 4.4.9)
		set(linkflags "${linkflags} -Xlinker -no_data_in_code_info")
	endif()
	set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS " ${linkflags}")
	set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "YES")
	set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
	set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_GCC_ENABLE_PASCAL_STRINGS "NO")

	# HACK: remove warnings that (CMake 2.8.10) sets by default: -Wmost -Wno-four-char-constants -Wno-unknown-pragmas
	set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_WARNING_CFLAGS "")

	# Currently (CMake 2.8.4) there is no built-in way to strip symbols just for Release builds.
	# Ideally we need configuration based XCODE_ATTRIBUTEs, e.g.
	# set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_DEPLOYMENT_POSTPROCESSING_RELEASE "YES")
	# set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT_RELEASE "YES")
	#
	# Add custom script to be called post link to strip release builds
	set(SCRIPT_NAME CustomStrip.sh)
	find_file(CUSTOM_POST_LINK_SCRIPT ${SCRIPT_NAME} PATHS ${PROJECT_SOURCE_DIR}/${${COMPONENT}_THIS_PROJECT_RELATIVEPATH}/toolkit/build/mac)

	if (CUSTOM_POST_LINK_SCRIPT)
		add_custom_command (TARGET ${target} POST_BUILD 
							COMMAND /bin/bash ${CUSTOM_POST_LINK_SCRIPT}
							COMMENT "Call ${SCRIPT_NAME}")
	endif()
endfunction(SetPlatformLinkFlags)

# ==============================================================================
# Function: Set precompiled headers
# ==============================================================================
#
function( SetPrecompiledHeaders target pchHeader pchSource )
	set_target_properties( ${target} PROPERTIES XCODE_ATTRIBUTE_GCC_PREFIX_HEADER ${pchHeader} )
	set_target_properties( ${target} PROPERTIES XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER "YES" )
endfunction( SetPrecompiledHeaders )
