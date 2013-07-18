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

#include "XMPFiles/source/FormatSupport/PSIR_Support.hpp"
#include "source/EndianUtils.hpp"
#include "source/XIO.hpp"

#include <string.h>

// =================================================================================================
/// \file PSIR_FileWriter.cpp
/// \brief Implementation of the file-based or read-write form of PSIR_Manager.
// =================================================================================================

// =================================================================================================
// IsMetadataImgRsrc
// =================

static inline bool IsMetadataImgRsrc ( XMP_Uns16 id )
{
	if ( id == 0 ) return false;

	int i;
	for ( i = 0; id < kPSIR_MetadataIDs[i]; ++i ) {}
	if ( id == kPSIR_MetadataIDs[i] ) return true;
	return false;

}	// IsMetadataImgRsrc

// =================================================================================================
// PSIR_FileWriter::DeleteExistingInfo
// ===================================
//
// Delete all existing info about image resources.

void PSIR_FileWriter::DeleteExistingInfo()
{
	XMP_Assert ( ! (this->memParsed && this->fileParsed) );

	if ( this->memParsed ) {
		if ( this->ownedContent ) free ( this->memContent );
	} else if ( this->fileParsed ) {
		InternalRsrcMap::iterator irPos = this->imgRsrcs.begin();
		InternalRsrcMap::iterator irEnd = this->imgRsrcs.end();
		for ( ; irPos != irEnd; ++irPos ) irPos->second.changed = true;	// Fool the InternalRsrcInfo destructor.
	}

	this->imgRsrcs.clear();

	this->memContent = 0;
	this->memLength  = 0;

	this->changed = false;
	this->legacyDeleted = false;
	this->memParsed = false;
	this->fileParsed = false;
	this->ownedContent = false;

}	// PSIR_FileWriter::DeleteExistingInfo

// =================================================================================================
// PSIR_FileWriter::~PSIR_FileWriter
// =================================

PSIR_FileWriter::~PSIR_FileWriter()
{
	XMP_Assert ( ! (this->memParsed && this->fileParsed) );

	if ( this->ownedContent ) {
		XMP_Assert ( this->memContent != 0 );
		free ( this->memContent );
	}

}	// PSIR_FileWriter::~PSIR_FileWriter

// =================================================================================================
// PSIR_FileWriter::GetImgRsrc
// ===========================

bool PSIR_FileWriter::GetImgRsrc ( XMP_Uns16 id, ImgRsrcInfo* info ) const
{
	InternalRsrcMap::const_iterator rsrcPos = this->imgRsrcs.find ( id );
	if ( rsrcPos == this->imgRsrcs.end() ) return false;

	const InternalRsrcInfo & rsrcInfo = rsrcPos->second;

	if ( info != 0 ) {
		info->id = rsrcInfo.id;
		info->dataLen = rsrcInfo.dataLen;
		info->dataPtr = rsrcInfo.dataPtr;
		info->origOffset = rsrcInfo.origOffset;
	}

	return true;

}	// PSIR_FileWriter::GetImgRsrc

// =================================================================================================
// PSIR_FileWriter::SetImgRsrc
// ===========================

void PSIR_FileWriter::SetImgRsrc ( XMP_Uns16 id, const void* clientPtr, XMP_Uns32 length )
{
	InternalRsrcInfo* rsrcPtr = 0;
	InternalRsrcMap::iterator rsrcPos = this->imgRsrcs.find ( id );

	if ( rsrcPos == this->imgRsrcs.end() ) {

		// This resource is not yet in the map, create the map entry.
		InternalRsrcMap::value_type mapValue ( id, InternalRsrcInfo ( id, length, this->fileParsed ) );
		rsrcPos = this->imgRsrcs.insert ( rsrcPos, mapValue );
		rsrcPtr = &rsrcPos->second;

	} else {

		rsrcPtr = &rsrcPos->second;

		// The resource already exists, make sure the value is actually changing.
		if ( (length == rsrcPtr->dataLen) &&
			 (memcmp ( rsrcPtr->dataPtr, clientPtr, length ) == 0) ) {
			return;
		}

		rsrcPtr->FreeData();		// Release any existing data allocation.
		rsrcPtr->dataLen = length;	// And this might be changing.

	}

	rsrcPtr->changed = true;
	rsrcPtr->dataPtr = malloc ( length );
	if ( rsrcPtr->dataPtr == 0 ) XMP_Throw ( "Out of memory", kXMPErr_NoMemory );
	memcpy ( rsrcPtr->dataPtr, clientPtr, length );	// AUDIT: Safe, malloc'ed length bytes above.

	this->changed = true;

}	// PSIR_FileWriter::SetImgRsrc

