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

#include "XMPFiles/source/FormatSupport/TIFF_Support.hpp"
#include "source/XIO.hpp"
#include "source/EndianUtils.hpp"

// =================================================================================================
/// \file TIFF_MemoryReader.cpp
/// \brief Implementation of the memory-based read-only TIFF_Manager.
///
/// The read-only forms of TIFF_Manager are derived from TIFF_Reader. The GetTag methods are common
/// implementations in TIFF_Reader. The parsing code is different in the TIFF_MemoryReader and
/// TIFF_FileReader constructors. There are also separate destructors to release captured info.
///
/// The read-only implementations use runtime data that is simple tweaks on the stored form. The
/// memory-based reader has one block of data for the whole TIFF stream. The file-based reader has
/// one for each IFD, plus one for the collected non-local data for each IFD. Otherwise the logic
/// is the same in both cases.
///
/// The count for each IFD is extracted and a pointer set to the first entry in each IFD (serving as
/// a normal C array pointer). The IFD entries are tweaked as follows:
///
/// \li The id and type fields are converted to native values.
/// \li The count field is converted to a native byte count.
/// \li If the data is not inline the offset is converted to a pointer.
///
/// The tag values, whether inline or not, are not converted to native values. The values returned
/// from the GetTag methods are converted on the fly. The id, type, and count fields are easier to
/// convert because their types are fixed. They are used more, and more valuable to convert.
// =================================================================================================

// =================================================================================================
// TIFF_MemoryReader::SortIFD
// ==========================
//
// Does a fairly simple minded insertion-like sort. This sort is not going to be optimal for edge
// cases like and IFD with lots of duplicates.

// *** Might be better done using read and write pointers and two loops. The first loop moves out
// *** of order tags by a modified bubble sort, shifting the middle down one at a time in the loop.
// *** The first loop stops when a duplicate is hit. The second loop continues by moving the tail
// *** entries up to the appropriate slot.

void TIFF_MemoryReader::SortIFD ( TweakedIFDInfo* thisIFD )
{

	XMP_Uns16 tagCount = thisIFD->count;
	TweakedIFDEntry* ifdEntries = thisIFD->entries;
	XMP_Uns16 prevTag = GetUns16AsIs ( &ifdEntries[0].id );

	for ( size_t i = 1; i < tagCount; ++i ) {

		XMP_Uns16 thisTag = GetUns16AsIs ( &ifdEntries[i].id );

		if ( thisTag > prevTag ) {

			// In proper order.
			prevTag = thisTag;

		} else if ( thisTag == prevTag ) {

			// Duplicate tag, keep the 2nd copy, move the tail of the array up, prevTag is unchanged.
			memcpy ( &ifdEntries[i-1], &ifdEntries[i], 12*(tagCount-i) );	// AUDIT: Safe, moving tail forward, i >= 1.
			--tagCount;
			--i; // ! Don't move forward in the array, we've moved the unseen part up.

		} else if ( thisTag < prevTag ) {

			// Out of order, move this tag up, prevTag is unchanged. Might still be a duplicate!
			XMP_Int32 j;	// ! Need a signed value.
			for ( j = (XMP_Int32)i-1; j >= 0; --j ) {
				if ( GetUns16AsIs(&ifdEntries[j].id) <= thisTag ) break;
			}

			if ( (j >= 0) && (ifdEntries[j].id == thisTag) ) {

				// Out of order duplicate, move it to position j, move the tail of the array up.
				ifdEntries[j] = ifdEntries[i];
				memcpy ( &ifdEntries[i], &ifdEntries[i+1], 12*(tagCount-(i+1)) );	// AUDIT: Safe, moving tail forward, i >= 1.
				--tagCount;
				--i; // ! Don't move forward in the array, we've moved the unseen part up.

			} else {

				// Move the out of order entry to position j+1, move the middle of the array down.
				#if ! SUNOS_SPARC
					TweakedIFDEntry temp = ifdEntries[i];
					++j;	// ! So the insertion index becomes j.
					memcpy ( &ifdEntries[j+1], &ifdEntries[j], 12*(i-j) );	// AUDIT: Safe, moving less than i entries to a location before i.
					ifdEntries[j] = temp;
				#else
					void * tempifdEntries = &ifdEntries[i];
					TweakedIFDEntry temp;
					memcpy ( &temp, tempifdEntries, sizeof(TweakedIFDEntry) );
					++j;	// ! So the insertion index becomes j.
					memcpy ( &ifdEntries[j+1], &ifdEntries[j], 12*(i-j) );	// AUDIT: Safe, moving less than i entries to a location before i.
					tempifdEntries = &ifdEntries[j];
					memcpy ( tempifdEntries, &temp, sizeof(TweakedIFDEntry) );
				#endif

			}

		}

	}

	thisIFD->count = tagCount;	// Save the final count.

}	// TIFF_MemoryReader::SortIFD

