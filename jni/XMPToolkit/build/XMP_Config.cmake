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
# Shared config for XMPTOOLKIT and TestRunner
# ==============================================================================
if(NOT DEFINED XMP_ROOT)
	set(XMP_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/${XMP_THIS_PROJECT_RELATIVEPATH})
	message (XMP_ROOT= " ${XMP_ROOT} ")
endif()
if(NOT DEFINED COMMON_BUILD_SHARED_DIR)
	set(COMMON_BUILD_SHARED_DIR ${XMP_ROOT}/build/shared)
	message (COMMON_BUILD_SHARED_DIR= " ${COMMON_BUILD_SHARED_DIR} ")
endif()

#add definition specific to XMP and shared by all projects
add_definitions(-DXML_STATIC=1 -DHAVE_EXPAT_CONFIG_H=1)
if(XMP_BUILD_STATIC)
	add_definitions(-DXMP_StaticBuild=1)
endif()

set (XMPROOT_DIR ${XMP_ROOT})
set (COMPONENT XMP)


# Load project specific MACRO, VARIABLES; set for component and pass to the common file where there are set
if (UNIX)
	if (APPLE)
		if (APPLE_IOS)
			include(${XMP_ROOT}/build/XMP_Ios.cmake)
		else ()
			include(${XMP_ROOT}/build/XMP_Mac.cmake)
		endif ()
	else ()
        execute_process(COMMAND "uname" OUTPUT_VARIABLE OSNAME)
        string(TOUPPER "${OSNAME}" OSNAME)

        if ( ${OSNAME} MATCHES SUNOS)
            execute_process(COMMAND "uname" "-p" OUTPUT_VARIABLE PLATFORM_SUNOS_ARCH)
            string(TOUPPER "${PLATFORM_SUNOS_ARCH}" PLATFORM_SUNOS_ARCH)

            if ( ${PLATFORM_SUNOS_ARCH} MATCHES SPARC)
		        include(${XMP_ROOT}/build/XMP_sunos_sparc.cmake)
            else()
		        include(${XMP_ROOT}/build/XMP_sunos_intel.cmake)
            endif()
        else()
		    include(${XMP_ROOT}/build/XMP_Linux.cmake)
        endif()
	endif ()
else ()
	if (WIN32)
		include(${XMP_ROOT}/build/XMP_Win.cmake)
	endif ()
endif ()



include(${COMMON_BUILD_SHARED_DIR}/SharedConfig.cmake)


# ==============================================================================
# Function: Create output dir and copy static library to it
# ==============================================================================
#
function(CreateStaticLib productname outputDir)
	if(UNIX)
		add_custom_command (TARGET ${productname} POST_BUILD 
								COMMAND mkdir -p ${outputDir}
								COMMAND cp -f -p ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/lib${productname}.a ${outputDir}/lib${productname}.a
								COMMENT "create output directory and copy lib to it")
	endif(UNIX)
endfunction(CreateStaticLib)

# ==============================================================================
# Function: Copy resource(s) to bundle/lib resource folder
# ==============================================================================
#
function(CopyResource appname outputDir copyWhat)
	if(XMP_BUILD_STATIC)
		add_custom_command (TARGET ${productname} COMMAND ${CMAKE_COMMAND} -E remove_directory ${outputDir} COMMENT "Remove resource dir")
		if(UNIX)
			add_custom_command (TARGET ${appname} POST_BUILD 
								COMMAND mkdir -p ${outputDir}
								COMMAND cp -f ${copyWhat} ${outputDir}
								COMMENT "Copy resource")
		else(UNIX)
			file(TO_NATIVE_PATH "${outputDir}" outputDir_NAT)
			file(TO_NATIVE_PATH "${copyWhat}" copyWhat_NAT)
			add_custom_command (TARGET ${appname} POST_BUILD 
								COMMAND mkdir ${outputDir_NAT}
								COMMAND copy /Y ${copyWhat_NAT} ${outputDir_NAT}
								COMMENT "Copy resource")
		endif(UNIX)
	else(XMP_BUILD_STATIC)
		if(UNIX)
			if(APPLE)
				if(APPLE_IOS)
				else(APPLE_IOS)
					add_custom_command (TARGET ${appname} POST_BUILD 
										COMMAND mkdir -p ${outputDir}
										COMMAND cp -f ${copyWhat} ${outputDir}
										COMMENT "Copy resource")
				endif(APPLE_IOS)
			else(APPLE)
				# Linux
				add_custom_command (TARGET ${appname} POST_BUILD 
									COMMAND mkdir -p ${outputDir}
									COMMAND cp -f ${copyWhat} ${outputDir}
									COMMENT "Copy resource")
			endif(APPLE)
		endif(UNIX)
	endif(XMP_BUILD_STATIC)
