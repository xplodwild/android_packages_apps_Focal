# =================================================================================================
# ADOBE SYSTEMS INCORPORATED
# Copyright 2013 Adobe Systems Incorporated
# All Rights Reserved
#
# NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
# of the Adobe license agreement accompanying it.
# =================================================================================================

# Toolchain 
include(CMakeForceCompiler)

# this one is important
set(CMAKE_SYSTEM_NAME Linux)

#
if(CMAKE_CL_64)
	# where is the target environment 
	set(CMAKE_FIND_ROOT_PATH  		"/usr/")
	set(CMAKE_SYSTEM_LIBRARY_PATH  	"${CMAKE_FIND_ROOT_PATH}/")
	set(CMAKE_SYSTEM_PROCESSOR 		"x86_64")

else()
	# where is the target environment 
	set(CMAKE_FIND_ROOT_PATH  		"/usr/")
	set(CMAKE_SYSTEM_LIBRARY_PATH  	"${CMAKE_FIND_ROOT_PATH}/")
	set(CMAKE_SYSTEM_PROCESSOR 		"i386")
endif()

# specify the cross compiler
CMAKE_FORCE_C_COMPILER("${CMAKE_SYSTEM_LIBRARY_PATH}/bin/gcc" GNU)
CMAKE_FORCE_CXX_COMPILER("${CMAKE_SYSTEM_LIBRARY_PATH}/bin/g++" GNU)

set(CMAKE_MAKE_PROGRAMM "${CMAKE_SYSTEM_LIBRARY_PATH}/bin/make")

# specify to use secure settings 
set(XMP_ENABLE_SECURE_SETTINGS "ON")

# search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# extra preprocessor defines
# make Ubuntu cruise happy (read http://old.nabble.com/g%2B%2B-cross-distro-compilation-problem-td30705910.html)
#set(CIT_TOOLCHAIN_COMPILE_FLAGS "-D__USE_XOPEN2K8")

#
message("+++ Toolchain setup:")
message("+++ ${CMAKE_SYSTEM_LIBRARY_PATH}")
message("+++ ${CMAKE_ASM_COMPILER}")
message("+++ ${CMAKE_AR}")
message("+++ ${CMAKE_LINKER}")
message("+++ ${CMAKE_BUILD_TOOL}")

