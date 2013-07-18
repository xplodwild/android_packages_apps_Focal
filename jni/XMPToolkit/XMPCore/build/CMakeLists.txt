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
set(TARGET_NAME XMPCore)
if(CMAKE_CL_64)
	project(${TARGET_NAME}64)
else(CMAKE_CL_64)
	project(${TARGET_NAME})
endif(CMAKE_CL_64)

set(STATIC_STR "Static")
if (XMP_BUILD_STATIC)
	set(TARGET_NAME "${TARGET_NAME}${STATIC_STR}")
	set(PROJECT_LABEL_STR "${TARGET_NAME}")
else(XMP_BUILD_STATIC)
	set(PROJECT_LABEL_STR "${TARGET_NAME}")
endif(XMP_BUILD_STATIC)

# ==============================================================================
# Shared config
# ==============================================================================
# setup some values before calling shared config
set(XMP_THIS_PROJECT_RELATIVEPATH "../..")

include(${CMAKE_CURRENT_SOURCE_DIR}/${XMP_THIS_PROJECT_RELATIVEPATH}/build/XMP_Config.cmake)

# ==============================================================================
# Project specific config starts here
# ==============================================================================
if(UNIX)
	if(APPLE)
		# OSX -------------------------------------------
		if(XMP_BUILD_STATIC)
			set(XMPCORE_LIB "XMPCoreStatic")
		else(XMP_BUILD_STATIC)
			set(XMPCORE_LIB "XMPCore")
		endif()
	else(APPLE)
		# Linux -------------------------------------------
		if(XMP_BUILD_STATIC)
			set(XMPCORE_LIB "staticXMPCore")
		else(XMP_BUILD_STATIC)
			set(XMPCORE_LIB "XMPCore")
		endif()
	endif(APPLE)
else(UNIX)
	if(WIN32)
		# Windows -------------------------------------------
		if(XMP_BUILD_STATIC)
			set(XMPCORE_LIB "XMPCoreStatic")
		else(XMP_BUILD_STATIC)
			set(XMPCORE_LIB "XMPCore")
		endif()
	endif(WIN32)
endif(UNIX)

# ==============================================================================
# For convenience we define the sources as a variable. You can add 
# header files and cpp/c files and CMake will sort them out
# ==============================================================================

list (APPEND INTERNAL_HEADER_FILES
	${XMPROOT_DIR}/source/ExpatAdapter.hpp
	${XMPROOT_DIR}/source/UnicodeConversions.hpp
	${XMPROOT_DIR}/source/UnicodeInlines.incl_cpp
	${XMPROOT_DIR}/source/XMLParserAdapter.hpp
	${XMPROOT_DIR}/build/XMP_BuildInfo.h
	${SOURCE_ROOT}/XMPCore_Impl.hpp
	${SOURCE_ROOT}/XMPIterator.hpp
	${SOURCE_ROOT}/XMPMeta.hpp
	${SOURCE_ROOT}/XMPUtils.hpp
	)
source_group("Header Files\\Internal Headers" FILES ${INTERNAL_HEADER_FILES})

