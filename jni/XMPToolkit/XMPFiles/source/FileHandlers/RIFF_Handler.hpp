// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2009 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================
#ifndef __RIFF_Handler_hpp__
#define __RIFF_Handler_hpp__	1

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.
#include "public/include/XMP_Const.h"

#include "XMPFiles/source/FormatSupport/RIFF_Support.hpp"
#include "source/XIO.hpp"

// =================================================================================================
/// \file RIFF_Handler.hpp
/// \brief File format handler for RIFF (AVI, WAV).
// =================================================================================================

extern XMPFileHandler * RIFF_MetaHandlerCTor ( XMPFiles * parent );

extern bool RIFF_CheckFormat ( XMP_FileFormat format,
							  XMP_StringPtr  filePath,
			                  XMP_IO *       fileRef,
			                  XMPFiles *     parent );

static const XMP_OptionBits kRIFF_HandlerFlags = (kXMPFiles_CanInjectXMP |
												  kXMPFiles_CanExpand |
												  kXMPFiles_PrefersInPlace |
												  kXMPFiles_AllowsOnlyXMP |
												  kXMPFiles_ReturnsRawPacket |
												  kXMPFiles_CanReconcile
												 );

class RIFF_MetaHandler : public XMPFileHandler
{
public:
	RIFF_MetaHandler ( XMPFiles* parent );
	~RIFF_MetaHandler();

	void CacheFileData();
	void ProcessXMP();

	void UpdateFile ( bool doSafeUpdate );
    void WriteTempFile ( XMP_IO* tempRef );


	////////////////////////////////////////////////////////////////////////////////////
	// instance vars
	// most often just one RIFF:* (except for AVI,[AVIX] >1 GB)
	std::vector<RIFF::ContainerChunk*> riffChunks;
	XMP_Int64 oldFileSize, newFileSize, trailingGarbageSize;

	// state variables, needed during parsing
	XMP_Uns8  level;

	RIFF::ContainerChunk *listInfoChunk, *listTdatChunk;
	RIFF::ValueChunk* dispChunk;
	RIFF::ValueChunk* bextChunk;
	RIFF::ValueChunk* cr8rChunk;
	RIFF::ValueChunk* prmlChunk;
	RIFF::XMPChunk* xmpChunk;
	RIFF::ContainerChunk* lastChunk;
	bool hasListInfoINAM; // needs to be known for the special 3-way merge around dc:title

};	// RIFF_MetaHandler

// =================================================================================================

#endif /* __RIFF_Handler_hpp__ */
