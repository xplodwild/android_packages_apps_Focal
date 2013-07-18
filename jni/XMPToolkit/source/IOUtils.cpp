// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2013 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "source/IOUtils.hpp"
#include "source/Host_IO.hpp"

#include <algorithm>

// =================================================================================================
// ListAllChildren
// =================================================================================================

static void ListAllChildren( XMP_StringPtr folderPath, XMP_StringVector & list, XMP_Bool listFolders, XMP_Bool listFiles, XMP_Bool sortList )
{
	try 
	{
		Host_IO::AutoFolder af;
		af.folder = Host_IO::OpenFolder ( folderPath );
		if ( af.folder != Host_IO::noFolderRef )
		{
			std::string resourceName;

			while ( Host_IO::GetNextChild ( af.folder, &resourceName ) == true )
			{

				XMP_Bool addPath = false;
				if ( listFolders && listFiles ) {
					addPath = true;
				}
				else if ( listFolders )
				{
					if ( Host_IO::GetChildMode ( folderPath, resourceName.c_str() ) == Host_IO::kFMode_IsFolder )
						addPath = true;
				}
				else if ( listFiles )
				{
					if ( Host_IO::GetChildMode ( folderPath, resourceName.c_str() ) == Host_IO::kFMode_IsFile )
						addPath = true;
				}

				if ( addPath )
					list.push_back ( resourceName );
			}
		}
	} catch ( XMP_Error & ) {
		// do nothing
	}
	if ( sortList )
		std::sort ( list.begin(), list.end() );
}


// =================================================================================================
// GetMatchingChildren
// =================================================================================================

void IOUtils::GetMatchingChildren ( XMP_StringVector & matchingChildList, const XMP_VarString & rootPath,
	const XMP_StringVector & regExStringVec, XMP_Bool includeFolders, XMP_Bool includeFiles, XMP_Bool prefixRootPath )
{
	try 
	{
		XMP_StringVector listOfAllResources;
		ListAllChildren (rootPath.c_str(), listOfAllResources, includeFolders, includeFiles, true);

		XMP_Bool matchRequired = !regExStringVec.empty();
		if ( matchRequired )
		{
			size_t childCount = listOfAllResources.size();
			for ( size_t index = 0; index < childCount; index++ )
			{
				XMP_Bool match = false;
				size_t regExpCount = regExStringVec.size();
				for ( size_t index2 = 0; index2 < regExpCount; index2++ )
				{
					XMP_RegExp regexObj ( regExStringVec[index2].c_str() );
					match = regexObj.Match ( listOfAllResources[index].c_str() );

					if ( match )
					{
						if ( prefixRootPath )
						{
							std::string fullPath = rootPath;
							if (fullPath[fullPath.length() - 1] != kDirChar )
								fullPath += kDirChar;
							fullPath += listOfAllResources[index];
							matchingChildList.push_back ( fullPath );
						}
						else
							matchingChildList.push_back ( listOfAllResources[index] );
						break;
					}
				}
			}
		}
	} catch ( XMP_Error & ) {
		// do nothing
	}
} // GetMatchingChildren

// =================================================================================================
// GetMatchingChildren
// =================================================================================================

void IOUtils::GetMatchingChildren ( XMP_StringVector & matchingChildList, const XMP_VarString & rootPath,
	const XMP_VarString & regExpStr, XMP_Bool includeFolders, XMP_Bool includeFiles, XMP_Bool prefixRootPath )
{
	XMP_StringVector regExpStringVec;
	regExpStringVec.push_back ( regExpStr );
	return GetMatchingChildren ( matchingChildList, rootPath, regExpStringVec, includeFolders, includeFiles, prefixRootPath );
} // GetMatchingChildren

// =================================================================================================
