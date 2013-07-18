#!/bin/bash
# =================================================================================================
# ADOBE SYSTEMS INCORPORATED
# Copyright 2013 Adobe Systems Incorporated
# All Rights Reserved
#
# NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
# of the Adobe license agreement accompanying it.
# =================================================================================================
# Get the absolut path of the script
abspath=$(cd ${0%/*} && echo $PWD/${0##*/})

# to get the path only - not the script name
path_only=$(dirname "$abspath")

#Set XMPROOT for further reference
XMPROOT=$path_only/..

# defines some handy function used over in several scripts
source "$XMPROOT/build/shared/CMakeUtils.sh"

# generate projects in above file defined function
GenerateBuildProjects "$@"
if [ $? -ne 0 ]; then
    echo "cmake.command failed";
    exit 1;
else
    echo "cmake.command Success";
    exit 0;
fi