// =================================================================================================
// PSIR_FileWriter::DeleteImgRsrc
// ==============================

void PSIR_FileWriter::DeleteImgRsrc ( XMP_Uns16 id )
{
	InternalRsrcMap::iterator rsrcPos = this->imgRsrcs.find ( id );
	if ( rsrcPos == this->imgRsrcs.end() ) return;	// Nothing to delete.

	this->imgRsrcs.erase ( id );
	this->changed = true;
	if ( id != kPSIR_XMP ) this->legacyDeleted = true;

}	// PSIR_FileWriter::DeleteImgRsrc

// =================================================================================================
// PSIR_FileWriter::IsLegacyChanged
// ================================

bool PSIR_FileWriter::IsLegacyChanged()
{

	if ( ! this->changed ) return false;
	if ( this->legacyDeleted ) return true;

	InternalRsrcMap::iterator irPos = this->imgRsrcs.begin();
	InternalRsrcMap::iterator irEnd = this->imgRsrcs.end();

	for ( ; irPos != irEnd; ++irPos ) {
		const InternalRsrcInfo & rsrcInfo = irPos->second;
		if ( rsrcInfo.changed && (rsrcInfo.id != kPSIR_XMP) ) return true;
	}

	return false;	// Can get here if the XMP is the only thing changed.

}	// PSIR_FileWriter::IsLegacyChanged

// =================================================================================================
// PSIR_FileWriter::ParseMemoryResources
// =====================================

void PSIR_FileWriter::ParseMemoryResources ( const void* data, XMP_Uns32 length, bool copyData /* = true */ )
{
	this->DeleteExistingInfo();
	this->memParsed = true;
	if ( length == 0 ) return;

	// Allocate space for the full in-memory data and copy it.

	if ( ! copyData ) {
		this->memContent = (XMP_Uns8*) data;
		XMP_Assert ( ! this->ownedContent );
	} else {
		if ( length > 100*1024*1024 ) XMP_Throw ( "Outrageous length for memory-based PSIR", kXMPErr_BadPSIR );
		this->memContent = (XMP_Uns8*) malloc ( length );
		if ( this->memContent == 0 ) XMP_Throw ( "Out of memory", kXMPErr_NoMemory );
		memcpy ( this->memContent, data, length );	// AUDIT: Safe, malloc'ed length bytes above.
		this->ownedContent = true;
	}
	this->memLength = length;

	// Capture the info for all of the resources. We're using a map keyed by ID, so only one
	// resource of each ID is recognized. Redundant resources are not legit, but have been seen in
	// the field. In particular, one case has been seen of a duplicate IIM block with one empty.
	// In general we keep the first seen copy to be compatible with Photoshop. A later non-empty
	// copy will be taken though if the current one is empty.
	
	// ! Don't use map[id] to lookup, that creates a default entry if none exists!

	XMP_Uns8* psirPtr   = this->memContent;
	XMP_Uns8* psirEnd   = psirPtr + length;
	XMP_Uns8* psirLimit = psirEnd - kMinImgRsrcSize;

	while ( psirPtr <= psirLimit ) {

		XMP_Uns8* origin = psirPtr;	// The beginning of this resource.
		XMP_Uns32 type = GetUns32BE(psirPtr);
		XMP_Uns16 id = GetUns16BE(psirPtr+4);
		psirPtr += 6;	// Advance to the resource name.

		XMP_Uns8* namePtr = psirPtr;
		XMP_Uns16 nameLen = namePtr[0];			// ! The length for the Pascal string, w/ room for "+2".
		psirPtr += ((nameLen + 2) & 0xFFFE);	// ! Round up to an even offset. Yes, +2!

		if ( psirPtr > psirEnd-4 ) break;	// Bad image resource. Throw instead?

		XMP_Uns32 dataLen = GetUns32BE(psirPtr);
		psirPtr += 4;	// Advance to the resource data.

		XMP_Uns32 dataOffset = (XMP_Uns32) ( psirPtr - this->memContent );
		XMP_Uns8* nextRsrc   = psirPtr + ((dataLen + 1) & 0xFFFFFFFEUL);	// ! Round up to an even offset.

		if ( (dataLen > length) || (psirPtr > psirEnd-dataLen) ) break;	// Bad image resource. Throw instead?

		if ( type != k8BIM ) {

			XMP_Uns32 rsrcOffset = XMP_Uns32( origin - this->memContent );
			XMP_Uns32 rsrcLength = XMP_Uns32( nextRsrc - origin );	// Includes trailing pad.
			XMP_Assert ( (rsrcLength & 1) == 0 );
			this->otherRsrcs.push_back ( OtherRsrcInfo ( rsrcOffset, rsrcLength ) );

		} else {

			InternalRsrcInfo newInfo ( id, dataLen, kIsMemoryBased );
			newInfo.dataPtr = psirPtr;
			newInfo.origOffset = dataOffset;
			if ( nameLen != 0 ) newInfo.rsrcName = namePtr;

			InternalRsrcMap::iterator rsrcPos = this->imgRsrcs.find ( id );
			if ( rsrcPos == this->imgRsrcs.end() ) {
				this->imgRsrcs.insert ( rsrcPos, InternalRsrcMap::value_type ( id, newInfo ) );
			} else if ( (rsrcPos->second.dataLen == 0) && (newInfo.dataLen != 0) ) {
				rsrcPos->second = newInfo;
			}

		}

		psirPtr = nextRsrc;

	}

}	// PSIR_FileWriter::ParseMemoryResources

