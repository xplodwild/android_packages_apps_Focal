#ifndef __FLV_Handler_hpp__
#define __FLV_Handler_hpp__	1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "source/XMPFiles_IO.hpp"

//  ================================================================================================
/// \file FLV_Handler.hpp
/// \brief File format handler for FLV.
///
/// This header ...
///
//  ================================================================================================

extern XMPFileHandler * FLV_MetaHandlerCTor ( XMPFiles * parent );

extern bool FLV_CheckFormat ( XMP_FileFormat format,
							  XMP_StringPtr  filePath,
							  XMP_IO*        fileRef,
							  XMPFiles *     parent );

static const XMP_OptionBits kFLV_HandlerFlags = ( kXMPFiles_CanInjectXMP |
												  kXMPFiles_CanExpand |
												  kXMPFiles_CanRewrite |
												  kXMPFiles_PrefersInPlace |
												  kXMPFiles_CanReconcile |
												  kXMPFiles_AllowsOnlyXMP |
												  kXMPFiles_ReturnsRawPacket |
												  kXMPFiles_AllowsSafeUpdate |
												  kXMPFiles_CanNotifyProgress
												);

class FLV_MetaHandler : public XMPFileHandler
{
public:

	void CacheFileData();
	void ProcessXMP();

	void UpdateFile ( bool doSafeUpdate );
    void WriteTempFile ( XMP_IO* tempRef );


	FLV_MetaHandler ( XMPFiles * _parent );
	virtual ~FLV_MetaHandler();

private:

	FLV_MetaHandler() : flvHeaderLen(0), longXMP(false),
						xmpTagPos(0), omdTagPos(0), xmpTagLen(0), omdTagLen(0) {};	// Hidden on purpose.

	void ExtractLiveXML();
	void MakeLegacyDigest ( std::string * digestStr );

	XMP_Uns32 flvHeaderLen;
	bool longXMP;	// True if the stored XMP is a long string (4 byte length).

	XMP_Uns64 xmpTagPos, omdTagPos;	// The file offset and length of onXMP and onMetaData tags.
	XMP_Uns32 xmpTagLen, omdTagLen;	// Zero if the tag is not present.

	std::string onXMP, onMetaData;	// ! Actually contains structured binary data.

};	// FLV_MetaHandler

// =================================================================================================

#endif // __FLV_Handler_hpp__
