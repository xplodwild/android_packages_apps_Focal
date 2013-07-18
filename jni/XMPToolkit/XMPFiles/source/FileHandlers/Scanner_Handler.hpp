#ifndef __Scanner_Handler_hpp__
#define __Scanner_Handler_hpp__	1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2004 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "XMPFiles/source/FileHandlers/Trivial_Handler.hpp"

// =================================================================================================
/// \file Scanner_Handler.hpp
/// \brief File format handler for packet scanning.
///
/// This header ...
///
// =================================================================================================

extern XMPFileHandler * Scanner_MetaHandlerCTor ( XMPFiles * parent );

static const XMP_OptionBits kScanner_HandlerFlags = kTrivial_HandlerFlags;

class Scanner_MetaHandler : public Trivial_MetaHandler
{
public:

	Scanner_MetaHandler () {};
	Scanner_MetaHandler ( XMPFiles * parent );

	~Scanner_MetaHandler();

	void CacheFileData();

};	// Scanner_MetaHandler

// =================================================================================================

#endif /* __Scanner_Handler_hpp__ */
