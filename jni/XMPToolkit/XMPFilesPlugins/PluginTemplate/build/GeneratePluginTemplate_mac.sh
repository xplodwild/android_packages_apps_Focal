#!/bin/bash 
# =================================================================================================
# ADOBE SYSTEMS INCORPORATED
# Copyright 2013 Adobe Systems Incorporated
# All Rights Reserved
#
# NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
# of the Adobe license agreement accompanying it.
# =================================================================================================
clear
# Get the absolute path of the script
abspath=$(cd ${0%/*} && echo $PWD/${0##*/})
scriptFolder=$(dirname "$abspath" | tr -s "\n" "/")
cmake_buildbitdepth='On'
clean()
{
echo "Cleaning..."
if [ -e xcode ] 
then
rm -rf xcode
fi	

echo "Done"
exit 0;
}
Generate()
{
cd  "$scriptFolder" >/dev/null
if [ ! -e "$cmakedir" ]; then
mkdir -p "$cmakedir"
fi 
cd "$cmakedir"
CMAKE="$scriptFolder/../../../tools/cmake/bin/CMake.app/Contents/bin/cmake"
echo "$CMAKE  ../../ -G Xcode -DCMAKE_CL_64=$cmake_buildbitdepth -DXMP_CMAKEFOLDER_NAME=$cmakedir -DXMP_BUILD_STATIC=Off -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN "
"$CMAKE"  ../../ -G"Xcode" -DCMAKE_CL_64="$cmake_buildbitdepth" -DXMP_CMAKEFOLDER_NAME="$cmakedir" -DXMP_BUILD_STATIC="Off" -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" 
if [  $? -ne 0 ]
then
echo "ERROR: CMAKE tool failed"
exit 1
else
echo "Xcode project created successfully"
fi
}

PLuginTemplate32()
{
#create 32bit Xcode Project
cmake_buildbitdepth='Off'
cmakedir="xcode/intel"
BITS="32"
TOOLCHAIN="$scriptFolder/../../../build/shared/ToolchainLLVM.cmake"
Generate
}
PLuginTemplate64()
{
#create 64bit Xcode Project
cmake_buildbitdepth='On'
cmakedir="xcode/intel_64"
BITS="64"
TOOLCHAIN="$scriptFolder/../../../build/shared/ToolchainLLVM.cmake"
Generate
}

echo "Enter your choice:"
echo "1. Clean All"
echo "2. Generate PLuginTemplate 32"
echo "3. Generate PluginTemplate 64"
echo "4. Generate All"


read choice

case $choice in
1) clean; break;;
2) PLuginTemplate32;;
3) PLuginTemplate64;;
4) PLuginTemplate32; PLuginTemplate64;;
*) echo "ERROR: Invalid Choice, Exiting"; exit 1;;
esac

exit 0
