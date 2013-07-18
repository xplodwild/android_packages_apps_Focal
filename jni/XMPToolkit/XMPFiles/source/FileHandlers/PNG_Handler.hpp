#ifndef __PNG_Handler_hpp__
#define __PNG_Handler_hpp__	1

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

#include "XMPFiles/source/FormatSupport/PNG_Support.hpp"
#include "source/XIO.hpp"

// =================================================================================================
/// \file PNG_Handler.hpp
/// \brief File format handler for PNG.
///
/// This header ...
///
// =================================================================================================

// *** Could derive from Basic_Handler - buffer file tail in a temp file.

extern XMPFileHandler* PNG_MetaHandlerCTor ( XMPFiles* parent );

extern bool PNG_CheckFormat ( XMP_FileFormat format,
							  XMP_StringPtr  filePath,
                              XMP_IO *       fileRef,
                              XMPFiles *     parent );

static const XMP_OptionBits kPNG_HandlerFlags = ( kXMPFiles_CanInjectXMP |
												  kXMPFiles_CanExpand |
												  kXMPFiles_PrefersInPlace |
												  kXMPFiles_AllowsOnlyXMP |
												  kXMPFiles_ReturnsRawPacket |
												  kXMPFiles_NeedsReadOnlyPacket );

class PNG_MetaHandler : public XMPFileHandler
{
public:

	void CacheFileData();
	void ProcessXMP();

	void UpdateFile ( bool doSafeUpdate );
    void WriteTempFile ( XMP_IO* tempRef );

	bool SafeWriteFile();

	PNG_MetaHandler ( XMPFiles* parent );
	virtual ~PNG_MetaHandler();

};	// PNG_MetaHandler

// =================================================================================================

#endif /* __PNG_Handler_hpp__ */
