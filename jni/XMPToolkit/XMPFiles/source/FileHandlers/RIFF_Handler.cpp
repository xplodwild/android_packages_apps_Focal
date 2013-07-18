// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2009 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.
#include "public/include/XMP_Const.h"

#include "XMPFiles/source/FormatSupport/RIFF.hpp"
#include "XMPFiles/source/FileHandlers/RIFF_Handler.hpp"
#include "source/XIO.hpp"

using namespace std;

// =================================================================================================
/// \file RIFF_Handler.cpp
/// \brief File format handler for RIFF.
// =================================================================================================

// =================================================================================================
// RIFF_MetaHandlerCTor
// ====================

XMPFileHandler * RIFF_MetaHandlerCTor ( XMPFiles * parent )
{
	return new RIFF_MetaHandler ( parent );
}

// =================================================================================================
// RIFF_CheckFormat
// ===============
//
// An RIFF file must begin with "RIFF", a 4 byte length, then the chunkType (AVI or WAV)
// The specified length MUST (in practice: SHOULD) match fileSize-8, but we don't bother checking this here.

bool RIFF_CheckFormat ( XMP_FileFormat  format,
					   XMP_StringPtr    filePath,
			           XMP_IO*      	file,
			           XMPFiles*        parent )
{
	IgnoreParam(format); IgnoreParam(parent);
	XMP_Assert ( (format == kXMP_AVIFile) || (format == kXMP_WAVFile) );

	if ( file->Length() < 12 ) return false;
	file ->Rewind();

	XMP_Uns8 chunkID[12];
	file->ReadAll ( chunkID, 12 );
	if ( ! CheckBytes( &chunkID[0], "RIFF", 4 )) return false;

	if ( CheckBytes(&chunkID[8],"AVI ",4) && format == kXMP_AVIFile ) return true;
	if ( CheckBytes(&chunkID[8],"WAVE",4) && format == kXMP_WAVFile ) return true;

	return false;

}	// RIFF_CheckFormat

// =================================================================================================
// RIFF_MetaHandler::RIFF_MetaHandler
// ================================

RIFF_MetaHandler::RIFF_MetaHandler ( XMPFiles * _parent )
{
	this->parent = _parent;
	this->handlerFlags = kRIFF_HandlerFlags;
	this->stdCharForm  = kXMP_Char8Bit;

	this->oldFileSize = this->newFileSize = this->trailingGarbageSize = 0;
	this->level = 0;
	this->listInfoChunk = this->listTdatChunk = 0;
	this->dispChunk = this->bextChunk = this->cr8rChunk = this->prmlChunk = 0;
	this->xmpChunk = 0;
	this->lastChunk = 0;
	this->hasListInfoINAM = false;
}

// =================================================================================================
// RIFF_MetaHandler::~RIFF_MetaHandler
// =================================

RIFF_MetaHandler::~RIFF_MetaHandler()
{
	while ( ! this->riffChunks.empty() )
	{
		delete this->riffChunks.back();
		this->riffChunks.pop_back();
	}
}

// =================================================================================================
// RIFF_MetaHandler::CacheFileData
// ==============================

void RIFF_MetaHandler::CacheFileData()
{
	this->containsXMP = false; //assume for now

	XMP_IO* file = this->parent->ioRef;
	this->oldFileSize = file ->Length();
	if ( (this->parent->format == kXMP_WAVFile) && (this->oldFileSize > 0xFFFFFFFF) )
		XMP_Throw ( "RIFF_MetaHandler::CacheFileData: WAV Files larger 4GB not supported", kXMPErr_Unimplemented );

	file ->Rewind();
	this->level = 0;

	// parse top-level chunks (most likely just one, except large avi files)
	XMP_Int64 filePos = 0;
	while ( filePos < this->oldFileSize )
	{

		this->riffChunks.push_back( (RIFF::ContainerChunk*) RIFF::getChunk( NULL, this ) );

		// Tolerate limited forms of trailing garbage in a file. Some apps append private data.

		filePos = file->Offset();
		XMP_Int64 fileTail = this->oldFileSize - filePos;

		if ( fileTail != 0 ) {

			if ( fileTail < 12 ) {

				this->oldFileSize = filePos;	// Pretend the file is smaller.
				this->trailingGarbageSize = fileTail;

			} else if ( this->parent->format == kXMP_WAVFile ) {

				if ( fileTail < 1024*1024 ) {
					this->oldFileSize = filePos;	// Pretend the file is smaller.
					this->trailingGarbageSize = fileTail;
				} else {
					XMP_Throw ( "Excessive garbage at end of file", kXMPErr_BadFileFormat )
				}

			} else {

				XMP_Int32 chunkInfo [3];
				file->ReadAll ( &chunkInfo, 12 );
				file->Seek ( -12, kXMP_SeekFromCurrent );
				if ( (GetUns32LE ( &chunkInfo[0] ) != RIFF::kChunk_RIFF) || (GetUns32LE ( &chunkInfo[2] ) != RIFF::kType_AVIX) ) {
					if ( fileTail < 1024*1024 ) {
						this->oldFileSize = filePos;	// Pretend the file is smaller.
						this->trailingGarbageSize = fileTail;
					} else {
						XMP_Throw ( "Excessive garbage at end of file", kXMPErr_BadFileFormat )
					}
				}

			}

		}

	}

	// covered before => internal error if it occurs
	XMP_Validate( file->Offset() == this->oldFileSize,
		          "RIFF_MetaHandler::CacheFileData: unknown data at end of file",
				  kXMPErr_InternalFailure );

}	// RIFF_MetaHandler::CacheFileData

