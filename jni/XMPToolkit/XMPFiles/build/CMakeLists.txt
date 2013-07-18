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
set(TARGET_NAME XMPFiles)
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
# platform specific config
# ==============================================================================
if(UNIX)
	if(APPLE)
		# OSX -------------------------------------------
		if(XMP_BUILD_STATIC)
			set(XMPFILES_LIB "XMPFilesStatic")
		else()
			set(XMPFILES_LIB "XMPFiles")
		endif()
	else(APPLE)
		# Linux -------------------------------------------
		if(XMP_BUILD_STATIC)
			set(XMPFILES_LIB "staticXMPFiles")
		else()
			set(XMPFILES_LIB "XMPFiles")
		endif()
	endif(APPLE)
else(UNIX)
	if(WIN32)
		# Windows -------------------------------------------
		if(XMP_BUILD_STATIC)
			set(XMPFILES_LIB "XMPFilesStatic")
		else(XMP_BUILD_STATIC)
			set(XMPFILES_LIB "XMPFiles")
		endif()
	endif(WIN32)
endif(UNIX)

# ==============================================================================
# For convenience we define the sources as a variable. You can add 
# header files and cpp/c files and CMake will sort them out
# ==============================================================================

#file (GLOB INTERNAL_HEADER_FILES ${PRODUCT_ROOT}/source/*.hpp ${PRODUCT_ROOT}/source/*.incl_cpp ${PRODUCT_ROOT}/build/*.h)
file (GLOB INTERNAL_HEADER_COMMONCODE_NMDS ${PRODUCT_ROOT}/XMPFiles/source/NativeMetadataSupport/*.h)
source_group("Header Files\\Internal Headers\\Common Code\\NativeMetadataSupport" FILES ${INTERNAL_HEADER_COMMONCODE_NMDS})

list (APPEND INTERNAL_HEADER_COMMONCODE
	${XMPROOT_DIR}/source/Endian.h
	${XMPROOT_DIR}/build/XMP_BuildInfo.h
	${SOURCE_ROOT}/XMPFiles.hpp
	${SOURCE_ROOT}/XMPFiles_Impl.hpp
	)
source_group("Header Files\\Internal Headers\\Common Code" FILES ${INTERNAL_HEADER_COMMONCODE})

file (GLOB INTERNAL_HEADER_FILEHANDLERS ${SOURCE_ROOT}/FileHandlers/*.hpp)
list (REMOVE_ITEM INTERNAL_HEADER_FILEHANDLERS
	${SOURCE_ROOT}/FileHandlers/AIFF_Handler.hpp
	${SOURCE_ROOT}/FileHandlers/GIF_Handler.hpp
	)
source_group("Header Files\\Internal Headers\\File Handlers" FILES ${INTERNAL_HEADER_FILEHANDLERS})

file (GLOB INTERNAL_HEADER_FORMATSUPPORT_AIFF ${SOURCE_ROOT}/FormatSupport/AIFF/*.h)
source_group("Header Files\\Internal Headers\\Format Support\\AIFF" FILES ${INTERNAL_HEADER_FORMATSUPPORT_AIFF})

file (GLOB INTERNAL_HEADER_FORMATSUPPORT_IFF ${SOURCE_ROOT}/FormatSupport/IFF/*.h)
source_group("Header Files\\Internal Headers\\Format Support\\IFF" FILES ${INTERNAL_HEADER_FORMATSUPPORT_IFF})

file (GLOB INTERNAL_HEADER_FORMATSUPPORT_WAVE ${SOURCE_ROOT}/FormatSupport/WAVE/*.h)
source_group("Header Files\\Internal Headers\\Format Support\\WAVE" FILES ${INTERNAL_HEADER_FORMATSUPPORT_WAVE})

file (GLOB INTERNAL_HEADER_FORMATSUPPORT ${SOURCE_ROOT}/FormatSupport/*.hpp)
list (REMOVE_ITEM INTERNAL_HEADER_FORMATSUPPORT
	${SOURCE_ROOT}/FormatSupport/GIF_Support.hpp
	)
list (REMOVE_ITEM INTERNAL_HEADER_FORMATSUPPORT
#	${SOURCE_ROOT}/source/MD5.cpp
	${XMPROOT_DIR}/source/UnicodeConversions.cpp
	)
source_group("Header Files\\Internal Headers\\Format Support" FILES ${INTERNAL_HEADER_FORMATSUPPORT})

file (GLOB INTERNAL_HEADER_PLUGINHANDLER ${SOURCE_ROOT}/PluginHandler/*.h)
source_group("Header Files\\Internal Headers\\PluginHandler" FILES ${INTERNAL_HEADER_PLUGINHANDLER})

list (APPEND PUBLIC_HEADER_CLIENTGLUE
	${XMPROOT_DIR}/public/include/client-glue/TXMPFiles.incl_cpp
	${XMPROOT_DIR}/public/include/client-glue/WXMP_Common.hpp
	${XMPROOT_DIR}/public/include/client-glue/WXMPFiles.hpp
	)
source_group("Header Files\\Public Headers\\Client Glue" FILES ${PUBLIC_HEADER_CLIENTGLUE})
list (APPEND PUBLIC_HEADER
	${XMPROOT_DIR}/public/include/TXMPFiles.hpp
	${XMPROOT_DIR}/public/include/TXMPIterator.hpp
	${XMPROOT_DIR}/public/include/TXMPMeta.hpp
	${XMPROOT_DIR}/public/include/TXMPUtils.hpp
	${XMPROOT_DIR}/public/include/XMP.hpp
	${XMPROOT_DIR}/public/include/XMP.incl_cpp
	${XMPROOT_DIR}/public/include/XMP_Const.h
	${XMPROOT_DIR}/public/include/XMP_Environment.h
	${XMPROOT_DIR}/public/include/XMP_IO.hpp
	${XMPROOT_DIR}/public/include/XMP_Version.h
	)
source_group("Header Files\\Public Headers" FILES ${PUBLIC_HEADER})

file (GLOB HEADERFILES_THIRDPARTY_ZLIB ${XMPROOT_DIR}/third-party/zlib/*.h)
list (REMOVE_ITEM HEADERFILES_THIRDPARTY_ZLIB
	${CMAKE_CURRENT_SOURCE_DIR}/${XMPROOT_DIR}/third-party/zlib/gzguts.h
	)
source_group("Header Files\\ThirdParty\\zlib" FILES ${HEADERFILES_THIRDPARTY_ZLIB})

list (APPEND HEADERFILES
	${XMPROOT_DIR}/source/Host_IO.hpp
	${XMPROOT_DIR}/source/XIO.hpp
	${XMPROOT_DIR}/source/IOUtils.hpp
	${XMPROOT_DIR}/source/XMPFiles_IO.hpp
	)
source_group("Header Files" FILES ${HEADERFILES})

#resource files
file (GLOB RESOURCE_FILES ${RESOURCE_ROOT}/${XMP_PLATFORM_SHORT}/${TARGET_NAME}.*)
if(WIN32 AND ${XMP_BUILD_STATIC})
	list (REMOVE_ITEM RESOURCE_FILES ${RESOURCE_ROOT}/${XMP_PLATFORM_SHORT}/${TARGET_NAME}.rc)
endif()
source_group("Resource Files" FILES ${RESOURCE_FILES})

#source files
file (GLOB SOURCEFILES_COMMONCODE_NMDS ${SOURCE_ROOT}/NativeMetadataSupport/*.cpp)
source_group("Source Files\\Common Code\\NativeMetadataSupport" FILES ${SOURCEFILES_COMMONCODE_NMDS})

list (APPEND SOURCEFILES_COMMONCODE
	${XMPROOT_DIR}/third-party/zuid/interfaces/MD5.cpp
	${XMPROOT_DIR}/source/UnicodeConversions.cpp
	${SOURCE_ROOT}/HandlerRegistry.cpp
	${XMPROOT_DIR}/source/PerfUtils.cpp
	${SOURCE_ROOT}/WXMPFiles.cpp
	${XMPROOT_DIR}/source/XIO.cpp
	${XMPROOT_DIR}/source/IOUtils.cpp
	${XMPROOT_DIR}/source/XML_Node.cpp
	${XMPROOT_DIR}/source/XMP_LibUtils.cpp
	${XMPROOT_DIR}/source/XMP_ProgressTracker.cpp
	${SOURCE_ROOT}/XMPFiles.cpp
	${SOURCE_ROOT}/XMPFiles_Impl.cpp
	${XMPROOT_DIR}/source/XMPFiles_IO.cpp
	)
if(UNIX)
	list(APPEND SOURCEFILES_COMMONCODE ${XMPROOT_DIR}/source/Host_IO-POSIX.cpp)
else()
	list(APPEND SOURCEFILES_COMMONCODE ${XMPROOT_DIR}/source/Host_IO-Win.cpp)
endif()
source_group("Source Files\\Common Code" FILES ${SOURCEFILES_COMMONCODE})

file (GLOB SOURCEFILES_FILEHANDLERS ${SOURCE_ROOT}/FileHandlers/*.cpp)
list (APPEND SOURCEFILES_FILEHANDLERS
	${SOURCE_ROOT}/FileHandlers/AIFF_Handler.hpp
	)
list (REMOVE_ITEM SOURCEFILES_FILEHANDLERS
	${SOURCE_ROOT}/FileHandlers/GIF_Handler.cpp
	)
source_group("Source Files\\File Handlers" FILES ${SOURCEFILES_FILEHANDLERS})

file (GLOB SOURCEFILES_FORMATSUPPORT_AIFF ${SOURCE_ROOT}/FormatSupport/AIFF/*.cpp)
source_group("Source Files\\Format Support\\AIFF" FILES ${SOURCEFILES_FORMATSUPPORT_AIFF})

file (GLOB SOURCEFILES_FORMATSUPPORT_IFF ${SOURCE_ROOT}/FormatSupport/IFF/*.cpp)
source_group("Source Files\\Format Support\\IFF" FILES ${SOURCEFILES_FORMATSUPPORT_IFF})

file (GLOB SOURCEFILES_FORMATSUPPORT_WAVE ${SOURCE_ROOT}/FormatSupport/WAVE/*.cpp)
source_group("Source Files\\Format Support\\WAVE" FILES ${SOURCEFILES_FORMATSUPPORT_WAVE})

file (GLOB SOURCEFILES_FORMATSUPPORT ${SOURCE_ROOT}/FormatSupport/*.cpp)
list (REMOVE_ITEM SOURCEFILES_FORMATSUPPORT
	${SOURCE_ROOT}/FormatSupport/GIF_Support.cpp
	)
source_group("Source Files\\Format Support" FILES ${SOURCEFILES_FORMATSUPPORT})

file (GLOB SOURCEFILES_PLUGINHANDLER ${SOURCE_ROOT}/PluginHandler/*.cpp)
list (REMOVE_ITEM SOURCEFILES_PLUGINHANDLER
	${SOURCE_ROOT}/PluginHandler/OS_Utils_Linux.cpp
	${SOURCE_ROOT}/PluginHandler/OS_Utils_WIN.cpp
	${SOURCE_ROOT}/PluginHandler/OS_Utils_Mac.cpp
	)
if (UNIX)
	if (APPLE)
		list (APPEND SOURCEFILES_PLUGINHANDLER ${SOURCE_ROOT}/PluginHandler/OS_Utils_Mac.cpp)
	else()
		list (APPEND SOURCEFILES_PLUGINHANDLER ${SOURCE_ROOT}/PluginHandler/OS_Utils_Linux.cpp)
	endif()
else()
		list (APPEND SOURCEFILES_PLUGINHANDLER ${SOURCE_ROOT}/PluginHandler/OS_Utils_WIN.cpp)
endif()

source_group("Source Files\\PluginHandler" FILES ${SOURCEFILES_PLUGINHANDLER})

list (APPEND HEADERFILES_THIRDPARTY_ZLIB
	${XMPROOT_DIR}/third-party/zlib/adler32.c
	${XMPROOT_DIR}/third-party/zlib/compress.c
	${XMPROOT_DIR}/third-party/zlib/crc32.c
	${XMPROOT_DIR}/third-party/zlib/deflate.c
	${XMPROOT_DIR}/third-party/zlib/infback.c
	${XMPROOT_DIR}/third-party/zlib/inffast.c
	${XMPROOT_DIR}/third-party/zlib/inflate.c
	${XMPROOT_DIR}/third-party/zlib/inftrees.c
	${XMPROOT_DIR}/third-party/zlib/trees.c
	${XMPROOT_DIR}/third-party/zlib/uncompr.c
	${XMPROOT_DIR}/third-party/zlib/zutil.c
	)
source_group("Source Files\\ThirdParty\\zlib" FILES ${HEADERFILES_THIRDPARTY_ZLIB})

list(APPEND SOURCE_FILES
	${INTERNAL_HEADER_COMMONCODE_NMDS}
	${INTERNAL_HEADER_COMMONCODE}
	${INTERNAL_HEADER_FILEHANDLERS}
	${INTERNAL_HEADER_FORMATSUPPORT_AIFF}
	${INTERNAL_HEADER_FORMATSUPPORT_IFF}
	${INTERNAL_HEADER_FORMATSUPPORT_WAVE}
	${INTERNAL_HEADER_FORMATSUPPORT}
	${INTERNAL_HEADER_PLUGINHANDLER}
	${PUBLIC_HEADER_CLIENTGLUE}
	${PUBLIC_HEADER}
	${HEADERFILES_THIRDPARTY_ZLIB}
	${HEADERFILES}
	${RESOURCE_FILES}
	${SOURCEFILES_COMMONCODE_NMDS}
	${SOURCEFILES_COMMONCODE}
	${SOURCEFILES_FILEHANDLERS}
	${SOURCEFILES_FORMATSUPPORT_AIFF}
	${SOURCEFILES_FORMATSUPPORT_IFF}
	${SOURCEFILES_FORMATSUPPORT_WAVE}
	${SOURCEFILES_FORMATSUPPORT}
	${SOURCEFILES_PLUGINHANDLER}
	${HEADERFILES_THIRDPARTY_ZLIB}
	)

# include directories
include_directories(${XMPROOT_DIR})
include_directories(${XMPROOT_DIR}/public/include)
include_directories(${XMPROOT_DIR}/third-party/expat/zlib)
include_directories(${XMPROOT_DIR}/XMPFilesPlugins/api/source)
include_directories(${RESOURCE_ROOT}/${XMP_PLATFORM_SHORT})

#additional link directory
set(LIB_ADOBEXMP XMPCore)
link_directories(${OUTPUT_DIR})

# ==============================================================================
# Define what to do, lib, exe, etc
if (UNIX AND APPLE AND NOT ${XMP_BUILD_STATIC})
	# preprocess Info.plist
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
	set(DEPENDENCY_LIST "ALL:${TARGET_NAME}InfoPlist" "DLL:XMPCore")
else ()
	set(DEPENDENCY_LIST "DLL:XMPCore")
endif()

AddLibraryAndDependencies(${TARGET_NAME} ${XMP_BUILD_STATIC} YES "SHARED" SOURCE_FILES DEPENDENCY_LIST)

# ==============================================================================
# Link dependencies

if(WIN32)
	target_link_libraries(
		${TARGET_NAME}
		${LIB_ADOBEXMP}
		${XMP_PLATFORM_LINK}
	)
else(WIN32)
	if(UNIX AND NOT APPLE)
		target_link_libraries(
			${TARGET_NAME}
			${LIB_ADOBEXMP}
		)
	endif()
endif()

set(FRAMEWORK_LIST "Mac:CoreFoundation" "Mac:CoreServices" "Mac:${LIB_ADOBEXMP}" "Mac:${XMP_PLATFORM_LINK}")
AddMacFramework(${TARGET_NAME} FRAMEWORK_LIST)

if(UNIX)
	if (NOT APPLE)
		SetWinLinkFlags(${TARGET_NAME} "-Xlinker --version-script -Xlinker \"${RESOURCE_ROOT}/${XMP_PLATFORM_SHORT}/${TARGET_NAME}.exp\"" "")
	else()
		set_target_properties(${TARGET_NAME} PROPERTIES BUILD_WITH_INSTALL_RPATH ON INSTALL_NAME_DIR "@executable_path/../Frameworks")
		SetWinLinkFlags(${TARGET_NAME} "-exported_symbols_list \"${RESOURCE_ROOT}/${XMP_PLATFORM_SHORT}/${TARGET_NAME}.exp\"" "${XMPFILES_LIB}")
	endif()
else()
	SetWinLinkFlags(${TARGET_NAME} "" "${XMPFILES_LIB}")
endif()

set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME  ${XMPFILES_LIB})

# ==============================================================================
# Define output for this project
SetOutputPath(${OUTPUT_DIR} 0)

# ==============================================================================
# Post build
# ==============================================================================

if(UNIX)
	#hack for unix to rename the output static library. cmake add lib and extenstion as .a, rename it
	if (NOT APPLE)
		if (${XMP_BUILD_STATIC})
			add_custom_command (TARGET ${TARGET_NAME}
				POST_BUILD
				COMMAND  mv ${OUTPUT_DIR}/lib${XMPFILES_LIB}.a  ${OUTPUT_DIR}/${XMPFILES_LIB}.ar
			)
		else()
			if((${CMAKE_BUILD_TYPE} MATCHES "Debug") OR (${CMAKE_BUILD_TYPE} MATCHES "debug") )
				add_custom_command (TARGET ${TARGET_NAME}
					POST_BUILD
					COMMAND ls -l ${OUTPUT_DIR}/lib${XMPFILES_LIB}.so
				)
			else()
				add_custom_command (TARGET ${TARGET_NAME}
					POST_BUILD
					COMMAND strip ${OUTPUT_DIR}/lib${XMPFILES_LIB}.so
					COMMAND ls -l ${OUTPUT_DIR}/lib${XMPFILES_LIB}.so
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
