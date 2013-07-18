#ifndef __Trivial_Handler_hpp__
#define __Trivial_Handler_hpp__	1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2004 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "XMPFiles/source/XMPFiles_Impl.hpp"

// =================================================================================================
/// \file Trivial_Handler.hpp
/// \brief Base class for trivial handlers that only process in-place XMP.
///
/// This header ...
///
/// \note There is no general promise here about crash-safe I/O. An update to an existing file might
/// have invalid partial state while rewriting existing XMP in-place. Crash-safe updates are managed
/// at a higher level of XMPFiles, using a temporary file and final swap of file content.
///
// =================================================================================================

static const XMP_OptionBits kTrivial_HandlerFlags = ( kXMPFiles_AllowsOnlyXMP |
                                                      kXMPFiles_ReturnsRawPacket |
                                                      kXMPFiles_AllowsSafeUpdate );

class Trivial_MetaHandler : public XMPFileHandler
{
public:

	Trivial_MetaHandler() {};
	~Trivial_MetaHandler();

	virtual void CacheFileData() = 0;

	void UpdateFile ( bool doSafeUpdate );
    void WriteTempFile ( XMP_IO* tempRef );

};	// Trivial_MetaHandler

// =================================================================================================

#endif /* __Trivial_Handler_hpp__ */