// =================================================================================================
// TIFF_MemoryReader::GetIFD
// =========================

bool TIFF_MemoryReader::GetIFD ( XMP_Uns8 ifd, TagInfoMap* ifdMap ) const
{
	if ( ifd > kTIFF_LastRealIFD ) XMP_Throw ( "Invalid IFD requested", kXMPErr_InternalFailure );
	const TweakedIFDInfo* thisIFD = &containedIFDs[ifd];

	if ( ifdMap != 0 ) ifdMap->clear();
	if ( thisIFD->count == 0 ) return false;

	if ( ifdMap != 0 ) {

		for ( size_t i = 0; i < thisIFD->count; ++i ) {

			TweakedIFDEntry* thisTag = &(thisIFD->entries[i]);
			if ( (thisTag->type < kTIFF_ByteType) || (thisTag->type > kTIFF_LastType) ) continue;	// Bad type, skip this tag.

			TagInfo info ( thisTag->id, thisTag->type, 0, 0, GetUns32AsIs(&thisTag->bytes)  );
			info.count = info.dataLen / (XMP_Uns32)kTIFF_TypeSizes[info.type];
			info.dataPtr = this->GetDataPtr ( thisTag );

			(*ifdMap)[info.id] = info;

		}

	}

	return true;

}	// TIFF_MemoryReader::GetIFD

// =================================================================================================
// TIFF_MemoryReader::FindTagInIFD
// ===============================

const TIFF_MemoryReader::TweakedIFDEntry* TIFF_MemoryReader::FindTagInIFD ( XMP_Uns8 ifd, XMP_Uns16 id ) const
{
	if ( ifd == kTIFF_KnownIFD ) {
		// ... lookup the tag in the known tag map
	}

	if ( ifd > kTIFF_LastRealIFD ) XMP_Throw ( "Invalid IFD requested", kXMPErr_InternalFailure );
	const TweakedIFDInfo* thisIFD = &containedIFDs[ifd];

	if ( thisIFD->count == 0 ) return 0;

	XMP_Uns32 spanLength = thisIFD->count;
	const TweakedIFDEntry* spanBegin = &(thisIFD->entries[0]);

	while ( spanLength > 1 ) {

		XMP_Uns32 halfLength = spanLength >> 1;	// Since spanLength > 1, halfLength > 0.
		const TweakedIFDEntry* spanMiddle = spanBegin + halfLength;

		// There are halfLength entries below spanMiddle, then the spanMiddle entry, then
		// spanLength-halfLength-1 entries above spanMiddle (which can be none).

		XMP_Uns16 middleID = GetUns16AsIs ( &spanMiddle->id );
		if ( middleID == id ) {
			spanBegin = spanMiddle;
			break;
		} else if ( middleID > id ) {
			spanLength = halfLength;	// Discard the middle.
		} else {
			spanBegin = spanMiddle;	// Keep a valid spanBegin for the return check, don't use spanMiddle+1.
			spanLength -= halfLength;
		}

	}

	if ( GetUns16AsIs(&spanBegin->id) != id ) spanBegin = 0;
	return spanBegin;

}	// TIFF_MemoryReader::FindTagInIFD

// =================================================================================================
// TIFF_MemoryReader::GetValueOffset
// =================================

XMP_Uns32 TIFF_MemoryReader::GetValueOffset ( XMP_Uns8 ifd, XMP_Uns16 id ) const
{
	const TweakedIFDEntry* thisTag = this->FindTagInIFD ( ifd, id );
	if ( thisTag == 0 ) return 0;

	XMP_Uns8 * valuePtr = (XMP_Uns8*) this->GetDataPtr ( thisTag );

	return (XMP_Uns32)(valuePtr - this->tiffStream);	// ! TIFF streams can't exceed 4GB.

}	// TIFF_MemoryReader::GetValueOffset