// =================================================================================================
// PSIR_FileWriter::ParseFileResources
// ===================================

void PSIR_FileWriter::ParseFileResources ( XMP_IO* fileRef, XMP_Uns32 length )
{
	// Parse the image resource block. We're using a map keyed by ID, so only one resource of each
	// ID is recognized. Redundant resources are not legit, but have been seen in the field. In
	// particular, one case has been seen of a duplicate IIM block with one empty. In general we
	// keep the first seen copy to be compatible with Photoshop. A later non-empty copy will be
	// taken though if the current one is empty.
	
	// ! Don't use map[id] to lookup, that creates a default entry if none exists!
	
	// PSIR layout:
	//	- Uns32 type, usually '8BIM'
	//	- Uns16 ID
	//	- PString name
	//	- Uns8 optional pad for even alignment
	//	- Uns32 data size
	//	- data
	//	- Uns8 optional pad for even alignment

	static const size_t kMinPSIRSize = 12;	// 4+2+1+1+4
	
	this->DeleteExistingInfo();
	this->fileParsed = true;
	if ( length == 0 ) return;

	XMP_Int64 psirOrigin = fileRef->Offset();	// Need this to determine the resource data offsets.
	XMP_Int64 fileEnd = psirOrigin + length;

	char nameBuffer [260];	// The name is a PString, at 1+255+1 including length and pad.

	while ( fileRef->Offset() < fileEnd ) {

		if ( ! XIO::CheckFileSpace ( fileRef, kMinPSIRSize ) ) break;	// Bad image resource.

		XMP_Int64 thisRsrcPos = fileRef->Offset();

		XMP_Uns32 type = XIO::ReadUns32_BE ( fileRef );
		XMP_Uns16 id   = XIO::ReadUns16_BE ( fileRef );

		XMP_Uns8 nameLen = XIO::ReadUns8 ( fileRef );	// ! The length for the Pascal string.
		XMP_Uns16 paddedLen = (nameLen + 2) & 0xFFFE;	// ! Round up to an even total. Yes, +2!
		if ( ! XIO::CheckFileSpace ( fileRef, paddedLen+4 ) ) break;	// Bad image resource.

		nameBuffer[0] = nameLen;
		fileRef->ReadAll ( &nameBuffer[1], paddedLen-1 );	// Include the pad byte, present for zero nameLen.

		XMP_Uns32 dataLen   = XIO::ReadUns32_BE ( fileRef );
		XMP_Uns32 dataTotal = ((dataLen + 1) & 0xFFFFFFFEUL);	// Round up to an even total.
		if ( ! XIO::CheckFileSpace ( fileRef, dataTotal ) ) break;	// Bad image resource.

		XMP_Int64 thisDataPos = fileRef->Offset();
		XMP_Int64 nextRsrcPos = thisDataPos + dataTotal;

		if ( type != k8BIM ) {
			XMP_Uns32 fullRsrcLen = (XMP_Uns32) (nextRsrcPos - thisRsrcPos);
			this->otherRsrcs.push_back ( OtherRsrcInfo ( (XMP_Uns32)thisRsrcPos, fullRsrcLen ) );
			fileRef->Seek ( nextRsrcPos, kXMP_SeekFromStart );
			continue;
		}

		InternalRsrcInfo newInfo ( id, dataLen, kIsFileBased );
		InternalRsrcMap::iterator rsrcPos = this->imgRsrcs.find ( id );
		if ( rsrcPos == this->imgRsrcs.end() ) {
			rsrcPos = this->imgRsrcs.insert ( rsrcPos, InternalRsrcMap::value_type ( id, newInfo ) );
		} else if ( (rsrcPos->second.dataLen == 0) && (newInfo.dataLen != 0) ) {
			rsrcPos->second = newInfo;
		} else {
			fileRef->Seek ( nextRsrcPos, kXMP_SeekFromStart );
			continue;
		}
		InternalRsrcInfo* rsrcPtr = &rsrcPos->second;

		rsrcPtr->origOffset = (XMP_Uns32)thisDataPos;

		if ( nameLen > 0 ) {
			rsrcPtr->rsrcName = (XMP_Uns8*) malloc ( paddedLen );
			if ( rsrcPtr->rsrcName == 0 ) XMP_Throw ( "Out of memory", kXMPErr_NoMemory );
			memcpy ( (void*)rsrcPtr->rsrcName, nameBuffer, paddedLen );	// AUDIT: Safe, allocated enough bytes above.
		}

		if ( ! IsMetadataImgRsrc ( id ) ) {
			fileRef->Seek ( nextRsrcPos, kXMP_SeekFromStart );
			continue;
		}

		rsrcPtr->dataPtr = malloc ( dataTotal );	// ! Allocate after the IsMetadataImgRsrc check.
		if ( rsrcPtr->dataPtr == 0 ) XMP_Throw ( "Out of memory", kXMPErr_NoMemory );
		fileRef->ReadAll ( (void*)rsrcPtr->dataPtr, dataTotal );

	}

	#if 0
	{
		printf ( "\nPSIR_FileWriter::ParseFileResources, count = %d\n", this->imgRsrcs.size() );
		InternalRsrcMap::iterator irPos = this->imgRsrcs.begin();
		InternalRsrcMap::iterator irEnd = this->imgRsrcs.end();
		for ( ; irPos != irEnd; ++irPos ) {
			InternalRsrcInfo& thisRsrc = irPos->second;
			printf ( "  #%d, dataLen %d, origOffset %d (0x%X)%s\n",
					 thisRsrc.id, thisRsrc.dataLen, thisRsrc.origOffset, thisRsrc.origOffset,
					 (thisRsrc.changed ? ", changed" : "") );
		}
	}
	#endif

}	// PSIR_FileWriter::ParseFileResources