endfunction(CopyResource)

# ==============================================================================
# Function: Create a plugin shell and copy necessary things into resource folder
# ==============================================================================
# Win: DLL, resources are linked into DLL
# OSX: Make Bundle, resources are copied into Bundle's resource folder
# Linux: so, resources reside in a folder next to the so with so's name and ".resource" extension
#
function(CreatePlugin productname outputDir copyWhat)
	if(UNIX)
		if(APPLE)
			if(APPLE_IOS OR XMP_BUILD_STATIC)
				CreateStaticLib(${productname} ${outputDir})
				CopyResource(${productname} "${outputDir}/lib${productname}.resources" "${copyWhat}")
			else()
				# create a bundle, copy essential executable into bundle, but don't change output path
				#MakeBundle(${productname} "bundle" ${outputDir} "Info.plist" "Info_plist.h")
				MakeBundle(${productname} "xpi" ${outputDir} "" )
				# copy resources
				#CopyResource(${productname} "${outputDir}/${productname}.bundle/Contents/Resources" "${copyWhat}")
			
				# redo on this level: force no extension to make below bundle creation work
				set(CMAKE_SHARED_MODULE_PREFIX "" PARENT_SCOPE)
				set(CMAKE_SHARED_MODULE_SUFFIX "" PARENT_SCOPE)
			endif()
		else(APPLE)
			# Linux
			if(XMP_BUILD_STATIC)
				CreateStaticLib(${productname} ${outputDir})
				CopyResource(${productname} "${outputDir}/lib${productname}.resources" "${copyWhat}")
			else()
				set(XMP_SHARED_MODULE_SUFFIX ".xpi")
				set_target_properties(${productname} PROPERTIES  SUFFIX ${XMP_SHARED_MODULE_SUFFIX})
				set_target_properties(${productname} PROPERTIES  PREFIX "")
				set(LIBRARY_OUTPUT_PATH ${outputDir} PARENT_SCOPE)
				# copy resources
				#CopyResource(${productname} "${outputDir}/${productname}.resources" "${copyWhat}")
				
				# ExpliXMPly set a softlink with the version  e.g plugin.so.2.0.0-> plugin.so for each Plugin
				#if ( NOT DEFINED XMP_VERSION)
					set (XMP_VERSION 1.0.0) # if not set this should only be the case for the SDK templates we set it to 1.0.0
				#endif()
				#set_target_properties(${productname} PROPERTIES SOVERSION ${XMP_VERSION})
			endif()
		endif(APPLE)
	else(UNIX)
		if(WIN32)
			# set pre/suf-fix
			set(CMAKE_SHARED_MODULE_PREFIX "" PARENT_SCOPE)
			set(XMP_SHARED_MODULE_SUFFIX ".xpi")
			set_target_properties(${productname} PROPERTIES  SUFFIX ${XMP_SHARED_MODULE_SUFFIX})
			# we can't use LIBRARY_OUTPUT_PATH, so build in CMake dir and copy, otherwise an unwanted subfolder is created 
			add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} 
								-E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/${XMP_BUILDMODE_DIR} ${outputDir}/${TARGET_NAME}
								COMMENT "Copy to output dir")
			
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPRODUCT_NAME=\"${PRODUCT_NAME}\"" PARENT_SCOPE)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPRODUCT_NAME=\"${PRODUCT_NAME}\"" PARENT_SCOPE)
			
			if(XMP_BUILD_STATIC)
				CopyResource(${productname} "${outputDir}/${productname}.resources" "${copyWhat}")
			endif(XMP_BUILD_STATIC)
		endif(WIN32)
	endif(UNIX)
endfunction(CreatePlugin)

