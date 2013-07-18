#ifndef __SWF_Handler_hpp__
#define __SWF_Handler_hpp__	1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2006 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "XMPFiles/source/FormatSupport/SWF_Support.hpp"

// =================================================================================================
/// \file SWF_Handler.hpp
/// \brief File format handler for SWF.
///
/// This header ...
///
// =================================================================================================

extern XMPFileHandler* SWF_MetaHandlerCTor ( XMPFiles* parent );

extern bool SWF_CheckFormat ( XMP_FileFormat format,
							  XMP_StringPtr  filePath,
                              XMP_IO *       fileRef,
                              XMPFiles *     parent );

static const XMP_OptionBits kSWF_HandlerFlags = ( kXMPFiles_CanInjectXMP |
												  kXMPFiles_CanExpand |
												  kXMPFiles_PrefersInPlace |
												  kXMPFiles_AllowsOnlyXMP |
												  kXMPFiles_ReturnsRawPacket );

class SWF_MetaHandler : public XMPFileHandler {

public:

	void CacheFileData();
	void ProcessXMP();

	void UpdateFile ( bool doSafeUpdate );
    void WriteTempFile ( XMP_IO* tempRef );

	XMP_OptionBits GetSerializeOptions();

	SWF_MetaHandler ( XMPFiles* parent );
	virtual ~SWF_MetaHandler();
	
private:

	SWF_MetaHandler() : isCompressed(false), hasFileAttributes(false), hasMetadata(false), brokenSWF(false),
						expandedSize(0), firstTagOffset(0) {};

	bool isCompressed, hasFileAttributes, hasMetadata, brokenSWF;
	XMP_Uns32 expandedSize, firstTagOffset;
	RawDataBlock expandedSWF;
	
	SWF_IO::TagInfo fileAttributesTag, metadataTag;

};	// SWF_MetaHandler

// =================================================================================================

#endif /* __SWF_Handler_hpp__ */