// =================================================================================================
// PSIR_FileWriter::UpdateMemoryResources
// ======================================

XMP_Uns32 PSIR_FileWriter::UpdateMemoryResources ( void** dataPtr )
{
	if ( this->fileParsed ) XMP_Throw ( "Not memory based", kXMPErr_EnforceFailure );

	// Compute the size and allocate the new image resource block.

	XMP_Uns32 newLength = 0;

	InternalRsrcMap::iterator irPos = this->imgRsrcs.begin();
	InternalRsrcMap::iterator irEnd = this->imgRsrcs.end();

	for ( ; irPos != irEnd; ++irPos ) {	// Add in the lengths for the 8BIM resources.
		const InternalRsrcInfo & rsrcInfo = irPos->second;
		newLength += 10;
		newLength += ((rsrcInfo.dataLen + 1) & 0xFFFFFFFEUL);
		if ( rsrcInfo.rsrcName == 0 ) {
			newLength += 2;
		} else {
			XMP_Uns32 nameLen = rsrcInfo.rsrcName[0];
			newLength += ((nameLen + 2) & 0xFFFFFFFEUL);	// ! Yes, +2 for the length and rounding.
		}
	}

	for ( size_t i = 0; i < this->otherRsrcs.size(); ++i ) {	// Add in the non-8BIM resources.
		newLength += this->otherRsrcs[i].rsrcLength;
	}

	XMP_Uns8* newContent = (XMP_Uns8*) malloc ( newLength );
	if ( newContent == 0 ) XMP_Throw ( "Out of memory", kXMPErr_NoMemory );

	// Fill in the new image resource block.

	XMP_Uns8* rsrcPtr = newContent;

	for ( irPos = this->imgRsrcs.begin(); irPos != irEnd; ++irPos ) {	// Do the 8BIM resources.

		const InternalRsrcInfo & rsrcInfo = irPos->second;

		PutUns32BE ( k8BIM, rsrcPtr );
		rsrcPtr += 4;
		PutUns16BE ( rsrcInfo.id, rsrcPtr );
		rsrcPtr += 2;

		if ( rsrcInfo.rsrcName == 0 ) {
			PutUns16BE ( 0, rsrcPtr );
			rsrcPtr += 2;
		} else {
			XMP_Uns32 nameLen = rsrcInfo.rsrcName[0];
			XMP_Assert ( nameLen > 0 );
			if ( (nameLen+1) > (newLength - (rsrcPtr - newContent)) ) {
				XMP_Throw ( "Buffer overrun", kXMPErr_InternalFailure );
			}
			memcpy ( rsrcPtr, rsrcInfo.rsrcName, nameLen+1 );	// AUDIT: Protected by the above check.
			rsrcPtr += nameLen+1;
			if ( (nameLen & 1) == 0 ) {
				*rsrcPtr = 0;	// Round to an even total.
				++rsrcPtr;
			}
		}

		PutUns32BE ( rsrcInfo.dataLen, rsrcPtr );
		rsrcPtr += 4;
		if ( rsrcInfo.dataLen > (newLength - (rsrcPtr - newContent)) ) {
			XMP_Throw ( "Buffer overrun", kXMPErr_InternalFailure );
		}
		memcpy ( rsrcPtr, rsrcInfo.dataPtr, rsrcInfo.dataLen );	// AUDIT: Protected by the above check.
		rsrcPtr += rsrcInfo.dataLen;
		if ( (rsrcInfo.dataLen & 1) != 0 ) {	// Pad to an even length if necessary.
			*rsrcPtr = 0;
			++rsrcPtr;
		}

	}

	for ( size_t i = 0; i < this->otherRsrcs.size(); ++i ) {	// Do the non-8BIM resources.
		XMP_Uns8* srcPtr = this->memContent + this->otherRsrcs[i].rsrcOffset;
		XMP_Uns32 srcLen = this->otherRsrcs[i].rsrcLength;
		if ( srcLen > (newLength - (rsrcPtr - newContent)) ) {
			XMP_Throw ( "Buffer overrun", kXMPErr_InternalFailure );
		}
		memcpy ( rsrcPtr, srcPtr, srcLen );	// AUDIT: Protected by the above check.
		rsrcPtr += srcLen;	// No need to pad, included in the original resource length.
	}

	XMP_Assert ( rsrcPtr == (newContent + newLength) );

	// Parse the rebuilt image resource block. This is the easiest way to reconstruct the map.

	this->ParseMemoryResources ( newContent, newLength, false );
	this->ownedContent = (newLength > 0);	// ! We really do own the new content, if not empty.

	if ( dataPtr != 0 ) *dataPtr = newContent;
	return newLength;

}	// PSIR_FileWriter::UpdateMemoryResources

