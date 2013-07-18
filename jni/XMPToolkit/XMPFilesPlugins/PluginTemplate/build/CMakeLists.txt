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

SET(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Configurations" FORCE)
# Enable folder grouping of projects in IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ==============================================================================
# This project
# ==============================================================================
set(PRODUCT_NAME PluginTemplate)
set(TARGET_NAME ${PRODUCT_NAME})

if(CMAKE_CL_64)
	project(${TARGET_NAME}64)
    set(BITS 64)
else()
	project(${TARGET_NAME})
    set(BITS 32)
endif()

# ==============================================================================
# Shared config
# ==============================================================================
# setup some values before calling shared config
set(XMP_THIS_PROJECT_RELATIVEPATH "../../..")

include(${CMAKE_CURRENT_SOURCE_DIR}/${XMP_THIS_PROJECT_RELATIVEPATH}/build/XMP_Config.cmake)

# ==============================================================================
# Add or overwite shared config variables
# ==============================================================================
# Windows specific
if(WIN32)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_WINDOWS")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WINDOWS")
	SET(CMAKE_EXE_LINKER_FLAGS /NODEFAULTLIB:LIBCMT)
endif()

# all platform
set(CXX_FLAGS_DEBUG "-D_DEBUG=1 -DDEBUG=1")
set(CXX_FLAGS_RELEASE "-DNDEBUG=1")

# ==============================================================================
# Locations
# ==============================================================================
SetPluginOutputPath()

# ==============================================================================
# For convenience we define the sources as a variable. You can add 
# header files and cpp/c files and CMake will sort them out
# ==============================================================================

AddPluginApiFiles("../../..")

list (APPEND COMMON_SOURCE_FILES ${XMPROOT_DIR}/source/UnicodeConversions.cpp)
list (APPEND COMMON_SOURCE_FILES ${XMPROOT_DIR}/source/PerfUtils.cpp)
list (APPEND COMMON_SOURCE_FILES ${XMPROOT_DIR}/source/XMP_ProgressTracker.cpp)
if(UNIX)
	list(APPEND COMMON_SOURCE_FILES ${XMPROOT_DIR}/source/Host_IO-POSIX.cpp)
else()
	list(APPEND COMMON_SOURCE_FILES ${XMPROOT_DIR}/source/Host_IO-Win.cpp)
endif()
source_group("Source Files\\common" FILES ${COMMON_SOURCE_FILES})

list(APPEND SOURCE_FILES
	${SOURCE_ROOT}/Template_Handler.cpp
	)
source_group("Source Files" FILES ${SOURCE_FILES})

# resources
AddPluginResourceFiles()

# merge list elements
list(APPEND SOURCE_FILES ${API_INCLUDE_FILES} ${API_SOURCE_FILES} ${COMMON_SOURCE_FILES} ${SOURCE_FILES} ${RESOURCE_FILES})

# include directories
include_directories(
	${XMPROOT_DIR}
	${XMPROOT_DIR}/public/include
)
if(UNIX)
	if(APPLE)
		set(XMPCORE_LIB "XMPCoreStatic")
	else(APPLE)
		set(XMPCORE_LIB "staticXMPCore")
	endif(APPLE)
else(UNIX)
	if(WIN32)
		set(XMPCORE_LIB "XMPCoreStatic")
	endif(WIN32)
endif(UNIX)
#setting up XMP_BUILDMODE_DIR variable
SetupInternalBuildDirectory()
#link directories
link_directories(${XMPROOT_DIR}/public/libraries/${XMP_PLATFORM_FOLDER})
# ==============================================================================
# Define what to do, lib, exe, etc
set(DEPENDENCY_LIST "LIB:$(XMPCORE_LIB)")
AddLibraryAndDependencies("${TARGET_NAME}" "${XMP_BUILD_STATIC}" "YES" "SHARED" "SOURCE_FILES" "DEPENDENCY_LIST")
# ==============================================================================
# Link dependencies

