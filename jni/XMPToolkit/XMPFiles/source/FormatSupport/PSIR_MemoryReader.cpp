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
/// \file PSIR_MemoryReader.cpp
/// \brief Implementation of the memory-based read-only form of PSIR_Manager.
// =================================================================================================

// =================================================================================================
// PSIR_MemoryReader::GetImgRsrc
// =============================

bool PSIR_MemoryReader::GetImgRsrc ( XMP_Uns16 id, ImgRsrcInfo* info ) const
{
	ImgRsrcMap::const_iterator rsrcPos = this->imgRsrcs.find ( id );
	if ( rsrcPos == this->imgRsrcs.end() ) return false;
	
	if ( info != 0 ) *info = rsrcPos->second;
	return true;
	
}	// PSIR_MemoryReader::GetImgRsrc

// =================================================================================================
// PSIR_MemoryReader::ParseMemoryResources
// =======================================

void PSIR_MemoryReader::ParseMemoryResources ( const void* data, XMP_Uns32 length, bool copyData /* = true */ )
{
	// Get rid of any existing image resources.
	
	if ( this->ownedContent ) free ( this->psirContent );
	this->ownedContent = false;
	this->psirContent  = 0;
	this->psirLength   = 0;
	this->imgRsrcs.clear();
	
	if ( length == 0 ) return;
	
	// Allocate space for the full in-memory data and copy it.
	
	if ( ! copyData ) {
		this->psirContent = (XMP_Uns8*) data;
		XMP_Assert ( ! this->ownedContent );
	} else {
		if ( length > 100*1024*1024 ) XMP_Throw ( "Outrageous length for memory-based PSIR", kXMPErr_BadPSIR );
		this->psirContent = (XMP_Uns8*) malloc(length);
		if ( this->psirContent == 0 ) XMP_Throw ( "Out of memory", kXMPErr_NoMemory );
		memcpy ( this->psirContent, data, length );	// AUDIT: Safe, malloc'ed length bytes above.
		this->ownedContent = true;
	}

	this->psirLength = length;
	
	// Capture the info for all of the resources. We're using a map keyed by ID, so only one
	// resource of each ID is recognized. Redundant resources are not legit, but have been seen in
	// the field. In particular, one case has been seen of a duplicate IIM block with one empty.
	// In general we keep the first seen copy to be compatible with Photoshop. A later non-empty
	// copy will be taken though if the current one is empty.
	
	// ! Don't use map[id] to lookup, that creates a default entry if none exists!
	
	XMP_Uns8* psirPtr   = this->psirContent;
	XMP_Uns8* psirEnd   = psirPtr + length;
	XMP_Uns8* psirLimit = psirEnd - kMinImgRsrcSize;
	
	while ( psirPtr <= psirLimit ) {
	
		XMP_Uns32 type = GetUns32BE(psirPtr);
		XMP_Uns16 id = GetUns16BE(psirPtr+4);
		psirPtr += 6;	// Advance to the resource name.

		XMP_Uns16 nameLen = psirPtr[0];			// ! The length for the Pascal string, w/ room for "+2".
		psirPtr += ((nameLen + 2) & 0xFFFE);	// ! Round up to an even offset. Yes, +2!
		
		if ( psirPtr > psirEnd-4 ) break;	// Bad image resource. Throw instead?

		XMP_Uns32 dataLen = GetUns32BE(psirPtr);
		psirPtr += 4;	// Advance to the resource data.
		XMP_Uns32 psirOffset = (XMP_Uns32) (psirPtr - this->psirContent);
		
		if ( (dataLen > length) || (psirPtr > psirEnd-dataLen) ) break;	// Bad image resource. Throw instead?
		
		if ( type == k8BIM ) {	// For read-only usage we ignore everything other than '8BIM' resources.
			ImgRsrcInfo newInfo ( id, dataLen, psirPtr, psirOffset );
			ImgRsrcMap::iterator rsrcPos = this->imgRsrcs.find ( id );
			if ( rsrcPos == this->imgRsrcs.end() ) {
				this->imgRsrcs.insert ( rsrcPos, ImgRsrcMap::value_type ( id, newInfo ) );
			} else if ( (rsrcPos->second.dataLen == 0) && (newInfo.dataLen != 0) ) {
				rsrcPos->second = newInfo;
			}
		}
		
		psirPtr += ((dataLen + 1) & 0xFFFFFFFEUL);	// ! Round up to an even offset.
	
	}

}	// PSIR_MemoryReader::ParseMemoryResources
