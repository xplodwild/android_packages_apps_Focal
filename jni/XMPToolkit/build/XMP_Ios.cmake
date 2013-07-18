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
# Shared config for ios
# ==============================================================================


set(XMP_PLATFORM_SHORT "ios")
	
set(CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD_32_BIT)")

add_definitions(-DIOS_ENV=1)

set(XMP_ENABLE_SECURE_SETTINGS "ON")

# shared flags
set(XMP_SHARED_COMPILE_FLAGS "${XMP_SHARED_COMPILE_FLAGS}  ${XMP_EXTRA_COMPILE_FLAGS}")
set(XMP_SHARED_COMPILE_DEBUG_FLAGS "-O0 -DDEBUG=1 -D_DEBUG=1")
set(XMP_SHARED_COMPILE_RELEASE_FLAGS "-Os -DNDEBUG=1 -D_NDEBUG=1")

set(XMP_PLATFORM_BEGIN_WHOLE_ARCHIVE "")
set(XMP_PLATFORM_END_WHOLE_ARCHIVE "")

# ==============================================================================
# Function: architecture related settings
# ==============================================================================
function(SetupTargetArchitecture)
	if(APPLE_IOS)
		set(XMP_CPU_FOLDERNAME	"$(ARCHS_STANDARD_32_BIT)" PARENT_SCOPE)
	else()
		if(CMAKE_CL_64)
			set(XMP_BITDEPTH		"64" PARENT_SCOPE)
			set(XMP_CPU_FOLDERNAME	"intel_64" PARENT_SCOPE)
		else()
			set(XMP_BITDEPTH		"32" PARENT_SCOPE)
			set(XMP_CPU_FOLDERNAME	"intel" PARENT_SCOPE)
		endif()
	endif()
endfunction(SetupTargetArchitecture)

SetupTargetArchitecture()
# XMP_PLATFORM_FOLDER is used in OUTPUT_DIR and Debug/Release get automatically added for VS/XCode projects
set(XMP_PLATFORM_FOLDER "ios/${XMP_CPU_FOLDERNAME}")

# ==============================================================================
# Function: Set internal build target directory. See XMP_PLATFORM_FOLDER for 
# further construction and when CMAKE_CFG_INTDIR/CMAKE_BUILD_TYPE are set 
# (generator dependent).
# ==============================================================================
function(SetupInternalBuildDirectory)
	if(CMAKE_CFG_INTDIR STREQUAL ".")
		# CMAKE_BUILD_TYPE only available for makefile builds
		set(XMP_IS_MAKEFILE_BUILD	"ON" PARENT_SCOPE)
		if((${CMAKE_BUILD_TYPE} MATCHES "Debug") OR (${CMAKE_BUILD_TYPE} MATCHES "debug") )
			set(XMP_BUILDMODE_DIR "debug" PARENT_SCOPE)
		else()
			set(XMP_BUILDMODE_DIR "release" PARENT_SCOPE)
		endif()
	else()
		# Visual/XCode have dedicated Debug/Release target modes. CMAKE_CFG_INTDIR is set.
		if(APPLE_IOS)
# TODO: fixme
#			set(XMP_BUILDMODE_DIR "$(CONFIGURATION)$(EFFECTIVE_PLATFORM_NAME)" PARENT_SCOPE)
			set(XMP_BUILDMODE_DIR "$(CONFIGURATION)" PARENT_SCOPE)
		else()
			set(XMP_BUILDMODE_DIR ${CMAKE_CFG_INTDIR} PARENT_SCOPE)
		endif()
		set(XMP_IS_MAKEFILE_BUILD	"OFF" PARENT_SCOPE)
	endif()
endfunction(SetupInternalBuildDirectory)

# ==============================================================================
# Function: Setup general and specific folder structures
# ==============================================================================
function(SetupGeneralDirectories)
	set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR} PARENT_SCOPE)
	set(PROJECT_ROOT ${PROJECT_SOURCE_DIR} PARENT_SCOPE)
	set(SOURCE_ROOT ${PROJECT_SOURCE_DIR}/../source PARENT_SCOPE)
	set(RESOURCE_ROOT ${PROJECT_SOURCE_DIR}/../resource PARENT_SCOPE)
	
	# ==============================================================================
	# XMP specific defines
	# ==============================================================================
	
	# Construct output directory
	# The CMAKE_CFG_INTDIR gets automatically set for generators which differenciate different build targets (eg. VS, XCode), but is emtpy for Linux !
	# In this case Debug/Release is added to the OUTPUT_DIR when used for LIBRARY_OUTPUT_PATH.
	string(TOLOWER ${XMP_BUILDMODE_DIR} LOWERCASE_XMP_BUILDMODE_DIR)
	set(OUTPUT_DIR ${PROJECT_SOURCE_DIR}/${XMP_THIS_PROJECT_RELATIVEPATH}/public/libraries/${XMP_PLATFORM_FOLDER}/${LOWERCASE_XMP_BUILDMODE_DIR} PARENT_SCOPE)

	# XMP lib locations constructed with ${XMP_PLATFORM_FOLDER}/${XMP_BUILDMODE_DIR}, since the target folder (Debug/Release) isn't added automatically
	set(XMPROOT_DIR ${PROJECT_SOURCE_DIR}/${XMP_THIS_PROJECT_RELATIVEPATH} PARENT_SCOPE)
	set(XMPLIBRARIES_DIR ${PROJECT_SOURCE_DIR}/${XMP_THIS_PROJECT_RELATIVEPATH}/public/libraries/${XMP_PLATFORM_FOLDER}/${LOWERCASE_XMP_BUILDMODE_DIR} PARENT_SCOPE)
	set(XMPPLUGIN_DIR ${PROJECT_SOURCE_DIR}/${XMP_THIS_PROJECT_RELATIVEPATH}/XMPPlugin PARENT_SCOPE)
	set(XMPPLUGIN_OUTPUT_DIR ${PROJECT_SOURCE_DIR}/${XMP_THIS_PROJECT_RELATIVEPATH}/XMPFilesPlugins/public/${XMP_PLATFORM_FOLDER}/${XMP_BUILDMODE_DIR} PARENT_SCOPE)

	# each plugin project should implement function named SetupProjectSpecifiedDirectories to specify the directories used by project itself
endfunction(SetupGeneralDirectories)