// =================================================================================================
// PSIR_FileWriter::UpdateFileResources
// ====================================

XMP_Uns32 PSIR_FileWriter::UpdateFileResources ( XMP_IO* sourceRef, XMP_IO* destRef,
												 XMP_AbortProc abortProc, void * abortArg,
												 XMP_ProgressTracker* progressTracker )
{
	const XMP_Uns32 zero32 = 0;
	const bool checkAbort = (abortProc != 0);

	struct RsrcHeader {
		XMP_Uns32 type;
		XMP_Uns16 id;
	};
	XMP_Assert ( (offsetof(RsrcHeader,type) == 0) && (offsetof(RsrcHeader,id) == 4) );

	if ( this->memParsed ) XMP_Throw ( "Not file based", kXMPErr_EnforceFailure );

	InternalRsrcMap::const_iterator rsrcPos;
	InternalRsrcMap::const_iterator rsrcEnd = this->imgRsrcs.end();

	if ( progressTracker != 0 ) {

		float totalLength = 8;
		for ( rsrcPos = this->imgRsrcs.begin(); rsrcPos != rsrcEnd; ++rsrcPos ) {
			 const InternalRsrcInfo& currRsrc = rsrcPos->second;
			totalLength += (currRsrc.dataLen + 12);
		}

		size_t sizeOtherRsrc = this->otherRsrcs.size();
		for ( size_t i = 0; i < sizeOtherRsrc; ++i ) { 
			totalLength += this->otherRsrcs[i].rsrcLength;	
		}

		XMP_Assert ( progressTracker->WorkInProgress() );
		progressTracker->AddTotalWork ( totalLength );

	}

	XMP_Uns32 destLength = 0;
	XMP_Int64 destLenOffset = destRef->Offset();
	destRef->Write ( &destLength, 4 );	// Write a placeholder for the new PSIR section length.

	#if 0
	{
		printf ( "\nPSIR_FileWriter::UpdateFileResources, count = %d\n", this->imgRsrcs.size() );
		InternalRsrcMap::iterator irPos = this->imgRsrcs.begin();
		InternalRsrcMap::iterator irEnd = this->imgRsrcs.end();
		for ( ; irPos != irEnd; ++irPos ) {
			InternalRsrcInfo& thisRsrc = irPos->second;
			printf ( "  #%d, dataLen %d, origOffset %d (0x%X)%s\n",
					 thisRsrc.id, thisRsrc.dataLen, thisRsrc.origOffset, thisRsrc.origOffset,
					 (thisRsrc.changed ? ", changed" : "") );
		}
	}
	#endif

	// First write all of the '8BIM' resources from the map. Use the internal data if present, else
	// copy the data from the file.

	RsrcHeader outHeader;
	outHeader.type  = MakeUns32BE ( k8BIM );

	// printf ( "\nPSIR_FileWriter::UpdateFileResources - 8BIM resources\n" );
	for ( rsrcPos = this->imgRsrcs.begin(); rsrcPos != rsrcEnd; ++rsrcPos ) {

		const InternalRsrcInfo& currRsrc = rsrcPos->second;

		outHeader.id = MakeUns16BE ( currRsrc.id );
		destRef->Write ( &outHeader, 6 );
		destLength += 6;

		if ( currRsrc.rsrcName == 0 ) {
			destRef->Write ( &zero32, 2 );
			destLength += 2;
		} else {
			XMP_Uns16 nameLen = currRsrc.rsrcName[0];	// ! Include room for +1.
			XMP_Assert ( nameLen > 0 );
			XMP_Uns16 paddedLen = (nameLen + 2) & 0xFFFE;	// ! Round up to an even total. Yes, +2!
			destRef->Write ( currRsrc.rsrcName, paddedLen );
			destLength += paddedLen;
		}

		XMP_Uns32 dataLen = MakeUns32BE ( currRsrc.dataLen );
		destRef->Write ( &dataLen, 4 );
		// printf ( "  #%d, offset %d (0x%X), dataLen %d\n", currRsrc.id, destLength, destLength, currRsrc.dataLen );

		if ( currRsrc.dataPtr != 0 ) {
			destRef->Write ( currRsrc.dataPtr, currRsrc.dataLen );
		} else {
			sourceRef->Seek ( currRsrc.origOffset, kXMP_SeekFromStart  );
			XIO::Copy ( sourceRef, destRef, currRsrc.dataLen );
		}

		destLength += 4 + currRsrc.dataLen;

		if ( (currRsrc.dataLen & 1) != 0 ) {
			destRef->Write ( &zero32, 1 );	// ! Pad the data to an even length.
			++destLength;
		}

	}

	// Now write all of the non-8BIM resources. Copy the entire resource chunk from the source file.

	// printf ( "\nPSIR_FileWriter::UpdateFileResources - other resources\n" );
	for ( size_t i = 0; i < this->otherRsrcs.size(); ++i ) {
		// printf ( "  offset %d (0x%X), length %d",
		//		 this->otherRsrcs[i].rsrcOffset, this->otherRsrcs[i].rsrcOffset, this->otherRsrcs[i].rsrcLength );
		sourceRef->Seek ( this->otherRsrcs[i].rsrcOffset, kXMP_SeekFromStart  );
		XIO::Copy ( sourceRef, destRef, this->otherRsrcs[i].rsrcLength );
		destLength += this->otherRsrcs[i].rsrcLength;	// Alignment padding is already included.
	}

	// Write the final PSIR section length, seek back to the end of the file, return the length.

	// printf ( "\nPSIR_FileWriter::UpdateFileResources - final length %d (0x%X)\n", destLength, destLength );
	destRef->Seek ( destLenOffset, kXMP_SeekFromStart  );
	XMP_Uns32 outLen = MakeUns32BE ( destLength );
	destRef->Write ( &outLen, 4 );
	destRef->Seek ( 0, kXMP_SeekFromEnd  );

	// *** Not rebuilding the internal map - turns out we never want it, why pay for the I/O.
	// *** Should probably add an option for all of these cases, memory and file based.

	return destLength;

}	// PSIR_FileWriter::UpdateFileResources
