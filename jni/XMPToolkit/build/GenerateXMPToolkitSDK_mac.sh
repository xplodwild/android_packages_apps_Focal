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
# Get the absolut path of the script
abspath=$(cd ${0%/*} && echo $PWD/${0##*/})
clean()
{
	echo "Cleaning..."
	if [ -e xcode ] 
	then
	rm -rf xcode
	fi	
	if [ -e ../XMPCore/build/xcode ]
	then
	rm -rf ../XMPCore/build/xcode
	fi
	if [ -e ../XMPFiles/build/xcode ] 
	then
	rm -rf ../XMPFiles/build/xcode
	fi
	if [ -e ../public/libraries/macintosh ] 
	then
	rm -rf ../public/libraries/macintosh
	fi
	if [ -e ../public/libraries/ios ] 
	then
	rm -rf ../public/libraries/ios
	fi
	
	echo "Done"
	exit 0;
}
Generate()
{
cd "`dirname \"$abspath\"`" >/dev/null
./cmake.command $BITS $BUILD_TYPE WarningAsError $TOOLCHAIN
if [  $? -ne 0 ]
then
echo "ERROR: CMAKE tool failed"
exit 1
else
echo "Xcode project created successfully"
fi
}

SDKDynamic32()
{
#create dynamic 32bit Xcode Project
BUILD_TYPE="Dynamic"
BITS="32"
TOOLCHAIN="ToolchainLLVM.cmake"
Generate
}
SDKStatic32()
{
#create static 32bit Xcode Project
BUILD_TYPE="Static"
BITS="32"
TOOLCHAIN="ToolchainLLVM.cmake"
Generate
}
SDKDynamic64()
{
#create dynamic 64bit Xcode Project
BUILD_TYPE="Dynamic"
BITS="64"
TOOLCHAIN="ToolchainLLVM.cmake"
Generate
}
SDKStatic64()
{
#create static 64bit Xcode Project
BUILD_TYPE="Static"
BITS="64"
TOOLCHAIN="ToolchainLLVM.cmake"
Generate
}
SDKStaticIos()
{
#create static ios Xcode Project
BUILD_TYPE="Static"
BITS="32"
TOOLCHAIN="Toolchain_ios.cmake"
Generate
}

echo "1. Clean All"
echo "2. Generate XMPToolkitSDK Dynamic 32"
echo "3. Generate XMPToolkitSDK Static  32"
echo "4. Generate XMPToolkitSDK Dynamic 64"
echo "5. Generate XMPToolkitSDK Static  64"
echo "6. Generate XMPToolkitSDK Static iOS"
echo "7. Generate All"

read -p "Enter your choice: " choice
case $choice in
  1) clean;;
  2) SDKDynamic32;;
  3) SDKStatic32;;
  4) SDKDynamic64;;
  5) SDKStatic64;;
  6) SDKStaticIos;;
  7) SDKDynamic32; SDKStatic32; SDKDynamic64; SDKStatic64; SDKStaticIos;;
  *) echo "ERROR: Invalid Choice, Exiting"; exit 1;;
esac

exit 0