// =================================================================================================
// TIFF_MemoryReader::GetTag
// =========================

bool TIFF_MemoryReader::GetTag ( XMP_Uns8 ifd, XMP_Uns16 id, TagInfo* info ) const
{
	const TweakedIFDEntry* thisTag = this->FindTagInIFD ( ifd, id );
	if ( thisTag == 0 ) return false;
	
	XMP_Uns16 thisType = GetUns16AsIs ( &thisTag->type );
	XMP_Uns32 thisBytes = GetUns32AsIs ( &thisTag->bytes );
	
	if ( (thisType < kTIFF_ByteType) || (thisType > kTIFF_LastType) ) return false;	// Bad type, skip this tag.

	if ( info != 0 ) {

		info->id = GetUns16AsIs ( &thisTag->id );
		info->type = thisType;
		info->count = thisBytes / (XMP_Uns32)kTIFF_TypeSizes[thisType];
		info->dataLen = thisBytes;

		info->dataPtr = this->GetDataPtr ( thisTag );

	}

	return true;

}	// TIFF_MemoryReader::GetTag

// =================================================================================================

static inline XMP_Uns8 GetUns8 ( const void* dataPtr )
{
	return *((XMP_Uns8*)dataPtr);
}

// =================================================================================================
// TIFF_MemoryReader::GetTag_Integer
// =================================
//
// Tolerate any size or sign.

bool TIFF_MemoryReader::GetTag_Integer ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns32* data ) const
{
	const TweakedIFDEntry* thisTag = this->FindTagInIFD ( ifd, id );
	if ( thisTag == 0 ) return false;

	if ( thisTag->type > kTIFF_LastType ) return false;	// Unknown type.
	if ( GetUns32AsIs(&thisTag->bytes) != kTIFF_TypeSizes[thisTag->type] ) return false;	// Wrong count.

	XMP_Uns32 uns32;
	XMP_Int32 int32;

	switch ( thisTag->type ) {
	
		case kTIFF_ByteType:
			uns32 = GetUns8 ( this->GetDataPtr ( thisTag ) );
			break;
	
		case kTIFF_ShortType:
			uns32 = this->GetUns16 ( this->GetDataPtr ( thisTag ) );
			break;
	
		case kTIFF_LongType:
			uns32 = this->GetUns32 ( this->GetDataPtr ( thisTag ) );
			break;
	
		case kTIFF_SByteType:
			int32 = (XMP_Int8) GetUns8 ( this->GetDataPtr ( thisTag ) );
			uns32 = (XMP_Uns32) int32;	// Make sure sign bits are properly set.
			break;
	
		case kTIFF_SShortType:
			int32 = (XMP_Int16) this->GetUns16 ( this->GetDataPtr ( thisTag ) );
			uns32 = (XMP_Uns32) int32;	// Make sure sign bits are properly set.
			break;
	
		case kTIFF_SLongType:
			int32 = (XMP_Int32) this->GetUns32 ( this->GetDataPtr ( thisTag ) );
			uns32 = (XMP_Uns32) int32;	// Make sure sign bits are properly set.
			break;
	
		default: return false;

	}

	if ( data != 0 ) *data = uns32;
	return true;

}	// TIFF_MemoryReader::GetTag_Integer

// =================================================================================================
// TIFF_MemoryReader::GetTag_Byte
// ==============================

bool TIFF_MemoryReader::GetTag_Byte ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns8* data ) const
{
	const TweakedIFDEntry* thisTag = this->FindTagInIFD ( ifd, id );
	if ( thisTag == 0 ) return false;
	if ( (thisTag->type != kTIFF_ByteType) || (thisTag->bytes != 1) ) return false;

	if ( data != 0 ) {
		*data = * ( (XMP_Uns8*) this->GetDataPtr ( thisTag ) );
	}

	return true;

}	// TIFF_MemoryReader::GetTag_Byte

// =================================================================================================
// TIFF_MemoryReader::GetTag_SByte
// ===============================

bool TIFF_MemoryReader::GetTag_SByte ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Int8* data ) const
{
	const TweakedIFDEntry* thisTag = this->FindTagInIFD ( ifd, id );
	if ( thisTag == 0 ) return false;
	if ( (thisTag->type != kTIFF_SByteType) || (thisTag->bytes != 1) ) return false;

	if ( data != 0 ) {
		*data = * ( (XMP_Int8*) this->GetDataPtr ( thisTag ) );
	}

	return true;

}	// TIFF_MemoryReader::GetTag_SByte