# ==============================================================================
# Function: Find plugins in XMPPlugin folder and return the list of names
# ==============================================================================
#
function(FindEnabledPlugins resultName)
	file (GLOB XMPPLUGIN_DIRS ${XMP_ROOT}/XMPPlugin/*)
	foreach(item ${XMPPLUGIN_DIRS})
		file (GLOB plugin_project FILES ${item}/build/CMakeLists.txt)
		if(plugin_project)
			get_filename_component(pluginProjectName ${item} NAME)
			if (NOT ${pluginProjectName}_EXCLUDE)
				set(names ${names} ${pluginProjectName}) 
			endif()
		endif()
	endforeach()
	set(${resultName} ${names} PARENT_SCOPE)
endfunction(FindEnabledPlugins)

# ==============================================================================
# Function: Append static lib to target
# ==============================================================================
#
function(AppendStaticLib outputDir targetName srcLibDir srcLibName)
	if(UNIX)
		if(APPLE_IOS)
			add_custom_command (TARGET ${targetName} POST_BUILD
								COMMAND mkdir -p ${srcLibDir}/${srcLibName}_TEMP
								COMMAND cd ${srcLibDir}/${srcLibName}_TEMP && ${CMAKE_AR} x ${srcLibDir}/lib${srcLibName}.a
								#COMMAND split lipo ${srcLibDir}/lib${srcLibName}.a -thin armv6 -output ${srcLibDir}/lib${srcLibName}arm6.a
								#COMMAND split lipo ${srcLibDir}/lib${srcLibName}.a -thin armv7 -output ${srcLibDir}/lib${srcLibName}arm7.a
								#COMMAND merge again lipo ${srcLibDir}/lib${srcLibName}arm6.a ${srcLibDir}/lib${srcLibName}arm7.a -create -output ${srcLibDir}/lib${srcLibName}.a
								COMMAND ${CMAKE_AR} rs ${outputDir}/lib${targetName}.a ${srcLibDir}/${srcLibName}_TEMP/*.o
								COMMAND rm -rf ${srcLibDir}/${srcLibName}_TEMP
								COMMENT "extract objects from lib${srcLibName}.a and pack to lib${targetName}.a")
		else()
			add_custom_command (TARGET ${targetName} POST_BUILD
								COMMAND mkdir -p ${srcLibDir}/${srcLibName}_TEMP
								COMMAND cd ${srcLibDir}/${srcLibName}_TEMP
										&& ${CMAKE_AR} x ${srcLibDir}/lib${srcLibName}.a
								COMMAND ${CMAKE_AR} rs ${outputDir}/lib${targetName}.a ${srcLibDir}/${srcLibName}_TEMP/*.o
								COMMAND rm -rf ${srcLibDir}/${srcLibName}_TEMP
								COMMENT "extract objects from lib${srcLibName}.a and pack to lib${targetName}.a")
		endif()
	else(UNIX)
		if(WIN32)
			add_custom_command (TARGET ${targetName} POST_BUILD
								COMMAND cd ${outputDir}
								&& lib.exe /OUT:${targetName}.lib ${outputDir}/${targetName}.lib ${srcLibDir}/${srcLibName}.lib
								COMMENT "extract objects from ${srcLibName}.lib and pack to ${targetName}.lib")
		endif(WIN32)
	endif(UNIX)
endfunction(AppendStaticLib)

# ==============================================================================
# Function: INTERNAL used only!
# Merge static plugin libraries into output library
# ==============================================================================
#
function(MergeStaticLibs_Internal outputDir targetName useSubDir srcLibDir srcLibNames)
	foreach(libName ${${srcLibNames}})
		if(useSubDir)
			set(correctedDir ${srcLibDir}/${libName})
		else()
			set(correctedDir ${srcLibDir})
		endif()
		AppendStaticLib(${outputDir} ${targetName} ${correctedDir} ${libName})
	endforeach()
endfunction(MergeStaticLibs_Internal)

# ==============================================================================
# Function: Merge static plugin libraries into output library
# ==============================================================================
#
function(MergeStaticLibs outputDir targetName useSubDir srcLibDir srcLibNames)
	MergeStaticLibs_Internal(${outputDir} ${targetName} ${useSubDir} ${srcLibDir} ${srcLibNames})
	if(APPLE)
		if(APPLE_IOS)
		else(APPLE_IOS)
			# APPLE need run ranlib to fix broken TOC
			add_custom_command (TARGET ${targetName} POST_BUILD 
								COMMENT "Run ranlib to fix broken TOC of library file lib${targetName}.a"
								COMMAND ${CMAKE_RANLIB} ${outputDir}/lib${targetName}.a
							   )
		endif(APPLE_IOS)
	endif(APPLE)
endfunction(MergeStaticLibs)

# ==============================================================================
# Function: Get XMP Framework's output name
# ==============================================================================
#
function(GetXMPFrameworkOutputName result)
	if(UNIX)
		if(APPLE)
			if(XMP_BUILD_STATIC)
				set(outputName "libXMP.a")
			else()
				set(outputName "XMP.framework")
			endif()
		else(APPLE)
			if(XMP_BUILD_STATIC)
				set(outputName "libXMP.a")
			else()
				set(outputName "libXMP.so")
			endif()
		endif(APPLE)
	else(UNIX)
		if(WIN32)
			if(XMP_BUILD_STATIC)
				set(outputName "XMP.lib")
			else()
				set(outputName "XMP.dll")
			endif()
		endif(WIN32)
	endif(UNIX)
	set(${result} ${outputName} PARENT_SCOPE)
endfunction(GetXMPFrameworkOutputName)

# ==============================================================================
# Macro: Add Plugin API files to project
# 		 For convenience we define the sources as a variable. You can add 
# 		 header files and cpp/c files and CMake will sort them out
# ==============================================================================
#
macro(AddPluginApiFiles relativePathToXMPPluginFolder)
	file (GLOB API_INCLUDE_FILES ${PROJECT_SOURCE_DIR}/${relativePathToXMPPluginFolder}/XMPFilesPlugins/api/source/*.h)
	source_group(api\\include FILES ${API_INCLUDE_FILES})
	
	file (GLOB API_SOURCE_FILES ${PROJECT_SOURCE_DIR}/${relativePathToXMPPluginFolder}/XMPFilesPlugins/api/source/*.cpp )
	source_group(api\\source FILES ${API_SOURCE_FILES})
endmacro(AddPluginApiFiles)

# ==============================================================================
# Macro: Add Resource files to project
# 		 For convenience we define the sources as a variable. You can add 
# 		 header files and cpp/c files and CMake will sort them out
# ==============================================================================
#
macro(AddPluginResourceFiles)
	list(APPEND RESOURCE_FILES ${PROJECT_ROOT}/resource/txt/MODULE_IDENTIFIER.txt)
	if (CMAKE_CL_64)
		list(APPEND RESOURCE_FILES ${PROJECT_ROOT}/resource/txt/XMPPLUGINUIDS-64.txt)
	else()
		list(APPEND RESOURCE_FILES ${PROJECT_ROOT}/resource/txt/XMPPLUGINUIDS-32.txt)
	endif()
	
	# If we build for windows systems, we also include the resource file containing the manifest, icon and other resources
	if(WIN32)
		if (CMAKE_CL_64)
			list (APPEND RESOURCE_FILES ${RESOURCE_ROOT}/${XMP_PLATFORM_SHORT}/${PRODUCT_NAME}-64.rc)
		else(CMAKE_CL_64)
				list (APPEND RESOURCE_FILES ${RESOURCE_ROOT}/${XMP_PLATFORM_SHORT}/${PRODUCT_NAME}-32.rc)
		endif(CMAKE_CL_64)
	endif()
	source_group("Resource Files" FILES ${RESOURCE_FILES})
endmacro(AddPluginResourceFiles)


# ==============================================================================
# Macro: Set Plugin Output folder
# ==============================================================================
#
macro(SetPluginOutputPath)
	if(UNIX)
		if(APPLE)
			set(OUTPUT_DIR ${PROJECT_SOURCE_DIR}/${XMP_THIS_PROJECT_RELATIVEPATH}/toolkit/XMPFilesPlugins/public/${XMP_PLATFORM_FOLDER}/${CMAKE_CFG_INTDIR})
		else()
		endif()
	else()
		set(OUTPUT_DIR ${PROJECT_SOURCE_DIR}/${XMP_THIS_PROJECT_RELATIVEPATH}/toolkit/XMPFilesPlugins/public/${XMP_PLATFORM_FOLDER}/${CMAKE_CFG_INTDIR}/${PRODUCT_NAME})
	endif()
endmacro(SetPluginOutputPath)

# ==============================================================================
# Function: Setup XMP for the application
#	Copy XMP library, create a symbolic link to Plug-ins, copy MediaAccess and fixup
# ==============================================================================
# 
function(SetupXMPForApp appname outputDir)
	# are optional parameter set?
	if(ARGV2 AND NOT APPLE_IOS)
		if(${ARGV2} MATCHES "FFMPEG")
			set(copyFFMpegLibs	1)
		else()
			set(copyFFMpegLibs	0)
		endif()
	else()
		set(copyFFMpegLibs	0)
	endif()
	
	if(NOT copyFFMpegLibs)
		set(ignoreCommand "#")
	else ()
		set(ignoreCommand "COMMAND")
	endif()

	if(APPLE_IOS)
		set(ignoreCommandOnIOS "#")
		set(ignoreCommandOnOSX "COMMAND")
	else ()
		set(ignoreCommandOnIOS "COMMAND")
		set(ignoreCommandOnOSX "#")
	endif()
	
	
	if(UNIX)
		if(APPLE)
			set(BUILT_PRODUCTS_DIR ${outputDir}/${appname}.app/Contents)
			# GetXMPFrameworkOutputName(XMP_FRAMEWORK_NAME)
			add_custom_command (TARGET ${appname} POST_BUILD 
								COMMENT "Copy necessary XMP items and create symlink to Plug-ins dir"
								COMMAND echo clear XMP dir
								${ignoreCommandOnIOS} rm -fR ${BUILT_PRODUCTS_DIR}/Frameworks/XMP
								COMMAND mkdir -p ${BUILT_PRODUCTS_DIR}/Frameworks/XMP
								${ignoreCommandOnIOS} echo Copy XMP
								${ignoreCommandOnIOS} rm -fR ${BUILT_PRODUCTS_DIR}/Frameworks/${XMP_FRAMEWORK_NAME}*
								${ignoreCommandOnIOS} ln -s ${XMPLIBRARIES_DIR}/${XMP_FRAMEWORK_NAME} ${BUILT_PRODUCTS_DIR}/Frameworks/${XMP_FRAMEWORK_NAME}
								${ignoreCommandOnIOS} ln -s ${XMPLIBRARIES_DIR}/${XMP_FRAMEWORK_NAME}.dSYM ${BUILT_PRODUCTS_DIR}/Frameworks/${XMP_FRAMEWORK_NAME}.dSYM
								${ignoreCommand} echo Copy MediaAccess
								${ignoreCommand} mkdir -p ${BUILT_PRODUCTS_DIR}/Frameworks/XMP/MediaAccess
								${ignoreCommand} cp -fpR ${PROJECT_SOURCE_DIR}/${XMP_THIS_PROJECT_RELATIVEPATH}/toolkit/public/SDK/bin/MediaAccess/${XMP_PLATFORM_FOLDER}/${XMP_BUILDMODE_DIR}/ ${BUILT_PRODUCTS_DIR}/Frameworks/XMP/MediaAccess
								${ignoreCommand} cp -fpR ${PROJECT_SOURCE_DIR}/${XMP_THIS_PROJECT_RELATIVEPATH}/toolkit/public/SDK/bin/MediaAccess/ffmpeg/bin/${XMP_BITDEPTH}/lib* ${BUILT_PRODUCTS_DIR}/Frameworks/XMP/MediaAccess
								${ignoreCommand} ${XMPROOT_DIR}/XMPMediaAccess/build/UpdateFFMPEGLibSearchPath.sh "@executable_path/../Frameworks/XMP/MediaAccess" "${BUILT_PRODUCTS_DIR}/Frameworks/XMP/MediaAccess"
								${ignoreCommandOnIOS} echo Copy XMP plugins shared libs
								${ignoreCommandOnIOS} cp -f -p -R ${XMPLIBRARIES_DIR}/Plug-ins/copy_to_app_dir/ ${BUILT_PRODUCTS_DIR}/Frameworks/
								${ignoreCommandOnIOS}  echo create symlinks
								${ignoreCommandOnIOS}  ln -s ${XMPLIBRARIES_DIR}/Plug-ins/ ${BUILT_PRODUCTS_DIR}/Frameworks/XMP/Plug-ins
								${ignoreCommandOnOSX}  echo copy plugin data
								${ignoreCommandOnOSX} mkdir -p ${BUILT_PRODUCTS_DIR}/Resources/XMP/Plug-ins
								${ignoreCommandOnOSX}  cp -fpR ${XMPLIBRARIES_DIR}/Plug-ins/ ${BUILT_PRODUCTS_DIR}/Resources/XMP/Plug-ins
								)
		else(APPLE)
			# Do not copy FFMpeg libs if they don't exist 
			if(NOT EXISTS ${PROJECT_SOURCE_DIR}/${XMP_THIS_PROJECT_RELATIVEPATH}/toolkit/public/SDK/bin/MediaAccess/${XMP_PLATFORM_FOLDER}/${XMP_BUILDMODE_DIR}/FFMpegWrapper.mediaaccess)
				set(ignoreCommand "#")
			else ()
				set(ignoreCommand "COMMAND")
			endif()
			
			set(BUILT_PRODUCTS_DIR ${outputDir})
			# GetXMPFrameworkOutputName(XMP_FRAMEWORK_NAME)
			add_custom_command (TARGET ${appname} POST_BUILD 
								COMMENT "Copy necessary XMP items and create symlink to Plug-ins dir"
								COMMAND echo clear XMP dir
								COMMAND rm -f -R ${BUILT_PRODUCTS_DIR}/XMP
								COMMAND mkdir -p ${BUILT_PRODUCTS_DIR}/XMP
								COMMAND echo Copy XMP
								COMMAND rm -f -R ${BUILT_PRODUCTS_DIR}/libXMP*
								COMMAND ln -s ${XMPLIBRARIES_DIR}/${XMP_FRAMEWORK_NAME} ${BUILT_PRODUCTS_DIR}/${XMP_FRAMEWORK_NAME}
								${ignoreCommand} echo Copy MediaAccess
								${ignoreCommand} mkdir -p ${BUILT_PRODUCTS_DIR}/XMP/MediaAccess
								${ignoreCommand} cp -fpR ${PROJECT_SOURCE_DIR}/${XMP_THIS_PROJECT_RELATIVEPATH}/toolkit/public/SDK/bin/MediaAccess/${XMP_PLATFORM_FOLDER}/${XMP_BUILDMODE_DIR}/*.* ${BUILT_PRODUCTS_DIR}/XMP/MediaAccess
								#COMMAND ${ignoreCommand} cp -fpR ${PROJECT_SOURCE_DIR}/${XMP_THIS_PROJECT_RELATIVEPATH}/toolkit/public/SDK/bin/MediaAccess/ffmpeg/bin/${XMP_BITDEPTH}/*.* ${BUILT_PRODUCTS_DIR}/XMP/MediaAccess
								COMMAND echo Copy XMP plugins shared libs
								#COMMAND cp -f -p -R ${XMPLIBRARIES_DIR}/Plug-ins/copy_to_app_dir/ ${BUILT_PRODUCTS_DIR}/
								COMMAND echo create symlinks
								COMMAND ln -s ${XMPLIBRARIES_DIR}/Plug-ins/ ${BUILT_PRODUCTS_DIR}/XMP/Plug-ins
								)
		endif(APPLE)
	else(UNIX)
		if(WIN32)
			# Windows -------------------------------------------
			set(BUILT_PRODUCTS_DIR ${outputDir})
			# GetXMPFrameworkOutputName(XMP_FRAMEWORK_NAME)
			file(TO_NATIVE_PATH "${BUILT_PRODUCTS_DIR}/XMP/Plug-ins" BUILT_PLUGINS_DIR_NAT)
			file(TO_NATIVE_PATH "${XMPLIBRARIES_DIR}/Plug-ins" PLUGINS_DIR_NAT)
			file(TO_NATIVE_PATH "${BUILT_PRODUCTS_DIR}/${XMP_FRAMEWORK_NAME}" DEST_XMP_NAT)
			file(TO_NATIVE_PATH "${XMPLIBRARIES_DIR}/${XMP_FRAMEWORK_NAME}" SRC_XMP_NAT)
			file(TO_NATIVE_PATH "${BUILT_PRODUCTS_DIR}/XMP.pdb" DEST_XMP_PDB_NAT)
			file(TO_NATIVE_PATH "${XMPLIBRARIES_DIR}/XMP.pdb" SRC_XMP_PDB_NAT)
			
			#For Windows XP we need to use junction.exe instead of mklink which is only available on Windows Vista and above 
			if((${CMAKE_SYSTEM} MATCHES "Windows-5.1") OR (${CMAKE_SYSTEM} MATCHES "Windows-5.2")) 
				message (STATUS "- Use junction.exe to create links instead of mklink we either run on Win XP 32 or 64 bit")
				set(link_command ${PROJECT_SOURCE_DIR}/${XMP_THIS_PROJECT_RELATIVEPATH}/resources/tools/junction/junction.exe -accepteula )
				set(remove_XMP "echo")
				set(copy_link_XMP ${CMAKE_COMMAND} -E copy ${SRC_XMP_NAT} ${BUILT_PRODUCTS_DIR}/)
				set(copy_link_XMP_pdb ${CMAKE_COMMAND} -E copy ${SRC_XMP_PDB_NAT} ${BUILT_PRODUCTS_DIR}/)
			else()
				set(link_command mklink /D )
				set(remove_XMP ${CMAKE_COMMAND} -E remove -f ${DEST_XMP_NAT} ${DEST_XMP_PDB_NAT})
				set(copy_link_XMP mklink ${DEST_XMP_NAT} ${SRC_XMP_NAT})
				set(copy_link_XMP_pdb mklink ${DEST_XMP_PDB_NAT} ${SRC_XMP_PDB_NAT})
			endif()
		
			add_custom_command (TARGET ${appname} POST_BUILD 
								COMMENT "Copy necessary XMP items and create symlink to Plug-ins dir"
								COMMAND echo - clear XMP dir
								COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILT_PRODUCTS_DIR}/XMP
								COMMAND echo - symlink XMP.dll and XMP.pdb
								COMMAND ${remove_XMP}
								COMMAND ${copy_link_XMP}
								COMMAND ${copy_link_XMP_pdb}
								COMMAND echo - copy XMP plugins shared libs (tbb etc)
								COMMAND ${CMAKE_COMMAND} -E copy_directory ${XMPLIBRARIES_DIR}/Plug-ins/copy_to_app_dir/ ${BUILT_PRODUCTS_DIR}
								${ignoreCommand} echo - copy FFMPEG libs
								${ignoreCommand} ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/${XMP_THIS_PROJECT_RELATIVEPATH}/toolkit/public/SDK/bin/MediaAccess/ffmpeg/bin/${XMP_BITDEPTH}/ ${BUILT_PRODUCTS_DIR}/XMP/MediaAccess
								${ignoreCommand} ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/${XMP_THIS_PROJECT_RELATIVEPATH}/toolkit/public/SDK/bin/MediaAccess/${XMP_PLATFORM_FOLDER}/${XMP_BUILDMODE_DIR}/FFMpegWrapper.mediaaccess ${BUILT_PRODUCTS_DIR}/XMP/MediaAccess/FFMpegWrapper.mediaaccess
								${ignoreCommand} ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/${XMP_THIS_PROJECT_RELATIVEPATH}/toolkit/public/SDK/bin/MediaAccess/${XMP_PLATFORM_FOLDER}/${XMP_BUILDMODE_DIR}/ffmpegwrapper.pdb ${BUILT_PRODUCTS_DIR}/XMP/MediaAccess/ffmpegwrapper.pdb
								COMMAND echo - remove symbolic link to plug-ins dir
								COMMAND rmdir ${BUILT_PLUGINS_DIR_NAT}
								COMMAND echo - execute symbolic link command 
								COMMAND ${link_command} ${BUILT_PLUGINS_DIR_NAT} ${PLUGINS_DIR_NAT} 
								)
		endif(WIN32)
	endif(UNIX)
endfunction(SetupXMPForApp)

# ==============================================================================
# Function: GenerateInitFileForStaticBuild
#	Collect registrar initializer functions and create source in temp directory 
#	out of it for static linkage
# ==============================================================================
# 
function(GenerateInitFileForStaticBuild)
	if(XMP_BUILD_STATIC)
		set(targetFile ${XMP_ROOT}/XMPApi/source/PluginStaticInit_TEMP.cpp)
		file(REMOVE ${targetFile})
		
		set(initDecl "#if XMP_BUILD_STATIC\n\n")
		set(initFunc "namespace XMP\n{\n    void InstanciateRegistrarsForAllPlugins()\n    {\n")
		foreach(pluginName ${XMP_ENABLED_PLUGINS})
			set(initDecl "${initDecl}extern void ${pluginName}_InstanciateRegistrars()\;\n")
			set(initFunc "${initFunc}        ::${pluginName}_InstanciateRegistrars()\;\n")
		endforeach()
		set(initFunc "${initFunc}    }\n}\n")
		set(initFunc "${initDecl}\n${initFunc}\n#endif\n")
		
		file(WRITE ${targetFile} ${initFunc})
	endif(XMP_BUILD_STATIC)
endfunction(GenerateInitFileForStaticBuild)