file (GLOB PUBLIC_CLIENTGLUE_HEADER_FILES ${XMPROOT_DIR}/public/include/client-glue/*.*)
list (REMOVE_ITEM PUBLIC_CLIENTGLUE_HEADER_FILES
	${XMPROOT_DIR}/public/include/client-glue/TXMPFiles.incl_cpp
	${XMPROOT_DIR}/public/include/client-glue/WXMPFiles.hpp
	)
source_group("Header Files\\Public Headers\\Client Glue" FILES ${PUBLIC_CLIENTGLUE_HEADER_FILES})

file (GLOB  PUBLIC_HEADER_FILES ${XMPROOT_DIR}/public/include/*.*)
list (REMOVE_ITEM PUBLIC_HEADER_FILES
	${XMPROOT_DIR}/public/include/TXMPFiles.hpp
	${XMPROOT_DIR}/public/include/XMP_IO.hpp
	)
source_group("Header Files\\Public Headers" FILES ${PUBLIC_HEADER_FILES})

file (GLOB ABIWRAPPER_SOURCE_FILES ${SOURCE_ROOT}/WXMP*.cpp)
source_group("Source Files\\ABI Wrappers" FILES ${ABIWRAPPER_SOURCE_FILES})

#XMP Core classes
file (GLOB CORECLASS_SOURCE_FILES ${SOURCE_ROOT}/XMP*.cpp)
list (REMOVE_ITEM CORECLASS_SOURCE_FILES ${SOURCE_ROOT}/XMPCore_Impl.cpp)
source_group("Source Files\\Core Classes" FILES ${CORECLASS_SOURCE_FILES})

file (GLOB THIRDPARTY_SOURCE_FILES ${XMPROOT_DIR}/third-party/zuid/interfaces/*.cpp)
list (APPEND THIRDPARTY_SOURCE_FILES
	${PROJECT_ROOT}/../../third-party/expat/lib/xmlparse.c
	${PROJECT_ROOT}/../../third-party/expat/lib/xmlrole.c
	${PROJECT_ROOT}/../../third-party/expat/lib/xmltok.c
	)
list (APPEND THIRDPARTY_SOURCE_FILES ${RESOURCE_ROOT}/${XMP_PLATFORM_SHORT}/expat_config.h)
source_group("Source Files\\Third Party" FILES ${THIRDPARTY_SOURCE_FILES})

list (APPEND UTILITY_SOURCE_FILES
	${SOURCE_ROOT}/ExpatAdapter.cpp
	${SOURCE_ROOT}/ParseRDF.cpp
	${SOURCE_ROOT}/XMPCore_Impl.cpp
	${XMPROOT_DIR}/source/UnicodeConversions.cpp
	${XMPROOT_DIR}/source/XML_Node.cpp
	${XMPROOT_DIR}/source/XMP_LibUtils.cpp
	)
source_group("Source Files\\Utilities" FILES ${UTILITY_SOURCE_FILES})

#resource files
file (GLOB RESOURCE_FILES ${RESOURCE_ROOT}/${XMP_PLATFORM_SHORT}/*.*)
if(WIN32 AND ${XMP_BUILD_STATIC})
	list(REMOVE_ITEM RESOURCE_FILES ${RESOURCE_ROOT}/${XMP_PLATFORM_SHORT}/${TARGET_NAME}.rc)
endif()
source_group("Resource Files" FILES ${RESOURCE_FILES})

list(APPEND SOURCE_FILES 
	${PUBLIC_CLIENTGLUE_HEADER_FILES}
	${PUBLIC_HEADER_FILES}
	${INTERNAL_HEADER_FILES}
	${ABIWRAPPER_SOURCE_FILES}
	${ABIWRAPPER_SOURCE_FILES}
	${CORECLASS_SOURCE_FILES}
	${THIRDPARTY_SOURCE_FILES}
	${UTILITY_SOURCE_FILES}
	${RESOURCE_FILES}
	)
# include directories
include_directories(${XMPROOT_DIR})
include_directories(${XMPROOT_DIR}/public/include)
include_directories(${XMPROOT_DIR}/third-party/expat/public/lib)
include_directories(${RESOURCE_ROOT}/${XMP_PLATFORM_SHORT})



# ==============================================================================
# Define what to do, lib, exe, etc 
if (UNIX AND APPLE AND NOT ${XMP_BUILD_STATIC})
	# preprocess Info.plist
	#if (${CMAKE_BUILD_TYPE} strequal "Debug")
	if((${CMAKE_BUILD_TYPE} MATCHES "Debug") OR (${CMAKE_BUILD_TYPE} MATCHES "debug"))
			set (INFO_PLIST_FLAGS "-DDEBUG=1")
	else()
		set (INFO_PLIST_FLAGS "-DNDEBUG=1")
	endif()
	
	add_custom_target(${TARGET_NAME}InfoPlist
		COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
		COMMAND if [ $(CONFIGURATION) != Debug ]; then
			${GCCTOOL} -E -P -x c ${RESOURCE_ROOT}/${XMP_PLATFORM_SHORT}/${TARGET_NAME}.plist
			-F${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/
			-DPRODUCT_NAME=${TARGET_NAME} -DMAC_ENV=1 -DNDEBUG=1
			-include ${RESOURCE_ROOT}/${XMP_PLATFORM_SHORT}/${TARGET_NAME}PList.h
			-o ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/Info.plist
			\; else
			${GCCTOOL} -E -P -x c ${RESOURCE_ROOT}/${XMP_PLATFORM_SHORT}/${TARGET_NAME}.plist
			-F${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/
			-DPRODUCT_NAME=${TARGET_NAME} -DMAC_ENV=1 -DDEBUG=1
			-include ${RESOURCE_ROOT}/${XMP_PLATFORM_SHORT}/${TARGET_NAME}PList.h
			-o ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/Info.plist
			\; fi
		COMMAND rm -f ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/../CMakeFiles/${TARGET_NAME}.dir/Info.plist
		COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/Info.plist ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/../CMakeFiles/${TARGET_NAME}.dir/Info.plist
		COMMENT "Preprocessing Info-plist"
	)
	set(DEPENDENCY_LIST "ALL:${TARGET_NAME}InfoPlist")
else ()
	set(DEPENDENCY_LIST "")
endif()

AddLibraryAndDependencies(${TARGET_NAME} ${XMP_BUILD_STATIC} YES "SHARED" SOURCE_FILES DEPENDENCY_LIST)

#set the output name
set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME  ${XMPCORE_LIB})

# ==============================================================================
# Link dependencies
set(FRAMEWORK_LIST "Mac:CoreFoundation" "Mac:CoreServices")
AddMacFramework(${TARGET_NAME} FRAMEWORK_LIST)

if(WIN32)
	SetWinLinkFlags(${TARGET_NAME} "rpcrt4.lib" "${XMPCORE_LIB}")
else(WIN32)
	if(UNIX AND NOT APPLE)
		SetWinLinkFlags(${TARGET_NAME} "-Xlinker --version-script -Xlinker \"${RESOURCE_ROOT}/${XMP_PLATFORM_SHORT}/${TARGET_NAME}.exp\"" "")
	else()
		set_target_properties(${TARGET_NAME} PROPERTIES BUILD_WITH_INSTALL_RPATH ON INSTALL_NAME_DIR "@executable_path/../Frameworks")
		SetWinLinkFlags(${TARGET_NAME} "-exported_symbols_list \"${RESOURCE_ROOT}/${XMP_PLATFORM_SHORT}/XMPCore.exp\"" "${XMPCORE_LIB}")
	endif()
endif()

# ==============================================================================
# Define output
SetOutputPath(${OUTPUT_DIR} 0)

# ==============================================================================
# Post build
# ==============================================================================
if(UNIX)
	if (NOT APPLE)
		if (${XMP_BUILD_STATIC})
			add_custom_command (TARGET ${TARGET_NAME}
				POST_BUILD
				COMMAND  mv ${OUTPUT_DIR}/lib${XMPCORE_LIB}.a  ${OUTPUT_DIR}/${XMPCORE_LIB}.ar
			)
		else()
			if((${CMAKE_BUILD_TYPE} MATCHES "Debug") OR (${CMAKE_BUILD_TYPE} MATCHES "debug") )
				add_custom_command (TARGET ${TARGET_NAME}
					POST_BUILD
					COMMAND ls -l ${OUTPUT_DIR}/lib${XMPCORE_LIB}.so
				)
			else()
				add_custom_command (TARGET ${TARGET_NAME}
					POST_BUILD
					COMMAND strip ${OUTPUT_DIR}/lib${XMPCORE_LIB}.so
					COMMAND ls -l ${OUTPUT_DIR}/lib${XMPCORE_LIB}.so
				)
			endif()
		endif()
	endif()

else()
	set_target_properties(${TARGET_NAME} PROPERTIES PROJECT_LABEL ${PROJECT_LABEL_STR})	
endif()

message (STATUS "===========================================================================")
message (STATUS " ${PROJECT_NAME} ")
message (STATUS "===========================================================================")
message (STATUS " OUTPUT_DIR = ${OUTPUT_DIR}")