// =================================================================================================
// TIFF_MemoryReader::GetTag_Short
// ===============================

bool TIFF_MemoryReader::GetTag_Short ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns16* data ) const
{
	const TweakedIFDEntry* thisTag = this->FindTagInIFD ( ifd, id );
	if ( thisTag == 0 ) return false;
	if ( (thisTag->type != kTIFF_ShortType) || (thisTag->bytes != 2) ) return false;

	if ( data != 0 ) {
		*data = this->GetUns16 ( this->GetDataPtr ( thisTag ) );
	}

	return true;

}	// TIFF_MemoryReader::GetTag_Short

// =================================================================================================
// TIFF_MemoryReader::GetTag_SShort
// ================================

bool TIFF_MemoryReader::GetTag_SShort ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Int16* data ) const
{
	const TweakedIFDEntry* thisTag = this->FindTagInIFD ( ifd, id );
	if ( thisTag == 0 ) return false;
	if ( (thisTag->type != kTIFF_SShortType) || (thisTag->bytes != 2) ) return false;

	if ( data != 0 ) {
		*data = (XMP_Int16) this->GetUns16 ( this->GetDataPtr ( thisTag ) );
	}

	return true;

}	// TIFF_MemoryReader::GetTag_SShort

// =================================================================================================
// TIFF_MemoryReader::GetTag_Long
// ==============================

bool TIFF_MemoryReader::GetTag_Long ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns32* data ) const
{
	const TweakedIFDEntry* thisTag = this->FindTagInIFD ( ifd, id );
	if ( thisTag == 0 ) return false;
	if ( (thisTag->type != kTIFF_LongType) || (thisTag->bytes != 4) ) return false;

	if ( data != 0 ) {
		*data = this->GetUns32 ( this->GetDataPtr ( thisTag ) );
	}

	return true;

}	// TIFF_MemoryReader::GetTag_Long

// =================================================================================================
// TIFF_MemoryReader::GetTag_SLong
// ===============================

bool TIFF_MemoryReader::GetTag_SLong ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Int32* data ) const
{
	const TweakedIFDEntry* thisTag = this->FindTagInIFD ( ifd, id );
	if ( thisTag == 0 ) return false;
	if ( (thisTag->type != kTIFF_SLongType) || (thisTag->bytes != 4) ) return false;

	if ( data != 0 ) {
		*data = (XMP_Int32) this->GetUns32 ( this->GetDataPtr ( thisTag ) );
	}

	return true;

}	// TIFF_MemoryReader::GetTag_SLong

// =================================================================================================
// TIFF_MemoryReader::GetTag_Rational
// ==================================

bool TIFF_MemoryReader::GetTag_Rational ( XMP_Uns8 ifd, XMP_Uns16 id, Rational* data ) const
{
	const TweakedIFDEntry* thisTag = this->FindTagInIFD ( ifd, id );
	if ( thisTag == 0 ) return false;
	if ( (thisTag->type != kTIFF_RationalType) || (thisTag->bytes != 8) ) return false;

	if ( data != 0 ) {
		XMP_Uns32* dataPtr = (XMP_Uns32*) this->GetDataPtr ( thisTag );
		data->num = this->GetUns32 ( dataPtr );
		data->denom = this->GetUns32 ( dataPtr+1 );
	}

	return true;

}	// TIFF_MemoryReader::GetTag_Rational

// =================================================================================================
// TIFF_MemoryReader::GetTag_SRational
// ===================================

bool TIFF_MemoryReader::GetTag_SRational ( XMP_Uns8 ifd, XMP_Uns16 id, SRational* data ) const
{
	const TweakedIFDEntry* thisTag = this->FindTagInIFD ( ifd, id );
	if ( thisTag == 0 ) return false;
	if ( (thisTag->type != kTIFF_SRationalType) || (thisTag->bytes != 8) ) return false;

	if ( data != 0 ) {
		XMP_Uns32* dataPtr = (XMP_Uns32*) this->GetDataPtr ( thisTag );
		data->num = (XMP_Int32) this->GetUns32 ( dataPtr );
		data->denom = (XMP_Int32) this->GetUns32 ( dataPtr+1 );
	}

	return true;

}	// TIFF_MemoryReader::GetTag_SRational

// =================================================================================================
// TIFF_MemoryReader::GetTag_Float
// ===============================

