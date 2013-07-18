#ifndef __XDCAMEX_Handler_hpp__
#define __XDCAMEX_Handler_hpp__	1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2008 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! This must be the first include.

#include "XMPFiles/source/XMPFiles_Impl.hpp"

#include "source/ExpatAdapter.hpp"

// =================================================================================================
/// \file XDCAMEX_Handler.hpp
/// \brief Folder format handler for XDCAMEX.
// =================================================================================================

extern XMPFileHandler * XDCAMEX_MetaHandlerCTor ( XMPFiles * parent );

extern bool XDCAMEX_CheckFormat ( XMP_FileFormat format,
								  const std::string & rootPath,
								  const std::string & gpName,
								  const std::string & parentName,
								  const std::string & leafName,
								  XMPFiles * parent );

static const XMP_OptionBits kXDCAMEX_HandlerFlags = (kXMPFiles_CanInjectXMP |
													 kXMPFiles_CanExpand |
													 kXMPFiles_CanRewrite |
													 kXMPFiles_PrefersInPlace |
													 kXMPFiles_CanReconcile |
													 kXMPFiles_AllowsOnlyXMP |
													 kXMPFiles_ReturnsRawPacket |
													 kXMPFiles_HandlerOwnsFile |
													 kXMPFiles_AllowsSafeUpdate |
													 kXMPFiles_FolderBasedFormat);

class XDCAMEX_MetaHandler : public XMPFileHandler
{
public:

	bool GetFileModDate ( XMP_DateTime * modDate );
	
	void FillMetadataFiles ( std::vector<std::string> * metadataFiles );
	void FillAssociatedResources ( std::vector<std::string> * resourceList );
	bool IsMetadataWritable ( );

	void CacheFileData();
	void ProcessXMP();

	void UpdateFile ( bool doSafeUpdate );
    void WriteTempFile ( XMP_IO* tempRef );

	XMP_OptionBits GetSerializeOptions()	// *** These should be standard for standalone XMP files.
		{ return (kXMP_UseCompactFormat | kXMP_OmitPacketWrapper); };

	XDCAMEX_MetaHandler ( XMPFiles * _parent );
	virtual ~XDCAMEX_MetaHandler();

private:

	XDCAMEX_MetaHandler() : expat(0), clipMetadata(0) {};	// Hidden on purpose.

	bool MakeClipFilePath ( std::string * path, XMP_StringPtr suffix, bool checkFile = false );
	bool MakeMediaproPath ( std::string * path, bool checkFile = false );
	void MakeLegacyDigest ( std::string * digestStr );

	void GetTakeUMID ( const std::string& clipUMID, std::string& takeUMID, std::string& takeXMLURI );
	void GetTakeDuration ( const std::string& takeUMID, std::string& duration );
	bool GetMediaProMetadata ( SXMPMeta * xmpObjPtr, const std::string& clipUMID, bool digestFound );

	void CleanupLegacyXML();

	std::string rootPath, clipName, xdcNS, legacyNS, clipUMID;

	// Used to Parse the Non-XMP /non real time metadata file associated with the clip 
	ExpatAdapter * expat;
	XML_Node * clipMetadata;

};	// XDCAMEX_MetaHandler

// =================================================================================================

#endif /* __XDCAMEX_Handler_hpp__ */
