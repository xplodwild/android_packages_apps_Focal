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

#include "XMPFiles/source/FormatSupport/ISOBaseMedia_Support.hpp"
#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "source/XIO.hpp"

// =================================================================================================
/// \file ISOBaseMedia_Support.cpp
/// \brief Manager for parsing and serializing ISO Base Media files ( MPEG-4 and JPEG-2000).
///
// =================================================================================================

namespace ISOMedia {

static BoxInfo voidInfo;

// =================================================================================================
// GetBoxInfo - from memory
// ========================

const XMP_Uns8 * GetBoxInfo ( const XMP_Uns8 * boxPtr, const XMP_Uns8 * boxLimit,
							  BoxInfo * info, bool throwErrors /* = false */ )
{
	XMP_Uns32 u32Size;
	
	if ( info == 0 ) info = &voidInfo;
	info->boxType = info->headerSize = 0;
	info->contentSize = 0;
	
	if ( boxPtr >= boxLimit ) XMP_Throw ( "Bad offset to GetBoxInfo", kXMPErr_InternalFailure );
	
	if ( (boxLimit - boxPtr) < 8 ) {	// Is there enough space for a standard box header?
		if ( throwErrors ) XMP_Throw ( "No space for ISO box header", kXMPErr_BadFileFormat );
		info->headerSize = (XMP_Uns32) (boxLimit - boxPtr);
		return boxLimit;
	}
	
	u32Size = GetUns32BE ( boxPtr );
	info->boxType = GetUns32BE ( boxPtr+4 );

	if ( u32Size >= 8 ) {
		info->headerSize  = 8;	// Normal explicit size case.
		info->contentSize = u32Size - 8;
	} else if ( u32Size == 0 ) {
		info->headerSize  = 8;	// The box goes to EoF - treat it as "to limit".
		info->contentSize = (boxLimit - boxPtr) - 8;
	} else if ( u32Size != 1 ) {
		if ( throwErrors ) XMP_Throw ( "Bad ISO box size, 2..7", kXMPErr_BadFileFormat );
		info->headerSize  = 8;	// Bad total size in the range of 2..7, treat as 8.
		info->contentSize = 0;
	} else {
		if ( (boxLimit - boxPtr) < 16 ) {	// Is there enough space for an extended box header?
			if ( throwErrors ) XMP_Throw ( "No space for ISO extended header", kXMPErr_BadFileFormat );
			info->headerSize = (XMP_Uns32) (boxLimit - boxPtr);
			return boxLimit;
		}
		XMP_Uns64 u64Size = GetUns64BE ( boxPtr+8 );
		if ( u64Size < 16 ) {
			if ( throwErrors ) XMP_Throw ( "Bad ISO extended box size, < 16", kXMPErr_BadFileFormat );
			u64Size = 16;	// Treat bad total size as 16.
		}
		info->headerSize  = 16;
		info->contentSize = u64Size - 16;
	}
	
	XMP_Assert ( (XMP_Uns64)(boxLimit - boxPtr) >= (XMP_Uns64)info->headerSize );
	if ( info->contentSize > (XMP_Uns64)((boxLimit - boxPtr) - info->headerSize) ) {
		if ( throwErrors ) XMP_Throw ( "Bad ISO box content size", kXMPErr_BadFileFormat );
		info->contentSize = ((boxLimit - boxPtr) - info->headerSize);	// Trim a bad content size to the limit.
	}
	
	return (boxPtr + info->headerSize + info->contentSize);

}	// GetBoxInfo

// =================================================================================================
// GetBoxInfo - from a file
// ========================

XMP_Uns64 GetBoxInfo ( XMP_IO* fileRef, const XMP_Uns64 boxOffset, const XMP_Uns64 boxLimit,
					   BoxInfo * info, bool doSeek /* = true */, bool throwErrors /* = false */ )
{
	XMP_Uns8  buffer [8];
	XMP_Uns32 u32Size;
	
	if ( info == 0 ) info = &voidInfo;
	info->boxType = info->headerSize = 0;
	info->contentSize = 0;
	
	if ( boxOffset >= boxLimit ) XMP_Throw ( "Bad offset to GetBoxInfo", kXMPErr_InternalFailure );
	
	if ( (boxLimit - boxOffset) < 8 ) {	// Is there enough space for a standard box header?
		if ( throwErrors ) XMP_Throw ( "No space for ISO box header", kXMPErr_BadFileFormat );
		info->headerSize = (XMP_Uns32) (boxLimit - boxOffset);
		return boxLimit;
	}
	
	if ( doSeek ) fileRef->Seek ( boxOffset, kXMP_SeekFromStart );
	(void) fileRef->ReadAll ( buffer, 8 );

	u32Size = GetUns32BE ( &buffer[0] );
	info->boxType = GetUns32BE ( &buffer[4] );

	if ( u32Size >= 8 ) {
		info->headerSize  = 8;	// Normal explicit size case.
		info->contentSize = u32Size - 8;
	} else if ( u32Size == 0 ) {
		info->headerSize  = 8;	// The box goes to EoF.
		info->contentSize = fileRef->Length() - (boxOffset + 8);
	} else if ( u32Size != 1 ) {
		if ( throwErrors ) XMP_Throw ( "Bad ISO box size, 2..7", kXMPErr_BadFileFormat );
		info->headerSize  = 8;	// Bad total size in the range of 2..7, treat as 8.
		info->contentSize = 0;
	} else {
		if ( (boxLimit - boxOffset) < 16 ) {	// Is there enough space for an extended box header?
			if ( throwErrors ) XMP_Throw ( "No space for ISO extended header", kXMPErr_BadFileFormat );
			info->headerSize = (XMP_Uns32) (boxLimit - boxOffset);
			return boxLimit;
		}
		(void) fileRef->ReadAll ( buffer, 8 );
		XMP_Uns64 u64Size = GetUns64BE ( &buffer[0] );
		if ( u64Size < 16 ) {
			if ( throwErrors ) XMP_Throw ( "Bad ISO extended box size, < 16", kXMPErr_BadFileFormat );
			u64Size = 16;	// Treat bad total size as 16.
		}
		info->headerSize  = 16;
		info->contentSize = u64Size - 16;
	}
	
	XMP_Assert ( (boxLimit - boxOffset) >= info->headerSize );
	if ( info->contentSize > (boxLimit - boxOffset - info->headerSize) ) {
		if ( throwErrors ) XMP_Throw ( "Bad ISO box content size", kXMPErr_BadFileFormat );
		info->contentSize = (boxLimit - boxOffset - info->headerSize);	// Trim a bad content size to the limit.
	}
	
	return (boxOffset + info->headerSize + info->contentSize);

}	// GetBoxInfo

}	// namespace ISO_Media


// =================================================================================================
