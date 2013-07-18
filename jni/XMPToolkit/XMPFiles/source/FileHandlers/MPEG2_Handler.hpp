#ifndef __MPEG2_Handler_hpp__
#define __MPEG2_Handler_hpp__	1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2005 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/XMPFiles_Impl.hpp"

// =================================================================================================
/// \file MPEG2_Handler.hpp
/// \brief File format handler for MPEG2.
///
/// This header ...
///
// =================================================================================================

extern XMPFileHandler * MPEG2_MetaHandlerCTor ( XMPFiles * parent );

extern bool MPEG2_CheckFormat ( XMP_FileFormat format,
							    XMP_StringPtr  filePath,
							    XMP_IO *       fileRef,
							    XMPFiles *     parent);

static const XMP_OptionBits kMPEG2_HandlerFlags = ( kXMPFiles_CanInjectXMP |
													kXMPFiles_CanExpand |
													kXMPFiles_CanRewrite |
													kXMPFiles_AllowsOnlyXMP |
													kXMPFiles_ReturnsRawPacket |
													kXMPFiles_HandlerOwnsFile |
													kXMPFiles_AllowsSafeUpdate |
													kXMPFiles_UsesSidecarXMP );

class MPEG2_MetaHandler : public XMPFileHandler
{
public:

	std::string sidecarPath;

	MPEG2_MetaHandler ( XMPFiles * parent );
	~MPEG2_MetaHandler();

	bool GetFileModDate ( XMP_DateTime * modDate );
	void FillAssociatedResources ( std::vector<std::string> * resourceList );
	bool IsMetadataWritable ( ) ;

	void CacheFileData();

	void UpdateFile ( bool doSafeUpdate );
    void WriteTempFile ( XMP_IO * tempRef );


};	// MPEG2_MetaHandler

// =================================================================================================

#endif /* __MPEG2_Handler_hpp__ */
