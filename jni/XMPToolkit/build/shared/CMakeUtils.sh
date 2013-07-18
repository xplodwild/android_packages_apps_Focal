#!/bin/bash
# =================================================================================================
# ADOBE SYSTEMS INCORPORATED
# Copyright 2013 Adobe Systems Incorporated
# All Rights Reserved
#
# NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
# of the Adobe license agreement accompanying it.
# =================================================================================================

# define cmake generator depending on the platform we run on
unamestr=`uname`
if [ "$unamestr" == 'Linux' ]; then
	    is_makefile='ON'
	    cmake_generator='Unix Makefiles'
	    compiler='gcc'
	    PATH=$PATH:$XMPROOT/tools/cmake/bin
elif [ "$unamestr" == 'Darwin' ]; then
	    is_makefile='OFF'
	    cmake_generator='Xcode'
	    compiler='xcode'
	    PATH=$PATH:$XMPROOT/tools/cmake/bin/CMake.app/Contents/bin
else
	    is_makefile='ON'
	    cmake_generator='Unix Makefiles'
	    compiler='gcc'
	    PATH=$PATH:$XMPROOT/tools/cmake/bin/
fi



function GenerateBuildProjects () {

#function call 

#echo "Invoke cmake with the following environment settings"
#echo $(env)
# Change to the directory where this script lives.
cd "$(dirname $0)" >/dev/null

#set defaults
cmake_build_warning_as_error="Off"
TOOLCHAIN=""
cmake_buildbitdepth='On'
cmake_modesubdir='_64'
export MACHTYPE=x86_64
cmake_buildmode="Release"
cmake_buildtype="dynamic"
cmake_build_static="Off"
clean_cmakedir="Off"

while [ "$1" != "" ]
do
if [ "$1" == "32" ]; then
	cmake_buildbitdepth='Off'
	cmake_modesubdir=
	export MACHTYPE=i386

elif [ "$1" == "Static" ]; then
	cmake_build_static="On"
    cmake_buildtype="static"
elif [ "$1" == "Debug" ]; then
    cmake_buildmode="Debug"
elif [ "$1" == "WarningAsError" ]; then
        cmake_build_warning_as_error="On"
elif [ "$1" == "Clean" ]; then
    clean_cmakedir="On"
elif [ "$1" == "ToolchainLLVM.cmake" ]; then
	TOOLCHAIN=$1
elif [ "$1" == "ToolchainGCC.cmake" ]; then
	TOOLCHAIN=$1	
elif [ "$1" == "Toolchain_ios.cmake" ]; then
	TOOLCHAIN=$1	
elif [ "$TOOLCHAIN" == "" ]; then
	TOOLCHAIN=$1
fi
shift
done
echo "---------------------------Config----------------------------------------"
echo "cmake_build_warning_as_error=$cmake_build_warning_as_error"
echo "TOOLCHAIN=$TOOLCHAIN"
echo "cmake_buildbitdepth=$cmake_buildbitdepth"
echo "cmake_modesubdir=$cmake_modesubdir"
echo "MACHTYPE=$MACHTYPE"
echo "cmake_buildtype=$cmake_buildtype"
echo "cmake_buildmode=$cmake_buildmode"
echo "cmake_build_static=$cmake_build_static"
echo "clean_cmakedir=$clean_cmakedir"
# make build dir
if [ "$is_makefile" == "ON" ]; then
	cmakedir="$compiler/$cmake_buildtype/i80386linux$cmake_modesubdir/$cmake_buildmode"
	cmakeconfigdir="../../../../."
else
    if [ "$TOOLCHAIN" == "Toolchain_ios.cmake" ]; then
        cmakedir="$compiler/$cmake_buildtype/ios"
    else
	    cmakedir="$compiler/$cmake_buildtype/intel$cmake_modesubdir"
	fi
	cmakeconfigdir="../../../."
fi
if [ "$clean_cmakedir" == "On" ]; then
	rm -r -f "$cmakedir"
fi
if [ ! -e "$cmakedir" ]; then
	mkdir -p "$cmakedir"
fi
cd "$cmakedir"
if [ "$TOOLCHAIN" != "" ]; then
	echo "Using toolchain $TOOLCHAIN"
	# generate projects with toolchain file
	cmake $cmakeconfigdir -G"$cmake_generator" -DCMAKE_CL_64="$cmake_buildbitdepth" -DCMAKE_BUILD_TYPE="$cmake_buildmode" -DXMP_CMAKEFOLDER_NAME="$cmakedir" -DXMP_BUILD_STATIC="$cmake_build_static" -DCMAKE_TOOLCHAIN_FILE="$XMPROOT/build/shared/$TOOLCHAIN" 
else
	# generate projects for  build
	cmake $cmakeconfigdir -G"$cmake_generator" -DCMAKE_CL_64="$cmake_buildbitdepth" -DCMAKE_BUILD_TYPE="$cmake_buildmode" -DXMP_CMAKEFOLDER_NAME="$cmakedir" -DXMP_BUILD_STATIC="$cmake_build_static"
fi

if [ $? -ne 0 ]; then
    echo " CmakeUtils.txt Failed."
    return 1;
else
    echo " CmakeUtils.txt Success. "
    return 0;
fi    
}
