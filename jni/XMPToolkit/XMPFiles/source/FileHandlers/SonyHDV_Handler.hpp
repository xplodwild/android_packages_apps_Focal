#ifndef __SonyHDV_Handler_hpp__
#define __SonyHDV_Handler_hpp__	1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! This must be the first include.

#include "XMPFiles/source/XMPFiles_Impl.hpp"

#include "source/ExpatAdapter.hpp"

// =================================================================================================
/// \file SonyHDV_Handler.hpp
/// \brief Folder format handler for SonyHDV.
///
/// This header ...
///
// =================================================================================================

extern XMPFileHandler * SonyHDV_MetaHandlerCTor ( XMPFiles * parent );

extern bool SonyHDV_CheckFormat ( XMP_FileFormat format,
								  const std::string & rootPath,
								  const std::string & gpName,
								  const std::string & parentName,
								  const std::string & leafName,
								  XMPFiles * parent );

static const XMP_OptionBits kSonyHDV_HandlerFlags = (kXMPFiles_CanInjectXMP |
													 kXMPFiles_CanExpand |
													 kXMPFiles_CanRewrite |
													 kXMPFiles_PrefersInPlace |
													 kXMPFiles_CanReconcile |
													 kXMPFiles_AllowsOnlyXMP |
													 kXMPFiles_ReturnsRawPacket |
													 kXMPFiles_HandlerOwnsFile |
													 kXMPFiles_AllowsSafeUpdate |
													 kXMPFiles_FolderBasedFormat);

class SonyHDV_MetaHandler : public XMPFileHandler
{
public:

	bool GetFileModDate ( XMP_DateTime * modDate );
	void FillMetadataFiles(std::vector<std::string>* metadataFiles );
	void FillAssociatedResources ( std::vector<std::string> * resourceList );
	bool IsMetadataWritable ( );

	void CacheFileData();
	void ProcessXMP();

	void UpdateFile ( bool doSafeUpdate );
    void WriteTempFile ( XMP_IO* tempRef );

	XMP_OptionBits GetSerializeOptions()	// *** These should be standard for standalone XMP files.
		{ return (kXMP_UseCompactFormat | kXMP_OmitPacketWrapper); };

	SonyHDV_MetaHandler ( XMPFiles * _parent );
	virtual ~SonyHDV_MetaHandler();

private:

	SonyHDV_MetaHandler() {};	// Hidden on purpose.

	bool MakeClipFilePath ( std::string * path, XMP_StringPtr suffix, bool checkFile = false );
	bool MakeIndexFilePath ( std::string& idxPath, const std::string& rootPath, const std::string& leafName );
	void MakeLegacyDigest ( std::string * digestStr );

	std::string rootPath, clipName;

};	// SonyHDV_MetaHandler

// =================================================================================================

#endif /* __SonyHDV_Handler_hpp__ */