bool TIFF_MemoryReader::GetTag_Float ( XMP_Uns8 ifd, XMP_Uns16 id, float* data ) const
{
	const TweakedIFDEntry* thisTag = this->FindTagInIFD ( ifd, id );
	if ( thisTag == 0 ) return false;
	if ( (thisTag->type != kTIFF_FloatType) || (thisTag->bytes != 4) ) return false;

	if ( data != 0 ) {
		*data = this->GetFloat ( this->GetDataPtr ( thisTag ) );
	}

	return true;

}	// TIFF_MemoryReader::GetTag_Float

// =================================================================================================
// TIFF_MemoryReader::GetTag_Double
// ================================

bool TIFF_MemoryReader::GetTag_Double ( XMP_Uns8 ifd, XMP_Uns16 id, double* data ) const
{
	const TweakedIFDEntry* thisTag = this->FindTagInIFD ( ifd, id );
	if ( thisTag == 0 ) return false;
	if ( (thisTag->type != kTIFF_DoubleType) || (thisTag->bytes != 8) ) return false;

	if ( data != 0 ) {
		double* dataPtr = (double*) this->GetDataPtr ( thisTag );
		*data = this->GetDouble ( dataPtr );
	}

	return true;

}	// TIFF_MemoryReader::GetTag_Double

// =================================================================================================
// TIFF_MemoryReader::GetTag_ASCII
// ===============================

bool TIFF_MemoryReader::GetTag_ASCII ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_StringPtr* dataPtr, XMP_StringLen* dataLen ) const
{
	const TweakedIFDEntry* thisTag = this->FindTagInIFD ( ifd, id );
	if ( thisTag == 0 ) return false;
	if ( thisTag->type != kTIFF_ASCIIType ) return false;

	if ( dataPtr != 0 ) {
		*dataPtr = (XMP_StringPtr) this->GetDataPtr ( thisTag );
	}

	if ( dataLen != 0 ) *dataLen = thisTag->bytes;

	return true;

}	// TIFF_MemoryReader::GetTag_ASCII

// =================================================================================================
// TIFF_MemoryReader::GetTag_EncodedString
// =======================================

bool TIFF_MemoryReader::GetTag_EncodedString ( XMP_Uns8 ifd, XMP_Uns16 id, std::string* utf8Str ) const
{
	const TweakedIFDEntry* thisTag = this->FindTagInIFD ( ifd, id );
	if ( thisTag == 0 ) return false;
	if ( thisTag->type != kTIFF_UndefinedType ) return false;

	if ( utf8Str == 0 ) return true;	// Return true if the converted string is not wanted.

	bool ok = this->DecodeString ( this->GetDataPtr ( thisTag ), thisTag->bytes, utf8Str );
	return ok;

}	// TIFF_MemoryReader::GetTag_EncodedString

// =================================================================================================
// TIFF_MemoryReader::ParseMemoryStream
// ====================================

// *** Need to tell TIFF/Exif from TIFF/EP, DNG files are the latter.

