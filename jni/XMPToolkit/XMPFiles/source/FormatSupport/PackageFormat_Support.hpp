#ifndef __PackageFormat_Support_hpp__
#define __PackageFormat_Support_hpp__	1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2013 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! This must be the first include.

#include "source/XMP_LibUtils.hpp"

// =================================================================================================
/// \file PackageFormat_Support.hpp
/// \brief XMPFiles support for folder based formats.
///
// =================================================================================================

namespace PackageFormat_Support 
{

	// Checks if the file at path "file" exists.
	// If it exists then it adds to "resourceList" and returns true.
	bool AddResourceIfExists ( XMP_StringVector * resourceList, const XMP_VarString & file );

	// This function adds all the existing files in the specified folder whose name starts with prefix and ends with postfix.
	bool AddResourceIfExists ( XMP_StringVector * resourceList, const XMP_VarString & folderPath,
		XMP_StringPtr prefix, XMP_StringPtr postfix);


} // namespace PackageFormat_Support

// =================================================================================================

#endif	// __PackageFormat_Support_hpp__
