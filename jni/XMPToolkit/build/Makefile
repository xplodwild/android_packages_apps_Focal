# =================================================================================================
# ADOBE SYSTEMS INCORPORATED
# Copyright 2013 Adobe Systems Incorporated
# All Rights Reserved
#
# NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
# of the Adobe license agreement accompanying it.
# =================================================================================================

arch=$(shell getconf LONG_BIT)
toolchain=ToolchainGCC.cmake

# Default target executed when no arguments are given to make.
default_target: all

#******************************************************************************************************************/
# you can specify which toolchain file is used to genereate the cmake project for the specific branch
#******************************************************************************************************************/

# Export for cmake binaries

# target to generate make files using cmake
DynamicRelease32:
ifeq ($(arch),32)
	./cmake.command 32 Dynamic Release ${toolchain} Clean
	${MAKE} -C ./gcc/dynamic/i80386linux/Release/ 
endif

DynamicDebug32:
ifeq ($(arch),32)
	./cmake.command 32 Dynamic Debug ${toolchain} Clean
	${MAKE} -C ./gcc/dynamic/i80386linux/Debug/ 
endif

StaticRelease32:
ifeq ($(arch),32)
	./cmake.command 32 Static Release ${toolchain} Clean
	${MAKE} -C ./gcc/static/i80386linux/Release/ 
endif

StaticDebug32:
ifeq ($(arch),32)
	./cmake.command 32 Static Debug ${toolchain} Clean
	${MAKE} -C ./gcc/static/i80386linux/Debug/ 
endif

DynamicRelease64:
ifeq ($(arch),64)
	./cmake.command 64 Dynamic Release ${toolchain} Clean
	${MAKE} -C ./gcc/dynamic/i80386linux_64/Release/ 
endif

DynamicDebug64:
ifeq ($(arch),64)
	./cmake.command 64 Dynamic Debug ${toolchain} Clean
	${MAKE} -C ./gcc/dynamic/i80386linux_64/Debug/ 
endif

StaticRelease64:
ifeq ($(arch),64)
	./cmake.command 64 Static Release ${toolchain} Clean
	${MAKE} -C ./gcc/static/i80386linux_64/Release/ 
endif

StaticDebug64:
ifeq ($(arch),64)
	./cmake.command 64 Static Debug ${toolchain} Clean
	${MAKE} -C ./gcc/static/i80386linux_64/Debug/ 
endif

StaticAll: StaticRelease32 StaticDebug32 StaticRelease64 StaticDebug64

DynamicAll: DynamicRelease32 DynamicDebug32 DynamicRelease64 DynamicDebug64

DebugAll: DynamicDebug32 DynamicDebug64 StaticDebug32 StaticDebug64

ReleaseAll: DynamicRelease32 DynamicRelease64 StaticRelease32 StaticRelease64

all: DynamicAll StaticAll

clean:
	test -d gcc && rm -rf gcc; \
	test -d ../XMPCore/build/gcc && rm -rf ../XMPCore/build/gcc; \
	test -d ../XMPFiles/build/gcc && rm -rf ../XMPFiles/build/gcc; \
	test -d ../public/libraries/i80386linux && rm -rf ../public/libraries/i80386linux; \
	test -d ../public/libraries/i80386linux_x64 && rm -rf ../public/libraries/i80386linux_x64; \
	echo "Clean Success"  

rebuild: clean all
