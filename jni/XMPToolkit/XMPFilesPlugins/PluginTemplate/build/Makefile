# =================================================================================================
# ADOBE SYSTEMS INCORPORATED
# Copyright 2013 Adobe Systems Incorporated
# All Rights Reserved
#
# NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
# of the Adobe license agreement accompanying it.
# =================================================================================================

CURRDIR := $(shell dirname "$(realpath $(lastword $(MAKEFILE_LIST)))")
arch=$(shell getconf LONG_BIT)
CMAKE="$(CURRDIR)/../../../tools/cmake/bin/cmake"
TOOLCHAIN="$(CURRDIR)/../../../build/shared/ToolchainGCC.cmake"

# Default target executed when no arguments are given to make.
default_target: all

# target to generate make files using cmake
Release32: DIR=./gcc/i80386linux/Release/
Release32:
ifeq ($(arch),32)
	mkdir -p $(DIR) || echo "Unable to create Directory $(DIR)"; \
	cd $(DIR) || echo "unable to Change Directory $(DIR)" ; \
	${MAKE} -C "$(CURRDIR)/../../../build/" StaticRelease32 ; \
	$(CMAKE) -D CMAKE_CL_64=0 -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"  -DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN) ../../../.
	${MAKE} -C $(DIR)
endif


Debug32: DIR=./gcc/i80386linux/Debug/
Debug32:
ifeq ($(arch),32)
	mkdir -p $(DIR) || echo "Unable to create Directory $(DIR)"; \
	cd $(DIR) || echo "unable to Change Directory $(DIR)" ; \
	${MAKE} -C "$(CURRDIR)/../../../build/" StaticDebug32 ; \
	$(CMAKE) -D CMAKE_CL_64=0 -DCMAKE_BUILD_TYPE=Debug -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN) ../../../.
	${MAKE} -C $(DIR)
endif

Release64: DIR=./gcc/i80386linux_x64/Release/
Release64:
ifeq ($(arch),64)
	mkdir -p $(DIR) || echo "Unable to create Directory $(DIR)"; \
	cd $(DIR) || echo "unable to Change Directory $(DIR)" ; \
	${MAKE} -C "$(CURRDIR)/../../../build/" StaticRelease64 ; \
	$(CMAKE) -D CMAKE_CL_64=1 -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"  -DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN) ../../../.
	${MAKE} -C $(DIR)
endif

Debug64: DIR=./gcc/i80386linux_x64/Debug/
Debug64:
ifeq ($(arch),64)
	mkdir -p $(DIR) || echo "Unable to create Directory $(DIR)"; \
	cd $(DIR) || echo "unable to Change Directory $(DIR)" ; \
	${MAKE} -C "$(CURRDIR)/../../../build/" StaticDebug64 ; \
	$(CMAKE) -D CMAKE_CL_64=1 -DCMAKE_BUILD_TYPE=Debug -G "Unix Makefiles"  -DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN) ../../../.
	${MAKE} -C $(DIR)
endif

all: Release32 Debug32 Release64 Debug64


clean:
	test -d gcc && rm -rf gcc
	