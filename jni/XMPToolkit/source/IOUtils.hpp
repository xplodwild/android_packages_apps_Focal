#ifndef __IOUtils_hpp__
#define __IOUtils_hpp__	1

// =================================================================================================
// Copyright 2013 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================


#include "public/include/XMP_Environment.h"	
#include "public/include/XMP_Const.h"

#include "source/XMP_LibUtils.hpp"

//Helper class for common IO function
class IOUtils
{
public:
	// Returns the list of folders or files matching particular string format in the given Directory
	static void GetMatchingChildren ( XMP_StringVector & matchingChildList, const XMP_VarString & rootPath,
		const XMP_StringVector & regExStringVec, XMP_Bool includeFolders, XMP_Bool includeFiles, XMP_Bool prefixRootPath );

	// Returns the list of folders or files matching particular string format in the given Directory
	static void GetMatchingChildren ( XMP_StringVector & matchingChildList, const XMP_VarString & rootPath,
		const XMP_VarString & regExpStr, XMP_Bool includeFolders, XMP_Bool includeFiles, XMP_Bool prefixRootPath );
};

#endif	// __IOUtils_hpp__