void TIFF_MemoryReader::ParseMemoryStream ( const void* data, XMP_Uns32 length, bool copyData /* = true */ )
{
	// Get rid of any current TIFF.

	if ( this->ownedStream ) free ( this->tiffStream );
	this->ownedStream = false;
	this->tiffStream  = 0;
	this->tiffLength  = 0;

	for ( size_t i = 0; i < kTIFF_KnownIFDCount; ++i ) {
		this->containedIFDs[i].count = 0;
		this->containedIFDs[i].entries = 0;
	}

	if ( length == 0 ) return;

	// Allocate space for the full in-memory stream and copy it.

	if ( ! copyData ) {
		XMP_Assert ( ! this->ownedStream );
		this->tiffStream = (XMP_Uns8*) data;
	} else {
		if ( length > 100*1024*1024 ) XMP_Throw ( "Outrageous length for memory-based TIFF", kXMPErr_BadTIFF );
		this->tiffStream = (XMP_Uns8*) malloc(length);
		if ( this->tiffStream == 0 ) XMP_Throw ( "Out of memory", kXMPErr_NoMemory );
		memcpy ( this->tiffStream, data, length );	// AUDIT: Safe, malloc'ed length bytes above.
		this->ownedStream = true;
	}

	this->tiffLength = length;
	XMP_Uns32 ifdLimit = this->tiffLength - 6;	// An IFD must start before this offset.

	// Find and process the primary, Exif, GPS, and Interoperability IFDs.

	XMP_Uns32 primaryIFDOffset = this->CheckTIFFHeader ( this->tiffStream, length );
	XMP_Uns32 tnailIFDOffset   = 0;

	if ( primaryIFDOffset != 0 ) tnailIFDOffset = this->ProcessOneIFD ( primaryIFDOffset, kTIFF_PrimaryIFD );
	
	// ! Need the thumbnail IFD for checking full Exif APP1 in some JPEG files!
	if ( tnailIFDOffset != 0 ) {
		if ( IsOffsetValid(tnailIFDOffset, 8, ifdLimit ) ) { 	// Ignore a bad Thumbnail IFD offset.
			(void) this->ProcessOneIFD ( tnailIFDOffset, kTIFF_TNailIFD );
		} else {
			XMP_Error error ( kXMPErr_BadTIFF, "Bad IFD offset" );
			this->NotifyClient (kXMPErrSev_Recoverable, error );
		}
	}

	const TweakedIFDEntry* exifIFDTag = this->FindTagInIFD ( kTIFF_PrimaryIFD, kTIFF_ExifIFDPointer );
	if ( (exifIFDTag != 0) && (exifIFDTag->type == kTIFF_LongType) && (GetUns32AsIs(&exifIFDTag->bytes) == 4) ) {
		XMP_Uns32 exifOffset = this->GetUns32 ( &exifIFDTag->dataOrPos );
		(void) this->ProcessOneIFD ( exifOffset, kTIFF_ExifIFD );
	}

	const TweakedIFDEntry* gpsIFDTag = this->FindTagInIFD ( kTIFF_PrimaryIFD, kTIFF_GPSInfoIFDPointer );
	if ( (gpsIFDTag != 0) && (gpsIFDTag->type == kTIFF_LongType) && (GetUns32AsIs(&gpsIFDTag->bytes) == 4) ) {
		XMP_Uns32 gpsOffset = this->GetUns32 ( &gpsIFDTag->dataOrPos );
		if ( IsOffsetValid ( gpsOffset, 8, ifdLimit ) ) {	// Ignore a bad GPS IFD offset.
			(void) this->ProcessOneIFD ( gpsOffset, kTIFF_GPSInfoIFD );
		} else {
			XMP_Error error ( kXMPErr_BadTIFF, "Bad IFD offset" );
			this->NotifyClient (kXMPErrSev_Recoverable, error );
		}
	}

	const TweakedIFDEntry* interopIFDTag = this->FindTagInIFD ( kTIFF_ExifIFD, kTIFF_InteroperabilityIFDPointer );
	if ( (interopIFDTag != 0) && (interopIFDTag->type == kTIFF_LongType) && (GetUns32AsIs(&interopIFDTag->bytes) == 4) ) {
		XMP_Uns32 interopOffset = this->GetUns32 ( &interopIFDTag->dataOrPos );
		if ( IsOffsetValid ( interopOffset, 8, ifdLimit ) ) {	// Ignore a bad Interoperability IFD offset.
			(void) this->ProcessOneIFD ( interopOffset, kTIFF_InteropIFD );
		} else {
			XMP_Error error ( kXMPErr_BadTIFF, "Bad IFD offset" );
			this->NotifyClient (kXMPErrSev_Recoverable, error );
		}
	}

}	// TIFF_MemoryReader::ParseMemoryStream

// =================================================================================================
// TIFF_MemoryReader::ProcessOneIFD
// ================================

