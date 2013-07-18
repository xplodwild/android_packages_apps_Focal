#ifndef __XDCAM_Handler_hpp__
#define __XDCAM_Handler_hpp__	1

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
/// \file XDCAM_Handler.hpp
/// \brief Folder format handler for XDCAM.
///
/// This header ...
///
// =================================================================================================

extern XMPFileHandler * XDCAM_MetaHandlerCTor ( XMPFiles * parent );

extern bool XDCAM_CheckFormat ( XMP_FileFormat format,
								const std::string & rootPath,
								const std::string & gpName,
								const std::string & parentName,
								const std::string & leafName,
								XMPFiles * parent );

static const XMP_OptionBits kXDCAM_HandlerFlags = (kXMPFiles_CanInjectXMP |
												   kXMPFiles_CanExpand |
												   kXMPFiles_CanRewrite |
												   kXMPFiles_PrefersInPlace |
												   kXMPFiles_CanReconcile |
												   kXMPFiles_AllowsOnlyXMP |
												   kXMPFiles_ReturnsRawPacket |
												   kXMPFiles_HandlerOwnsFile |
												   kXMPFiles_AllowsSafeUpdate |
												   kXMPFiles_FolderBasedFormat);

class XDCAM_MetaHandler : public XMPFileHandler
{
public:

	bool GetFileModDate ( XMP_DateTime * modDate );

	void FillMetadataFiles ( std::vector<std::string> * metadataFiles );
	void FillAssociatedResources ( std::vector<std::string> * resourceList );
	bool IsMetadataWritable ( ) ;

	void CacheFileData();
	void ProcessXMP();

	void UpdateFile ( bool doSafeUpdate );
    void WriteTempFile ( XMP_IO* tempRef );

	XMP_OptionBits GetSerializeOptions()	// *** These should be standard for standalone XMP files.
		{ return (kXMP_UseCompactFormat | kXMP_OmitPacketWrapper); };

	XDCAM_MetaHandler ( XMPFiles * _parent );
	virtual ~XDCAM_MetaHandler();

private:

	XDCAM_MetaHandler() : isFAM(false), expat(0), clipMetadata(0) {};	// Hidden on purpose.

	bool MakeClipFilePath ( std::string * path, XMP_StringPtr suffix, bool checkFile = false );
	bool MakeMediaproPath ( std::string * path, bool checkFile = false );
	void MakeLegacyDigest ( std::string * digestStr );
	void CleanupLegacyXML();
	void SetSidecarPath();
	
	void readXMLFile( XMP_StringPtr filePath,ExpatAdapter* &expat );
    bool GetClipUmid ( std::string &clipUmid ) ;
	bool IsClipsPlanning ( std::string clipUmid , XMP_StringPtr planPath ) ;
	bool RefersClipUmid ( std::string clipUmid , XMP_StringPtr editInfoPath )  ;
	bool GetInfoFilesFAM ( std::vector<std::string> &InfoList, std::string pathToFolder) ;
	bool GetPlanningFilesFAM ( std::vector<std::string> &planInfoList, std::string pathToFolder) ;
	bool GetEditInfoFilesSAM ( std::vector<std::string> &editInfoList ) ;
	void FillFAMAssociatedResources ( std::vector<std::string> * resourceList );
	void FillSAMAssociatedResources ( std::vector<std::string> * resourceList );

	bool GetMediaProMetadata ( SXMPMeta * xmpObjPtr, const std::string& clipUMID, bool digestFound );	

	std::string rootPath, clipName, xdcNS, legacyNS, sidecarPath;

	bool isFAM;

	ExpatAdapter * expat;
	XML_Node * clipMetadata;	// ! Don't delete, points into the Expat tree.

};	// XDCAM_MetaHandler

// =================================================================================================

#endif /* __XDCAM_Handler_hpp__ */
