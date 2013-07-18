# =================================================================================================
# ADOBE SYSTEMS INCORPORATED
# Copyright 2013 Adobe Systems Incorporated
# All Rights Reserved
#
# NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
# of the Adobe license agreement accompanying it.
# =================================================================================================

# Toolchain 

# Force XCode to use specific compiler
# options are "4.0", "4.2", "com.apple.compilers.llvmgcc42", "com.intel.compilers.icc.12_1_0", "com.apple.compilers.llvm.clang.1_0"
set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvm.clang.1_0")

# SDK and deployment 
set(XMP_OSX_SDK		 	10.7)
set(XMP_OSX_TARGET	 	10.6)