XMP_Uns32 TIFF_MemoryReader::ProcessOneIFD ( XMP_Uns32 ifdOffset, XMP_Uns8 ifd )
{
	TweakedIFDInfo& ifdInfo = this->containedIFDs[ifd];

	if ( (ifdOffset < 8) || (ifdOffset > (this->tiffLength - kEmptyIFDLength)) ) {
		XMP_Error error(kXMPErr_BadTIFF, "Bad IFD offset" );
		this->NotifyClient ( kXMPErrSev_FileFatal, error );
	}

	XMP_Uns8* ifdPtr = this->tiffStream + ifdOffset;
	XMP_Uns16 ifdCount = this->GetUns16 ( ifdPtr );
	TweakedIFDEntry* ifdEntries = (TweakedIFDEntry*)(ifdPtr+2);

	if ( ifdCount >= 0x8000 ) {
		XMP_Error error(kXMPErr_BadTIFF, "Outrageous IFD count" );
		this->NotifyClient ( kXMPErrSev_FileFatal, error );
	}

	if ( (XMP_Uns32)(2 + ifdCount*12 + 4) > (this->tiffLength - ifdOffset) ) {
		XMP_Error error(kXMPErr_BadTIFF, "Out of bounds IFD" );
		this->NotifyClient ( kXMPErrSev_FileFatal, error );
	}

	ifdInfo.count = ifdCount;
	ifdInfo.entries = ifdEntries;

	XMP_Int32 prevTag = -1;	// ! The GPS IFD has a tag 0, so we need a signed initial value.
	bool needsSorting = false;
	for ( size_t i = 0; i < ifdCount; ++i ) {

		TweakedIFDEntry* thisEntry = &ifdEntries[i];	// Tweak the IFD entry to be more useful.

		if ( ! this->nativeEndian ) {
			Flip2 ( &thisEntry->id );
			Flip2 ( &thisEntry->type );
			Flip4 ( &thisEntry->bytes );
		}

		if ( GetUns16AsIs(&thisEntry->id) <= prevTag ) needsSorting = true;
		prevTag = GetUns16AsIs ( &thisEntry->id );

		if ( (GetUns16AsIs(&thisEntry->type) < kTIFF_ByteType) || (GetUns16AsIs(&thisEntry->type) > kTIFF_LastType) ) continue;	// Bad type, skip this tag.

		#if ! SUNOS_SPARC
	
			thisEntry->bytes *= (XMP_Uns32)kTIFF_TypeSizes[thisEntry->type];
			if ( thisEntry->bytes > 4 ) {
				if ( ! this->nativeEndian ) Flip4 ( &thisEntry->dataOrPos );
				if ( (thisEntry->dataOrPos < 8) || (thisEntry->dataOrPos >= this->tiffLength) ) {
					thisEntry->bytes = thisEntry->dataOrPos = 0;	// Make this bad tag look empty.
				}
				if ( thisEntry->bytes > (this->tiffLength - thisEntry->dataOrPos) ) {
					thisEntry->bytes = thisEntry->dataOrPos = 0;	// Make this bad tag look empty.
				}
			}
	
		#else
	
			void *tempEntryByte = &thisEntry->bytes;
			XMP_Uns32 temp = GetUns32AsIs(&thisEntry->bytes);
			temp = temp * (XMP_Uns32)kTIFF_TypeSizes[GetUns16AsIs(&thisEntry->type)];
			memcpy ( tempEntryByte, &temp, sizeof(thisEntry->bytes) );
	
			// thisEntry->bytes *= (XMP_Uns32)kTIFF_TypeSizes[thisEntry->type];
			if ( GetUns32AsIs(&thisEntry->bytes) > 4 ) {
				void *tempEntryDataOrPos = &thisEntry->dataOrPos;
				if ( ! this->nativeEndian ) Flip4 ( &thisEntry->dataOrPos );
				if ( (GetUns32AsIs(&thisEntry->dataOrPos) < 8) || (GetUns32AsIs(&thisEntry->dataOrPos) >= this->tiffLength) ) {
					// thisEntry->bytes = thisEntry->dataOrPos = 0;	// Make this bad tag look empty.
					memset ( tempEntryByte, 0, sizeof(XMP_Uns32) );
					memset ( tempEntryDataOrPos, 0, sizeof(XMP_Uns32) );
				}
				if ( GetUns32AsIs(&thisEntry->bytes) > (this->tiffLength - GetUns32AsIs(&thisEntry->dataOrPos)) ) {
					// thisEntry->bytes = thisEntry->dataOrPos = 0;	// Make this bad tag look empty.
					memset ( tempEntryByte, 0, sizeof(XMP_Uns32) );
					memset ( tempEntryDataOrPos, 0, sizeof(XMP_Uns32) );
				}
			}
	
		#endif

	}

	ifdPtr += (2 + ifdCount*12);
	XMP_Uns32 nextIFDOffset = this->GetUns32 ( ifdPtr );

	if ( needsSorting ) SortIFD ( &ifdInfo );	// ! Don't perturb the ifdCount used to find the next IFD offset.

	return nextIFDOffset;

}	// TIFF_MemoryReader::ProcessOneIFD

// =================================================================================================