if(WIN32)
	target_link_libraries(
		${TARGET_NAME}
		${XMPCORE_LIB}
		${XMP_PLATFORM_LINK}
	)
else(WIN32)
	if(UNIX AND NOT APPLE)
		target_link_libraries(
			${TARGET_NAME}
			${XMPROOT_DIR}/public/libraries/${XMP_PLATFORM_FOLDER}/${XMP_BUILDMODE_DIR}/${XMPCORE_LIB}.ar 
			${XMP_PLATFORM_LINK}
		)
	else(UNIX AND NOT APPLE)
		target_link_libraries(
			${TARGET_NAME}
			${XMPCORE_LIB}
		)
	endif()
endif()

set(FRAMEWORK_LIST "Mac:CoreFoundation" "Mac:Foundation" "Mac:CoreServices"  "Mac:${XMP_PLATFORM_LINK}")
AddMacFramework(${TARGET_NAME} FRAMEWORK_LIST)


SetWinLinkFlags(${TARGET_NAME} "" "${TARGET_NAME}")

if(APPLE)
	set_target_properties(${TARGET_NAME} PROPERTIES BUILD_WITH_INSTALL_RPATH ON INSTALL_NAME_DIR "@executable_path/../Frameworks")
endif()

set(DEPEND_LIBS "")
CreatePlugin(${TARGET_NAME} ${XMPPLUGIN_OUTPUT_DIR} "${PROJECT_ROOT}/resource/txt/*.txt" XMP_PLUGIN_DEPENDENCIES DEPEND_LIBS)

# ==============================================================================
# Post build
# ==============================================================================

if (UNIX)
	if(APPLE)
		add_custom_command (TARGET ${TARGET_NAME} POST_BUILD 
			COMMAND cp -f ${PROJECT_ROOT}/resource/txt/MODULE_IDENTIFIER.txt ${XMPPLUGIN_OUTPUT_DIR}/${TARGET_NAME}.xpi/Versions/A/Resources/MODULE_IDENTIFIER.txt
			COMMAND cp -f ${PROJECT_ROOT}/resource/txt/XMPPLUGINUIDS-${BITS}.txt ${XMPPLUGIN_OUTPUT_DIR}/${TARGET_NAME}.xpi/Versions/A/Resources/XMPPLUGINUIDS.txt
			COMMENT "Copy resource")
	else()
		if((${CMAKE_BUILD_TYPE} MATCHES "Debug") OR (${CMAKE_BUILD_TYPE} MATCHES "debug") )
			add_custom_command (TARGET ${TARGET_NAME}
				POST_BUILD
				COMMAND ls -l ${XMPPLUGIN_OUTPUT_DIR}/${TARGET_NAME}.xpi
			)
		else()
			add_custom_command (TARGET ${TARGET_NAME}
				POST_BUILD
				COMMAND strip ${XMPPLUGIN_OUTPUT_DIR}/${TARGET_NAME}.xpi
				COMMAND ls -l ${XMPPLUGIN_OUTPUT_DIR}/${TARGET_NAME}.xpi
			)
		endif()
		add_custom_command (TARGET ${TARGET_NAME} POST_BUILD 
			COMMAND mkdir -p ${XMPPLUGIN_OUTPUT_DIR}/${TARGET_NAME}.resources
			COMMAND cp -f ${PROJECT_ROOT}/resource/txt/MODULE_IDENTIFIER.txt ${XMPPLUGIN_OUTPUT_DIR}/${TARGET_NAME}.resources/MODULE_IDENTIFIER.txt
			COMMAND cp -f ${PROJECT_ROOT}/resource/txt/XMPPLUGINUIDS-${BITS}.txt ${XMPPLUGIN_OUTPUT_DIR}/${TARGET_NAME}.resources/XMPPLUGINUIDS.txt
			COMMENT "Copy resource")
	endif()
endif()
# ==============================================================================
# configuration feedback
# ==============================================================================

message (STATUS "===========================================================================")
message (STATUS " ${TARGET_NAME}		")
message (STATUS "===========================================================================")

