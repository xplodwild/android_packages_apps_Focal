// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2013 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.

#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "XMPFiles/source/FormatSupport/PackageFormat_Support.hpp"
#include <algorithm>

// =================================================================================================
/// \file PackageFormat_Support.cpp
///
// =================================================================================================


// =================================================================================================
// PackageFormat_Support::GetPostfixRange
// ================================

#if 0
XMP_Bool PackageFormat_Support::GetPostfixRange ( XMP_FileFormat format , XMP_StringPtr extension, XMP_Uns32 * range )
{
	switch ( format ) {
		case kXMP_XDCAM_EXFile:
			range[0] = 1; range[1] = 99;
			break;
		case kXMP_CanonXFFile:
			range[0] = 1; range[1] = 99;
			break;
		case kXMP_XDCAM_SAMFile:
		case kXMP_XDCAM_FAMFile:
			range[0] = 1; range[1] = 99;
			break;
		case kXMP_P2File:
			range[0] = 0; range[1] = 99;// for voice memo files
			if(strcmp(extension, ".MXF") == 0)
				range[1] = 15;// for audio essence files
			break;
		default:
			return false;
	}
	return true;
}	// PackageFormat_Support::GetPostfixRange
#endif

// =================================================================================================
// PackageFormat_Support::AddResourceIfExists
// ================================

bool PackageFormat_Support::AddResourceIfExists ( XMP_StringVector * resourceList, const XMP_VarString & file )
{
	if ( Host_IO::Exists ( file.c_str() ) ) {
		resourceList->push_back ( file );
		return true;
	}
	return false;
}	// PackageFormat_Support::AddResourceIfExists

#if 0
// =================================================================================================
// PackageFormat_Support::AddResourceIfExists
// ================================

bool PackageFormat_Support::AddResourceIfExists ( XMP_StringVector * resourceList, const XMP_VarString & noExtPath,
	XMP_StringPtr extension, XMP_FileFormat format )
{
	XMP_Uns32 range[2];
	XMP_Bool atLeastOneFileAdded = false, fileAdded = false;
	XMP_VarString iStr, filePath;
	if ( GetPostfixRange ( format, extension, range ) ) {
		for( XMP_Uns32 index = range[0]; index <= range[1] ; ++index )
		{
			SXMPUtils::ConvertFromInt ( index, NULL, &iStr ) ;
			if ( index < LEAST_TWO_DIGIT_INT )
				iStr = '0' + iStr;
			filePath = noExtPath + iStr + extension;
			fileAdded = AddResourceIfExists ( resourceList, filePath );
			atLeastOneFileAdded  |= fileAdded ;
		}
	}
	return atLeastOneFileAdded;
}	// PackageFormat_Support::AddResourceIfExists

#endif
// =================================================================================================
// PackageFormat_Support::AddResourceIfExists
// ================================
bool PackageFormat_Support::AddResourceIfExists ( XMP_StringVector * resourceList, const XMP_VarString & folderPath,
	XMP_StringPtr prefix, XMP_StringPtr postfix )
{
	Host_IO::FolderRef folderHandle = Host_IO::OpenFolder ( folderPath.c_str() );
	if ( folderHandle == Host_IO::noFolderRef || !prefix || !postfix )
		return false;// can't open folder.
	XMP_VarString fileName, filePath;
	size_t fileNameLength;
	size_t prefixLength = strlen ( prefix );
	size_t postfixLength = strlen ( postfix );
	bool atleastOneFileAdded = false;
	while ( Host_IO::GetNextChild ( folderHandle, &fileName ) )
	{
		fileNameLength = fileName.length();
		// Check if the file name starts with prefix and ends with postfix
		if ( fileNameLength >= ( prefixLength + postfixLength ) && 
			fileName.compare ( fileNameLength-postfixLength, postfixLength, postfix ) == 0 && 
			fileName.compare ( 0, prefixLength, prefix ) == 0)
		{
			filePath = folderPath + kDirChar + fileName;
			PackageFormat_Support::AddResourceIfExists ( resourceList, filePath );
			atleastOneFileAdded = true;
		}
	}
	// close folder
	Host_IO::CloseFolder ( folderHandle );
	return atleastOneFileAdded;
}	// PackageFormat_Support::AddResourceIfExists


// =================================================================================================