// =================================================================================================
// RIFF_MetaHandler::ProcessXMP
// ============================

void RIFF_MetaHandler::ProcessXMP()
{
	SXMPUtils::RemoveProperties ( &this->xmpObj, 0, 0, kXMPUtil_DoAllProperties );
	// process physical packet first
	if ( this->containsXMP ) this->xmpObj.ParseFromBuffer ( this->xmpPacket.c_str(), (XMP_StringLen)this->xmpPacket.size() );
	// then import native properties:
	RIFF::importProperties( this );
	this->processedXMP = true;
}

// =================================================================================================
// RIFF_MetaHandler::UpdateFile
// ===========================

void RIFF_MetaHandler::UpdateFile ( bool doSafeUpdate )
{
	XMP_Validate( this->needsUpdate, "nothing to update", kXMPErr_InternalFailure );

	////////////////////////////////////////////////////////////////////////////////////////
	//////////// PASS 1: basics, exports, packet reserialze
	XMP_IO* file = this->parent->ioRef;
	RIFF::containerVect *rc = &this->riffChunks;

	//temptemp
	//printf( "BEFORE:\n%s\n", rc->at(0)->toString().c_str() );

	XMP_Enforce( rc->size() >= 1);
	RIFF::ContainerChunk* mainChunk = rc->at(0);
	this->lastChunk = rc->at( rc->size() - 1 );  // (may or may not coincide with mainChunk: )
	XMP_Enforce( mainChunk != NULL );

	RIFF::relocateWronglyPlacedXMPChunk( this );
	// [2435625] lifted disablement for AVI
	RIFF::exportAndRemoveProperties( this );

	// always rewrite both LISTs (implicit size changes, e.g. through 0-term corrections may
	// have very well led to size changes...)
	// set XMP packet info, re-serialize
	this->packetInfo.charForm = stdCharForm;
	this->packetInfo.writeable = true;
	this->packetInfo.offset = kXMPFiles_UnknownOffset;
	this->packetInfo.length = kXMPFiles_UnknownLength;

	// re-serialization ( needed because of above exportAndRemoveProperties() )
	try {
		if ( this->xmpChunk == 0 ) // new chunk? pad with 2K
			this->xmpObj.SerializeToBuffer ( &this->xmpPacket, kXMP_NoOptions , 2048 );
		else // otherwise try to match former size
			this->xmpObj.SerializeToBuffer ( &this->xmpPacket, kXMP_ExactPacketLength , (XMP_Uns32) this->xmpChunk->oldSize-8 );
	}   catch ( ... ) { // if that fails, be happy with whatever.
			this->xmpObj.SerializeToBuffer ( &this->xmpPacket, kXMP_NoOptions );
	}

	if ( (this->xmpPacket.size() & 1) == 1 ) this->xmpPacket += ' ';	// Force the XMP to have an even size.

	// if missing, add xmp packet at end:
	if( this->xmpChunk == 0 )
		this->xmpChunk = new RIFF::XMPChunk( this->lastChunk );
	// * parenting happens within this call.
	// * size computation will happen in XMPChunk::changesAndSize()
	// * actual data will be set in XMPChunk::write()

	////////////////////////////////////////////////////////////////////////////////////////
	// PASS 2: compute sizes, optimize container structure (no writing yet)
	{
		this->newFileSize = 0;

		// note: going through forward (not vice versa) is an important condition,
		// so that parking LIST:Tdat,Cr8r, PrmL to last chunk is doable
		// when encountered en route
		for ( XMP_Uns32 chunkNo = 0; chunkNo < rc->size(); chunkNo++ )
		{
			RIFF::Chunk* cur = rc->at(chunkNo);
			cur->changesAndSize( this );
			this->newFileSize += cur->newSize;
			if ( this->newFileSize % 2 == 1 ) this->newFileSize++; // pad byte
		}
		this->newFileSize += this->trailingGarbageSize;
	} // PASS2

	////////////////////////////////////////////////////////////////////////////////////////
	// PASS 2a: verify no chunk violates 2GB boundaries
	switch( this->parent->format )
	{
		// NB: <4GB for ALL chunks asserted before in ContainerChunk::changesAndSize()

		case kXMP_AVIFile:
			// ensure no chunk (single or multi, no matter) crosses 2 GB ...
			for ( XMP_Int32 chunkNo = 0; chunkNo < (XMP_Int32)rc->size(); chunkNo++ )
			{
				if ( rc->at(chunkNo)->oldSize <= 0x80000000LL ) // ... if <2GB before
					XMP_Validate( rc->at(chunkNo)->newSize <= 0x80000000LL,
							"Chunk grew beyond 2 GB", kXMPErr_Unimplemented );
			}

			// compatibility: if single-chunk AND below <1GB, ensure <1GB
			if ( ( rc->size() > 1 ) && ( rc->at(0)->oldSize < 0x40000000 ) )
			{
				 XMP_Validate( rc->at(0)->newSize < 0x40000000LL, "compatibility: mainChunk must remain < 1GB" , kXMPErr_Unimplemented );
			}

			// [2473381] compatibility: if single-chunk AND >2GB,<4GB, ensure <4GB
			if ( ( rc->size() > 1 ) &&
				 ( rc->at(0)->oldSize >  0x80000000LL ) && // 2GB
				 ( rc->at(0)->oldSize < 0x100000000LL ) )  // 4GB
			{
				 XMP_Validate( rc->at(0)->newSize < 0x100000000LL, "compatibility: mainChunk must remain < 4GB" , kXMPErr_Unimplemented );
			}

			break;

		case kXMP_WAVFile:
			XMP_Validate( 1 == rc->size(), "WAV must be single-chunk", kXMPErr_InternalFailure );
			XMP_Validate( rc->at(0)->newSize <= 0xFFFFFFFFLL, "WAV above 4 GB not supported", kXMPErr_Unimplemented );
			break;

		default:
			XMP_Throw( "unknown format", kXMPErr_InternalFailure );
	}

	////////////////////////////////////////////////////////////////////////////////////////
	// PASS 3: write avix chunk(s) if applicable (shrinks or stays)
	//         and main chunk. -- operation order depends on mainHasShrunk.
	{
		// if needed, extend file beforehand
		if ( this->newFileSize > this->oldFileSize ) {
			file->Seek ( newFileSize, kXMP_SeekFromStart );
			file->Rewind();
		}

		RIFF::Chunk* mainChunk = rc->at(0);

		XMP_Int64 mainGrowth = mainChunk->newSize - mainChunk->oldSize;
		XMP_Enforce( mainGrowth >= 0 ); // main always stays or grows

		//temptemp
		//printf( "AFTER:\n%s\n", rc->at(0)->toString().c_str() );

		if ( rc->size() > 1 ) // [2457482]
			XMP_Validate( mainGrowth == 0, "mainChunk must not grow, if multiple RIFF chunks", kXMPErr_InternalFailure );

		// back to front:

		XMP_Int64 avixStart = newFileSize; // count from the back

		if ( this->trailingGarbageSize != 0 ) {
			XMP_Int64 goodDataEnd = this->newFileSize - this->trailingGarbageSize;
			XIO::Move ( file, this->oldFileSize, file, goodDataEnd, this->trailingGarbageSize );
			avixStart = goodDataEnd;
		}

		for ( XMP_Int32 chunkNo = ((XMP_Int32)rc->size()) -1; chunkNo >= 0; chunkNo-- )
		{
			RIFF::Chunk* cur = rc->at(chunkNo);

			avixStart -= cur->newSize;
			if ( avixStart % 2 == 1 ) // rewind one more
				avixStart -= 1;

			file->Seek ( avixStart , kXMP_SeekFromStart  );

			if ( cur->hasChange ) // need explicit write-out ?
				cur->write( this, file, chunkNo == 0 );
			else // or will a simple move do?
			{
				XMP_Enforce( cur->oldSize == cur->newSize );
				if ( cur->oldPos != avixStart ) // important optimization: only move if there's a need to
					XIO::Move( file, cur->oldPos, file, avixStart, cur->newSize );
			}
		}

		// if needed, shrink file afterwards
		if ( this->newFileSize < this->oldFileSize ) file->Truncate ( this->newFileSize  );
	} // PASS 3

	this->needsUpdate = false; //do last for safety
}	// RIFF_MetaHandler::UpdateFile

// =================================================================================================
// RIFF_MetaHandler::WriteTempFile
// ===============================

void RIFF_MetaHandler::WriteTempFile ( XMP_IO* tempRef  )
{
	IgnoreParam( tempRef );
	XMP_Throw ( "RIFF_MetaHandler::WriteTempFile: Not supported (must go through UpdateFile", kXMPErr_Unavailable );
}

