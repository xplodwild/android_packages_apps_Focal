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

#include "XMPFiles/source/FileHandlers/PNG_Handler.hpp"
#include "XMPFiles/source/FormatSupport/PNG_Support.hpp"

#include "source/XIO.hpp"

using namespace std;

// =================================================================================================
/// \file PNG_Handler.hpp
/// \brief File format handler for PNG.
///
/// This handler ...
///
// =================================================================================================

// =================================================================================================
// PNG_MetaHandlerCTor
// ====================

XMPFileHandler * PNG_MetaHandlerCTor ( XMPFiles * parent )
{
	return new PNG_MetaHandler ( parent );

}	// PNG_MetaHandlerCTor

// =================================================================================================
// PNG_CheckFormat
// ===============

bool PNG_CheckFormat ( XMP_FileFormat format,
					   XMP_StringPtr  filePath,
                       XMP_IO*    fileRef,
                       XMPFiles *     parent )
{
	IgnoreParam(format); IgnoreParam(fileRef); IgnoreParam(parent);
	XMP_Assert ( format == kXMP_PNGFile );

	if ( fileRef->Length() < PNG_SIGNATURE_LEN ) return false;
	XMP_Uns8 buffer [PNG_SIGNATURE_LEN];

	fileRef->Rewind();
	fileRef->Read ( buffer, PNG_SIGNATURE_LEN );
	if ( ! CheckBytes ( buffer, PNG_SIGNATURE_DATA, PNG_SIGNATURE_LEN ) ) return false;

	return true;

}	// PNG_CheckFormat

// =================================================================================================
// PNG_MetaHandler::PNG_MetaHandler
// ==================================

PNG_MetaHandler::PNG_MetaHandler ( XMPFiles * _parent )
{
	this->parent = _parent;
	this->handlerFlags = kPNG_HandlerFlags;
	this->stdCharForm  = kXMP_Char8Bit;

}

// =================================================================================================
// PNG_MetaHandler::~PNG_MetaHandler
// ===================================

PNG_MetaHandler::~PNG_MetaHandler()
{
}

// =================================================================================================
// PNG_MetaHandler::CacheFileData
// ===============================

void PNG_MetaHandler::CacheFileData()
{

	this->containsXMP = false;

	XMP_IO* fileRef ( this->parent->ioRef );
	if ( fileRef == 0) return;

	PNG_Support::ChunkState chunkState;
	long numChunks = PNG_Support::OpenPNG ( fileRef, chunkState );
	if ( numChunks == 0 ) return;

	if (chunkState.xmpLen != 0)
	{
		// XMP present

		this->xmpPacket.reserve(chunkState.xmpLen);
		this->xmpPacket.assign(chunkState.xmpLen, ' ');

		if (PNG_Support::ReadBuffer ( fileRef, chunkState.xmpPos, chunkState.xmpLen, const_cast<char *>(this->xmpPacket.data()) ))
		{
			this->packetInfo.offset = chunkState.xmpPos;
			this->packetInfo.length = chunkState.xmpLen;
			this->containsXMP = true;
		}
	}
	else
	{
		// no XMP
	}

}	// PNG_MetaHandler::CacheFileData

// =================================================================================================
// PNG_MetaHandler::ProcessXMP
// ============================
//
// Process the raw XMP and legacy metadata that was previously cached.

void PNG_MetaHandler::ProcessXMP()
{
	this->processedXMP = true;	// Make sure we only come through here once.

	// Process the XMP packet.

	if ( ! this->xmpPacket.empty() ) {

		XMP_Assert ( this->containsXMP );
		XMP_StringPtr packetStr = this->xmpPacket.c_str();
		XMP_StringLen packetLen = (XMP_StringLen)this->xmpPacket.size();

		this->xmpObj.ParseFromBuffer ( packetStr, packetLen );

		this->containsXMP = true;

	}

}	// PNG_MetaHandler::ProcessXMP

// =================================================================================================
// PNG_MetaHandler::UpdateFile
// ============================

void PNG_MetaHandler::UpdateFile ( bool doSafeUpdate )
{
	bool updated = false;

	if ( ! this->needsUpdate ) return;
	if ( doSafeUpdate ) XMP_Throw ( "PNG_MetaHandler::UpdateFile: Safe update not supported", kXMPErr_Unavailable );

	XMP_StringPtr packetStr = xmpPacket.c_str();
	XMP_StringLen packetLen = (XMP_StringLen)xmpPacket.size();
	if ( packetLen == 0 ) return;

	XMP_IO* fileRef(this->parent->ioRef);
	if ( fileRef == 0 ) return;

	PNG_Support::ChunkState chunkState;
	long numChunks = PNG_Support::OpenPNG ( fileRef, chunkState );
	if ( numChunks == 0 ) return;

	// write/update chunk
	if (chunkState.xmpLen == 0)
	{
		// no current chunk -> inject
		updated = SafeWriteFile();
	}
	else if (chunkState.xmpLen >= packetLen )
	{
		// current chunk size is sufficient -> write and update CRC (in place update)
		updated = PNG_Support::WriteBuffer(fileRef, chunkState.xmpPos, packetLen, packetStr );
		PNG_Support::UpdateChunkCRC(fileRef, chunkState.xmpChunk );
	}
	else if (chunkState.xmpLen < packetLen)
	{
		// XMP is too large for current chunk -> expand
		updated = SafeWriteFile();
	}

	if ( ! updated )return;	// If there's an error writing the chunk, bail.

	this->needsUpdate = false;

}	// PNG_MetaHandler::UpdateFile

// =================================================================================================
// PNG_MetaHandler::WriteTempFile
// ==============================

void PNG_MetaHandler::WriteTempFile ( XMP_IO* tempRef )
{
	XMP_IO* originalRef = this->parent->ioRef;

	PNG_Support::ChunkState chunkState;
	long numChunks = PNG_Support::OpenPNG ( originalRef, chunkState );
	if ( numChunks == 0 ) return;

	tempRef->Truncate ( 0 );
	tempRef->Write ( PNG_SIGNATURE_DATA, PNG_SIGNATURE_LEN );

	PNG_Support::ChunkIterator curPos = chunkState.chunks.begin();
	PNG_Support::ChunkIterator endPos = chunkState.chunks.end();

	for (; (curPos != endPos); ++curPos)
	{
		PNG_Support::ChunkData chunk = *curPos;

		// discard existing XMP chunk
		if (chunk.xmp)
			continue;

		// copy any other chunk
		PNG_Support::CopyChunk(originalRef, tempRef, chunk);

		// place XMP chunk immediately after IHDR-chunk
		if (PNG_Support::CheckIHDRChunkHeader(chunk))
		{
			XMP_StringPtr packetStr = xmpPacket.c_str();
			XMP_StringLen packetLen = (XMP_StringLen)xmpPacket.size();

			PNG_Support::WriteXMPChunk(tempRef, packetLen, packetStr );
		}
	}

}	// PNG_MetaHandler::WriteTempFile

// =================================================================================================
// PNG_MetaHandler::SafeWriteFile
// ==============================

bool PNG_MetaHandler::SafeWriteFile()
{
	XMP_IO* originalFile = this->parent->ioRef;
	XMP_IO* tempFile = originalFile->DeriveTemp();
	if ( tempFile == 0 ) XMP_Throw ( "Failure creating PNG temp file", kXMPErr_InternalFailure );

	this->WriteTempFile ( tempFile );
	originalFile->AbsorbTemp();

	return true;

} // PNG_MetaHandler::SafeWriteFile

// =================================================================================================
