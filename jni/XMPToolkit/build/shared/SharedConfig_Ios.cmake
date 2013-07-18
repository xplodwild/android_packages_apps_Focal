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
# Shared config for iOS
# ==============================================================================



set(CMAKE_C_FLAGS "${${COMPONENT}_SHARED_COMPILE_FLAGS} ${${COMPONENT}_EXTRA_C_COMPILE_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG "${${COMPONENT}_SHARED_COMPILE_DEBUG_FLAGS}")
set(CMAKE_C_FLAGS_RELEASE "${${COMPONENT}_SHARED_COMPILE_RELEASE_FLAGS}")

set(CMAKE_CXX_FLAGS "${${COMPONENT}_SHARED_COMPILE_FLAGS} ${${COMPONENT}_EXTRA_CXX_COMPILE_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${${COMPONENT}_SHARED_COMPILE_DEBUG_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${${COMPONENT}_SHARED_COMPILE_RELEASE_FLAGS}")

if(${COMPONENT}_ENABLE_SECURE_SETTINGS)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector -D_FORTIFY_SOURCE=2")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector")
endif()


# set compiler flags
SetupCompilerFlags()

# set various working and output directories
SetupGeneralDirectories()

# ==============================================================================
# Function: Add iOS framework to list of libraries to link to
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
# Function: Set platform specific link flags to target
# ==============================================================================
#
function(SetPlatformLinkFlags target linkflags)
	# set additional XCode attribute for iOS
	#set_target_properties(${target} PROPERTIES XCODE_TARGETED_DEVICE_FAMILY "1,2 ")
	if (CMAKE_INSTALLED_XCODE_VERSION GREATER 4.4.9)
		set(linkflags "${linkflags} -Xlinker -no_data_in_code_info")
	endif()
	set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS " ${linkflags}")

	set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "YES")
	set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_GCC_ENABLE_PASCAL_STRINGS "NO")
	set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_GCC_NO_COMMON_BLOCKS "YES")
	set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_GCC_FAST_MATH "YES")
	set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_GCC_SHORT_ENUMS "YES")
	set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_GCC_CHAR_IS_UNSIGNED_CHAR "YES")
	set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_WARNING_CFLAGS "")
	set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_INITIALIZER_NOT_FULLY_BRACKETED "YES")
	set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_ABOUT_RETURN_TYPE "YES")
	set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_ABOUT_MISSING_PROTOTYPES "YES")
	set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_ABOUT_MISSING_NEWLINE "YES")
	set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_NON_VIRTUAL_DESTRUCTOR "YES")
	set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_HIDDEN_VIRTUAL_FUNCTIONS "YES")
	set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_SIGN_COMPARE "YES")
	set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_UNKNOWN_PRAGMAS "YES")
	set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_UNUSED_VALUE "NO")
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

	if(${targetType} MATCHES "STATIC")
		set_property(TARGET ${targetName} PROPERTY ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${correctedPath}/$(CONFIGURATION)$(EFFECTIVE_PLATFORM_NAME))
		set_property(TARGET ${targetName} PROPERTY ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${correctedPath}/$(CONFIGURATION)$(EFFECTIVE_PLATFORM_NAME))
		message("SetOutputPath2: Set ARCHIVE_OUTPUT_DIRECTORY to ${correctedPath}/$(CONFIGURATION)$(EFFECTIVE_PLATFORM_NAME)")
	endif()
endfunction(SetOutputPath2)

# ==============================================================================
# Function: Set precompiled headers
# ==============================================================================
#
function( SetPrecompiledHeaders target pchHeader pchSource )
	set_target_properties( ${target} PROPERTIES XCODE_ATTRIBUTE_GCC_PREFIX_HEADER ${pchHeader} )
	set_target_properties( ${target} PROPERTIES XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER "YES" )
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
		set(result "${libpath}/lib${libname_item}.dylib")
		set(resultList ${resultList} ${result})
	endforeach()	
	set(${resultName} ${${resultName}} ${resultList} PARENT_SCOPE)
	message("AddSharedLibraryNameInNativeFormat: ${resultList} ")
endfunction(AddSharedLibraryNameInNativeFormat)

# ==============================================================================
# Function: Add Boost library to variables (resultName and resultNameDebug)
# ==============================================================================
#
function(AddBoostLib libname resultName resultNameDebug)
    # optional parameter set?
    if(ARGV3)
		set(boostDir ${ARGV3})
    else()
        set(boostDir "${XMPROOT_DIR}/../resources/third_party/${XMP_BOOST_VERSIONNAME}/libraries/${XMP_PLATFORM_FOLDER}")
    endif()

	set(libExtension	"a")
	set(libPrefix 		"libboost_")

    # add debug/optimized keyword for proper linking
    set(result "optimized;${boostDir}/release/${libPrefix}${libname}.${libExtension}")
    set(resultDebug "debug;${boostDir}/debug/${libPrefix}${libname}.${libExtension}")
    set(${resultName} ${${resultName}} ${result} PARENT_SCOPE)
    set(${resultNameDebug} ${${resultNameDebug}} ${resultDebug} PARENT_SCOPE)
endfunction(AddBoostLib)

# ==============================================================================
# Function: Add tbb library to variables (resultName and resultNameDebug)
# ==============================================================================
#
function(AddTbbLib resultName resultNameDebug)
    message("NO TBB library available on iOS!")
endfunction(AddTbbLib)

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
		add_custom_command (TARGET ${appname} PRE_BUILD 
								COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
								COMMAND ${GCCTOOL} -E -P -x c ${RESOURCE_ROOT}/${XMP_PLATFORM_SHORT}/${infoPlist}
								-F${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/ -DPRODUCT_NAME=${appname} "$(OTHER_CFLAGS)"
								-include ${RESOURCE_ROOT}/${XMP_PLATFORM_SHORT}/${infoPlistHeader} -o ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/Info.plist
								COMMENT "Preprocessing Info-plist")
	
		# define extra command
		set(extraCmd cp -f ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/Info.plist ${outputDir}/${appname}.${extension}/Contents/Info.plist)
	endif()
	
	# make bundle
	add_custom_command (TARGET ${appname} POST_BUILD 
						COMMAND mkdir -p ${outputDir}/${appname}.${extension}/Contents/MacOS
						COMMAND cp -f ${PROJECT_SOURCE_DIR}/${XMP_THIS_PROJECT_RELATIVEPATH}/toolkit/build/mac/PkgInfo ${outputDir}/${appname}.${extension}/Contents/PkgInfo
						COMMAND ${extraCmd}
						COMMAND cp -f ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${appname} ${outputDir}/${appname}.${extension}/Contents/MacOS/${appname}
						COMMAND cp -rf ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${appname}.dSYM ${outputDir}/${appname}.${extension}.dSYM
						COMMENT "Make bundle")
endfunction(MakeBundle)

