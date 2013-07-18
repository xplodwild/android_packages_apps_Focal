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
/// \file TIFF_FileWriter.cpp
/// \brief TIFF_FileWriter is used for memory-based read-write access and all file-based access.
///
/// \c TIFF_FileWriter is used for memory-based read-write access and all file-based access. The
/// main internal data structure is the InternalTagMap, a std::map that uses the tag number as the
/// key and InternalTagInfo as the value. There are 5 of these maps, one for each of the recognized
/// IFDs. The maps contain an entry for each tag in the IFD, whether we capture the data or not. The
/// dataPtr and dataLen fields in the InternalTagInfo are zero if the tag is not captured.
// =================================================================================================

// =================================================================================================
// TIFF_FileWriter::TIFF_FileWriter
// ================================
//
// Set big endian Get/Put routines so that routines are in place for creating TIFF without a parse.
// Parsing will reset them to the proper endianness for the stream. Big endian is a good default
// since JPEG and PSD files are big endian overall.

TIFF_FileWriter::TIFF_FileWriter() : changed(false), legacyDeleted(false), memParsed(false),
									 fileParsed(false), ownedStream(false), memStream(0), tiffLength(0)
{

	XMP_Uns8 bogusTIFF [kEmptyTIFFLength];

	bogusTIFF[0] = 0x4D;
	bogusTIFF[1] = 0x4D;
	bogusTIFF[2] = 0x00;
	bogusTIFF[3] = 0x2A;
	bogusTIFF[4] = bogusTIFF[5] = bogusTIFF[6] = bogusTIFF[7] = 0x00;

	(void) this->CheckTIFFHeader ( bogusTIFF, sizeof ( bogusTIFF ) );

}	// TIFF_FileWriter::TIFF_FileWriter

// =================================================================================================
// TIFF_FileWriter::~TIFF_FileWriter
// =================================

TIFF_FileWriter::~TIFF_FileWriter()
{
	XMP_Assert ( ! (this->memParsed && this->fileParsed) );

	if ( this->ownedStream ) {
		XMP_Assert ( this->memStream != 0 );
		free ( this->memStream );
	}

}	// TIFF_FileWriter::~TIFF_FileWriter

// =================================================================================================
// TIFF_FileWriter::DeleteExistingInfo
// ===================================

void TIFF_FileWriter::DeleteExistingInfo()
{
	XMP_Assert ( ! (this->memParsed && this->fileParsed) );

	if ( this->ownedStream ) free ( this->memStream );	// ! Current TIFF might be memory-parsed.
	this->memStream = 0;
	this->tiffLength = 0;

	for ( int ifd = 0; ifd < kTIFF_KnownIFDCount; ++ifd ) this->containedIFDs[ifd].clear();

	this->changed = false;
	this->legacyDeleted = false;
	this->memParsed = false;
	this->fileParsed = false;
	this->ownedStream = false;

}	// TIFF_FileWriter::DeleteExistingInfo

// =================================================================================================
// TIFF_FileWriter::PickIFD
// ========================

XMP_Uns8 TIFF_FileWriter::PickIFD ( XMP_Uns8 ifd, XMP_Uns16 id )
{
	if ( ifd > kTIFF_LastRealIFD ) {
		if ( ifd != kTIFF_KnownIFD ) XMP_Throw ( "Invalid IFD number", kXMPErr_BadParam );
		XMP_Throw ( "kTIFF_KnownIFD not yet implemented", kXMPErr_Unimplemented );
		// *** Likely to stay unimplemented until there is a client need.
	}

	return ifd;

}	// TIFF_FileWriter::PickIFD

// =================================================================================================
// TIFF_FileWriter::FindTagInIFD
// =============================

const TIFF_FileWriter::InternalTagInfo* TIFF_FileWriter::FindTagInIFD ( XMP_Uns8 ifd, XMP_Uns16 id ) const
{
	ifd = PickIFD ( ifd, id );
	const InternalTagMap& currIFD = this->containedIFDs[ifd].tagMap;

	InternalTagMap::const_iterator tagPos = currIFD.find ( id );
	if ( tagPos == currIFD.end() ) return 0;
	return &tagPos->second;

}	// TIFF_FileWriter::FindTagInIFD

// =================================================================================================
// TIFF_FileWriter::GetIFD
// =======================

bool TIFF_FileWriter::GetIFD ( XMP_Uns8 ifd, TagInfoMap* ifdMap ) const
{
	if ( ifd > kTIFF_LastRealIFD ) XMP_Throw ( "Invalid IFD number", kXMPErr_BadParam );
	const InternalTagMap& currIFD = this->containedIFDs[ifd].tagMap;

	InternalTagMap::const_iterator tagPos = currIFD.begin();
	InternalTagMap::const_iterator tagEnd = currIFD.end();

	if ( ifdMap != 0 ) ifdMap->clear();
	if ( tagPos == tagEnd ) return false;	// Empty IFD.

	if ( ifdMap != 0 ) {
		for ( ; tagPos != tagEnd; ++tagPos ) {
			const InternalTagInfo& intInfo = tagPos->second;
			TagInfo extInfo ( intInfo.id, intInfo.type, intInfo.count, intInfo.dataPtr, intInfo.dataLen  );
			(*ifdMap)[intInfo.id] = extInfo;
		}
	}

	return true;

}	// TIFF_FileWriter::GetIFD

// =================================================================================================
// TIFF_FileWriter::GetValueOffset
// ===============================

XMP_Uns32 TIFF_FileWriter::GetValueOffset ( XMP_Uns8 ifd, XMP_Uns16 id ) const
{
	const InternalTagInfo* thisTag = this->FindTagInIFD ( ifd, id );
	if ( (thisTag == 0) || (thisTag->origDataLen == 0) ) return 0;

	return thisTag->origDataOffset;

}	// TIFF_FileWriter::GetValueOffset

// =================================================================================================
// TIFF_FileWriter::GetTag
// =======================

bool TIFF_FileWriter::GetTag ( XMP_Uns8 ifd, XMP_Uns16 id, TagInfo* info ) const
{
	const InternalTagInfo* thisTag = this->FindTagInIFD ( ifd, id );
	if ( thisTag == 0 ) return false;

	if ( info != 0 ) {

		info->id = thisTag->id;
		info->type = thisTag->type;
		info->count = thisTag->dataLen / (XMP_Uns32)kTIFF_TypeSizes[thisTag->type];
		info->dataLen = thisTag->dataLen;
		info->dataPtr = (const void*)(thisTag->dataPtr);

	}

	return true;

}	// TIFF_FileWriter::GetTag

// =================================================================================================
// TIFF_FileWriter::SetTag
// =======================

void TIFF_FileWriter::SetTag ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns16 type, XMP_Uns32 count, const void* clientPtr )
{
	if ( (type < kTIFF_ByteType) || (type > kTIFF_LastType) ) XMP_Throw ( "Invalid TIFF tag type", kXMPErr_BadParam );
	size_t typeSize = kTIFF_TypeSizes[type];
	size_t fullSize = count * typeSize;

	ifd = PickIFD ( ifd, id );
	InternalTagMap& currIFD = this->containedIFDs[ifd].tagMap;

	InternalTagInfo* tagPtr = 0;
	InternalTagMap::iterator tagPos = currIFD.find ( id );

	if ( tagPos == currIFD.end() ) {

		// The tag does not yet exist, add it.
		InternalTagMap::value_type mapValue ( id, InternalTagInfo ( id, type, count, this->fileParsed ) );
		tagPos = currIFD.insert ( tagPos, mapValue );
		tagPtr = &tagPos->second;

	} else {

		tagPtr = &tagPos->second;

		// The tag already exists, make sure the value is actually changing.
		if ( (type == tagPtr->type) && (count == tagPtr->count) &&
			 (memcmp ( clientPtr, tagPtr->dataPtr, tagPtr->dataLen ) == 0) ) {
			return;	// ! The value is unchanged, exit.
		}

		tagPtr->FreeData();	// Release any existing data allocation.

		tagPtr->type  = type;	// These might be changing also.
		tagPtr->count = count;

	}

	tagPtr->changed = true;
	tagPtr->dataLen = (XMP_Uns32)fullSize;

	if ( fullSize <= 4 ) {
		// The data is less than 4 bytes, store it in the smallValue field using native endianness.
		tagPtr->dataPtr = (XMP_Uns8*) &tagPtr->smallValue;
	} else {
		// The data is more than 4 bytes, make a copy.
		tagPtr->dataPtr = (XMP_Uns8*) malloc ( fullSize );
		if ( tagPtr->dataPtr == 0 ) XMP_Throw ( "Out of memory", kXMPErr_NoMemory );
	}
	memcpy ( tagPtr->dataPtr, clientPtr, fullSize );	// AUDIT: Safe, space guaranteed to be fullSize.

	if ( ! this->nativeEndian ) {
		if ( typeSize == 2 ) {
			XMP_Uns16* flipPtr = (XMP_Uns16*) tagPtr->dataPtr;
			for ( XMP_Uns32 i = 0; i < count; ++i ) Flip2 ( flipPtr[i] );
		} else if ( typeSize == 4 ) {
			XMP_Uns32* flipPtr = (XMP_Uns32*) tagPtr->dataPtr;
			for ( XMP_Uns32 i = 0; i < count; ++i ) Flip4 ( flipPtr[i] );
		} else if ( typeSize == 8 ) {
			XMP_Uns64* flipPtr = (XMP_Uns64*) tagPtr->dataPtr;
			for ( XMP_Uns32 i = 0; i < count; ++i ) Flip8 ( flipPtr[i] );
		}
	}

	this->containedIFDs[ifd].changed = true;
	this->changed = true;

}	// TIFF_FileWriter::SetTag

// =================================================================================================
// TIFF_FileWriter::DeleteTag
// ==========================

void TIFF_FileWriter::DeleteTag ( XMP_Uns8 ifd, XMP_Uns16 id )
{
	ifd = PickIFD ( ifd, id );
	InternalTagMap& currIFD = this->containedIFDs[ifd].tagMap;

	InternalTagMap::iterator tagPos = currIFD.find ( id );
	if ( tagPos == currIFD.end() ) return;	// ! Don't set the changed flags if the tag didn't exist.

	currIFD.erase ( tagPos );
	this->containedIFDs[ifd].changed = true;
	this->changed = true;
	if ( (ifd != kTIFF_PrimaryIFD) || (id != kTIFF_XMP) ) this->legacyDeleted = true;

}	// TIFF_FileWriter::DeleteTag

// =================================================================================================

static inline XMP_Uns8 GetUns8 ( const void* dataPtr )
{
	return *((XMP_Uns8*)dataPtr);
}

// =================================================================================================
// TIFF_FileWriter::GetTag_Integer
// ===============================

bool TIFF_FileWriter::GetTag_Integer ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns32* data ) const
{
	const InternalTagInfo* thisTag = this->FindTagInIFD ( ifd, id );
	if ( thisTag == 0 ) return false;
	if ( thisTag->count != 1 ) return false;

	XMP_Uns32 uns32;
	XMP_Int32 int32;

	switch ( thisTag->type ) {
	
		case kTIFF_ByteType:
			uns32 = GetUns8 ( thisTag->dataPtr );
			break;
	
		case kTIFF_ShortType:
			uns32 = this->GetUns16 ( thisTag->dataPtr );
			break;
	
		case kTIFF_LongType:
			uns32 = this->GetUns32 ( thisTag->dataPtr );
			break;
	
		case kTIFF_SByteType:
			int32 = (XMP_Int8) GetUns8 ( thisTag->dataPtr );
			uns32 = (XMP_Uns32) int32;	// Make sure sign bits are properly set.
			break;
	
		case kTIFF_SShortType:
			int32 = (XMP_Int16) this->GetUns16 ( thisTag->dataPtr );
			uns32 = (XMP_Uns32) int32;	// Make sure sign bits are properly set.
			break;
	
		case kTIFF_SLongType:
			int32 = (XMP_Int32) this->GetUns32 ( thisTag->dataPtr );
			uns32 = (XMP_Uns32) int32;	// Make sure sign bits are properly set.
			break;
	
		default: return false;

	}

	if ( data != 0 ) *data = uns32;
	return true;

}	// TIFF_FileWriter::GetTag_Integer

// =================================================================================================
// TIFF_FileWriter::GetTag_Byte
// ============================

bool TIFF_FileWriter::GetTag_Byte ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns8* data ) const
{
	const InternalTagInfo* thisTag = this->FindTagInIFD ( ifd, id );
	if ( thisTag == 0 ) return false;
	if ( (thisTag->type != kTIFF_ByteType) || (thisTag->dataLen != 1) ) return false;

	if ( data != 0 ) *data = *thisTag->dataPtr;
	return true;

}	// TIFF_FileWriter::GetTag_Byte

// =================================================================================================
// TIFF_FileWriter::GetTag_SByte
// =============================

bool TIFF_FileWriter::GetTag_SByte ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Int8* data ) const
{
	const InternalTagInfo* thisTag = this->FindTagInIFD ( ifd, id );
	if ( thisTag == 0 ) return false;
	if ( (thisTag->type != kTIFF_SByteType) || (thisTag->dataLen != 1) ) return false;

	if ( data != 0 ) *data = *thisTag->dataPtr;
	return true;

}	// TIFF_FileWriter::GetTag_SByte

// =================================================================================================
// TIFF_FileWriter::GetTag_Short
// =============================

bool TIFF_FileWriter::GetTag_Short ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns16* data ) const
{
	const InternalTagInfo* thisTag = this->FindTagInIFD ( ifd, id );
	if ( thisTag == 0 ) return false;
	if ( (thisTag->type != kTIFF_ShortType) || (thisTag->dataLen != 2) ) return false;

	if ( data != 0 ) *data = this->GetUns16 ( thisTag->dataPtr );
	return true;

}	// TIFF_FileWriter::GetTag_Short

// =================================================================================================
// TIFF_FileWriter::GetTag_SShort
// ==============================

bool TIFF_FileWriter::GetTag_SShort ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Int16* data ) const
{
	const InternalTagInfo* thisTag = this->FindTagInIFD ( ifd, id );
	if ( thisTag == 0 ) return false;
	if ( (thisTag->type != kTIFF_SShortType) || (thisTag->dataLen != 2) ) return false;

	if ( data != 0 ) *data = (XMP_Int16) this->GetUns16 ( thisTag->dataPtr );
	return true;

}	// TIFF_FileWriter::GetTag_SShort

// =================================================================================================
// TIFF_FileWriter::GetTag_Long
// ============================

bool TIFF_FileWriter::GetTag_Long ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns32* data ) const
{
	const InternalTagInfo* thisTag = this->FindTagInIFD ( ifd, id );
	if ( thisTag == 0 ) return false;
	if ( (thisTag->type != kTIFF_LongType) || (thisTag->dataLen != 4) ) return false;

	if ( data != 0 ) *data = this->GetUns32 ( thisTag->dataPtr );
	return true;

}	// TIFF_FileWriter::GetTag_Long

// =================================================================================================
// TIFF_FileWriter::GetTag_SLong
// =============================

bool TIFF_FileWriter::GetTag_SLong ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Int32* data ) const
{
	const InternalTagInfo* thisTag = this->FindTagInIFD ( ifd, id );
	if ( thisTag == 0 ) return false;
	if ( (thisTag->type != kTIFF_SLongType) || (thisTag->dataLen != 4) ) return false;

	if ( data != 0 ) *data = (XMP_Int32) this->GetUns32 ( thisTag->dataPtr );
	return true;

}	// TIFF_FileWriter::GetTag_SLong

// =================================================================================================
// TIFF_FileWriter::GetTag_Rational
// ================================

bool TIFF_FileWriter::GetTag_Rational ( XMP_Uns8 ifd, XMP_Uns16 id, Rational* data ) const
{
	const InternalTagInfo* thisTag = this->FindTagInIFD ( ifd, id );
	if ( (thisTag == 0) || (thisTag->dataPtr == 0) ) return false;
	if ( (thisTag->type != kTIFF_RationalType) || (thisTag->dataLen != 8) ) return false;

	if ( data != 0 ) {
		XMP_Uns32* dataPtr = (XMP_Uns32*)thisTag->dataPtr;
		data->num   = this->GetUns32 ( dataPtr );
		data->denom = this->GetUns32 ( dataPtr+1 );
	}

	return true;

}	// TIFF_FileWriter::GetTag_Rational

// =================================================================================================
// TIFF_FileWriter::GetTag_SRational
// =================================

bool TIFF_FileWriter::GetTag_SRational ( XMP_Uns8 ifd, XMP_Uns16 id, SRational* data ) const
{
	const InternalTagInfo* thisTag = this->FindTagInIFD ( ifd, id );
	if ( (thisTag == 0) || (thisTag->dataPtr == 0) ) return false;
	if ( (thisTag->type != kTIFF_SRationalType) || (thisTag->dataLen != 8) ) return false;

	if ( data != 0 ) {
		XMP_Uns32* dataPtr = (XMP_Uns32*)thisTag->dataPtr;
		data->num   = (XMP_Int32) this->GetUns32 ( dataPtr );
		data->denom = (XMP_Int32) this->GetUns32 ( dataPtr+1 );
	}

	return true;

}	// TIFF_FileWriter::GetTag_SRational

// =================================================================================================
// TIFF_FileWriter::GetTag_Float
// =============================

bool TIFF_FileWriter::GetTag_Float ( XMP_Uns8 ifd, XMP_Uns16 id, float* data ) const
{
	const InternalTagInfo* thisTag = this->FindTagInIFD ( ifd, id );
	if ( thisTag == 0 ) return false;
	if ( (thisTag->type != kTIFF_FloatType) || (thisTag->dataLen != 4) ) return false;

	if ( data != 0 ) *data = this->GetFloat ( thisTag->dataPtr );
	return true;

}	// TIFF_FileWriter::GetTag_Float

// =================================================================================================
// TIFF_FileWriter::GetTag_Double
// ==============================

bool TIFF_FileWriter::GetTag_Double ( XMP_Uns8 ifd, XMP_Uns16 id, double* data ) const
{
	const InternalTagInfo* thisTag = this->FindTagInIFD ( ifd, id );
	if ( (thisTag == 0) || (thisTag->dataPtr == 0) ) return false;
	if ( (thisTag->type != kTIFF_DoubleType) || (thisTag->dataLen != 8) ) return false;

	if ( data != 0 ) *data = this->GetDouble ( thisTag->dataPtr );
	return true;

}	// TIFF_FileWriter::GetTag_Double

// =================================================================================================
// TIFF_FileWriter::GetTag_ASCII
// =============================

bool TIFF_FileWriter::GetTag_ASCII ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_StringPtr* dataPtr, XMP_StringLen* dataLen ) const
{
	const InternalTagInfo* thisTag = this->FindTagInIFD ( ifd, id );
	if ( thisTag == 0 ) return false;
	if ( (thisTag->dataLen > 4) && (thisTag->dataPtr == 0) ) return false;
	if ( thisTag->type != kTIFF_ASCIIType ) return false;

	if ( dataPtr != 0 ) *dataPtr = (XMP_StringPtr)thisTag->dataPtr;
	if ( dataLen != 0 ) *dataLen = thisTag->dataLen;

	return true;

}	// TIFF_FileWriter::GetTag_ASCII

// =================================================================================================
// TIFF_FileWriter::GetTag_EncodedString
// =====================================

bool TIFF_FileWriter::GetTag_EncodedString ( XMP_Uns8 ifd, XMP_Uns16 id, std::string* utf8Str ) const
{
	const InternalTagInfo* thisTag = this->FindTagInIFD ( ifd, id );
	if ( thisTag == 0 ) return false;
	if ( thisTag->type != kTIFF_UndefinedType ) return false;

	if ( utf8Str == 0 ) return true;	// Return true if the converted string is not wanted.

	bool ok = this->DecodeString ( thisTag->dataPtr, thisTag->dataLen, utf8Str );
	return ok;

}	// TIFF_FileWriter::GetTag_EncodedString

// =================================================================================================
// TIFF_FileWriter::SetTag_EncodedString
// =====================================

void TIFF_FileWriter::SetTag_EncodedString ( XMP_Uns8 ifd, XMP_Uns16 id, const std::string& utf8Str, XMP_Uns8 encoding )
{
	std::string encodedStr;

	this->EncodeString ( utf8Str, encoding, &encodedStr );
	this->SetTag ( ifd, id, kTIFF_UndefinedType, (XMP_Uns32)encodedStr.size(), encodedStr.c_str() );

}	// TIFF_FileWriter::SetTag_EncodedString

// =================================================================================================
// TIFF_FileWriter::IsLegacyChanged
// ================================

bool TIFF_FileWriter::IsLegacyChanged()
{

	if ( ! this->changed ) return false;
	if ( this->legacyDeleted ) return true;

	for ( int ifd = 0; ifd < kTIFF_KnownIFDCount; ++ifd ) {

		InternalIFDInfo & thisIFD = this->containedIFDs[ifd];
		if ( ! thisIFD.changed ) continue;

		InternalTagMap::iterator tagPos;
		InternalTagMap::iterator tagEnd = thisIFD.tagMap.end();

		for ( tagPos = thisIFD.tagMap.begin(); tagPos != tagEnd; ++tagPos ) {
			InternalTagInfo & thisTag = tagPos->second;
			if ( thisTag.changed && (thisTag.id != kTIFF_XMP) ) return true;
		}

	}

	return false;	// Can get here if the XMP tag is the only one changed.

}	// TIFF_FileWriter::IsLegacyChanged

// =================================================================================================
// TIFF_FileWriter::ParseMemoryStream
// ==================================

void TIFF_FileWriter::ParseMemoryStream ( const void* data, XMP_Uns32 length, bool copyData /* = true */ )
{
	this->DeleteExistingInfo();
	this->memParsed = true;
	if ( length == 0 ) return;

	// Allocate space for the full in-memory stream and copy it.

	if ( ! copyData ) {
		XMP_Assert ( ! this->ownedStream );
		this->memStream = (XMP_Uns8*) data;
	} else {
		if ( length > 100*1024*1024 ) XMP_Throw ( "Outrageous length for memory-based TIFF", kXMPErr_BadTIFF );
		this->memStream = (XMP_Uns8*) malloc(length);
		if ( this->memStream == 0 ) XMP_Throw ( "Out of memory", kXMPErr_NoMemory );
		memcpy ( this->memStream, data, length );	// AUDIT: Safe, malloc'ed length bytes above.
		this->ownedStream = true;
	}
	
	this->tiffLength = length;
	XMP_Uns32 ifdLimit = this->tiffLength - 6;	// An IFD must start before this offset.

	// Find and process the primary, thumbnail, Exif, GPS, and Interoperability IFDs.

	XMP_Uns32 primaryIFDOffset = this->CheckTIFFHeader ( this->memStream, length );

	if ( primaryIFDOffset != 0 ) {
		XMP_Uns32 tnailOffset = this->ProcessMemoryIFD ( primaryIFDOffset, kTIFF_PrimaryIFD );
		if ( tnailOffset != 0 ) {
			if ( IsOffsetValid ( tnailOffset, 8, ifdLimit ) ) {	// Remove a bad Thumbnail IFD Offset
				( void ) this->ProcessMemoryIFD ( tnailOffset, kTIFF_TNailIFD );
			} else {
				XMP_Error error ( kXMPErr_BadTIFF, "Bad IFD offset" );
				this->NotifyClient (kXMPErrSev_Recoverable, error );
				this->DeleteTag ( kTIFF_PrimaryIFD, kTIFF_TNailIFD );
			}
		}
	}

	const InternalTagInfo* exifIFDTag = this->FindTagInIFD ( kTIFF_PrimaryIFD, kTIFF_ExifIFDPointer );
	if ( (exifIFDTag != 0) && (exifIFDTag->type == kTIFF_LongType) && (exifIFDTag->dataLen == 4) ) {
		XMP_Uns32 exifOffset = this->GetUns32 ( exifIFDTag->dataPtr );
		(void) this->ProcessMemoryIFD ( exifOffset, kTIFF_ExifIFD );
	}

	const InternalTagInfo* gpsIFDTag = this->FindTagInIFD ( kTIFF_PrimaryIFD, kTIFF_GPSInfoIFDPointer );
	if ( (gpsIFDTag != 0) && (gpsIFDTag->type == kTIFF_LongType) && (gpsIFDTag->dataLen == 4) ) {
		XMP_Uns32 gpsOffset = this->GetUns32 ( gpsIFDTag->dataPtr );
		if ( IsOffsetValid ( gpsOffset, 8, ifdLimit ) ) {	// Remove a bad GPS IFD offset.
			(void) this->ProcessMemoryIFD ( gpsOffset, kTIFF_GPSInfoIFD );
		} else {
			XMP_Error error ( kXMPErr_BadTIFF, "Bad IFD offset" );
			this->NotifyClient (kXMPErrSev_Recoverable, error );
			this->DeleteTag ( kTIFF_PrimaryIFD, kTIFF_GPSInfoIFDPointer );
		}
	}

	const InternalTagInfo* interopIFDTag = this->FindTagInIFD ( kTIFF_ExifIFD, kTIFF_InteroperabilityIFDPointer );
	if ( (interopIFDTag != 0) && (interopIFDTag->type == kTIFF_LongType) && (interopIFDTag->dataLen == 4) ) {
		XMP_Uns32 interopOffset = this->GetUns32 ( interopIFDTag->dataPtr );
		if ( IsOffsetValid ( interopOffset, 8, ifdLimit ) ) {	// Remove a bad Interoperability IFD offset.
			(void) this->ProcessMemoryIFD ( interopOffset, kTIFF_InteropIFD );
		} else {
			XMP_Error error ( kXMPErr_BadTIFF, "Bad IFD offset" );
			this->NotifyClient (kXMPErrSev_Recoverable, error );
			this->DeleteTag ( kTIFF_ExifIFD, kTIFF_InteroperabilityIFDPointer );
		}
	}

	#if 0
	{
		printf ( "\nExiting TIFF_FileWriter::ParseMemoryStream\n" );
		for ( int ifd = 0; ifd < kTIFF_KnownIFDCount; ++ifd ) {
			InternalIFDInfo & thisIFD = this->containedIFDs[ifd];
			printf ( "\n   IFD %d, count %d, mapped %d, offset %d (0x%X), next IFD %d (0x%X)\n",
					 ifd, thisIFD.origCount, thisIFD.tagMap.size(),
					 thisIFD.origDataOffset, thisIFD.origDataOffset, thisIFD.origNextIFD, thisIFD.origNextIFD );
			InternalTagMap::iterator tagPos;
			InternalTagMap::iterator tagEnd = thisIFD.tagMap.end();
			for ( tagPos = thisIFD.tagMap.begin(); tagPos != tagEnd; ++tagPos ) {
				InternalTagInfo & thisTag = tagPos->second;
				printf ( "      Tag %d, smallValue 0x%X, origDataLen %d, origDataOffset %d (0x%X)\n",
						 thisTag.id, thisTag.smallValue, thisTag.origDataLen, thisTag.origDataOffset, thisTag.origDataOffset );
			}
		}
		printf ( "\n" );
	}
	#endif

}	// TIFF_FileWriter::ParseMemoryStream

// =================================================================================================
// TIFF_FileWriter::ProcessMemoryIFD
// =================================

XMP_Uns32 TIFF_FileWriter::ProcessMemoryIFD ( XMP_Uns32 ifdOffset, XMP_Uns8 ifd )
{
	InternalIFDInfo& ifdInfo ( this->containedIFDs[ifd] );

	if ( (ifdOffset < 8) || (ifdOffset > (this->tiffLength - kEmptyIFDLength)) ) {
		XMP_Error error ( kXMPErr_BadTIFF, "Bad IFD offset" );
		this->NotifyClient ( kXMPErrSev_FileFatal, error );
	}

	XMP_Uns8* ifdPtr = this->memStream + ifdOffset;
	XMP_Uns16 tagCount = this->GetUns16 ( ifdPtr );
	RawIFDEntry* ifdEntries = (RawIFDEntry*)(ifdPtr+2);

	if ( tagCount >= 0x8000 ) {
		XMP_Error error ( kXMPErr_BadTIFF, "Outrageous IFD count" );
		this->NotifyClient ( kXMPErrSev_FileFatal, error );
	}

	if ( (XMP_Uns32)(2 + tagCount*12 + 4) > (this->tiffLength - ifdOffset) ) {
		XMP_Error error ( kXMPErr_BadTIFF, "Out of bounds IFD" );
		this->NotifyClient ( kXMPErrSev_FileFatal, error );
	}

	ifdInfo.origIFDOffset = ifdOffset;
	ifdInfo.origCount  = tagCount;

	for ( size_t i = 0; i < tagCount; ++i ) {

		RawIFDEntry* rawTag  = &ifdEntries[i];
		XMP_Uns16    tagType = this->GetUns16 ( &rawTag->type );
		if ( (tagType < kTIFF_ByteType) || (tagType > kTIFF_LastType) ) continue;	// Bad type, skip this tag.

		XMP_Uns16 tagID    = this->GetUns16 ( &rawTag->id );
		XMP_Uns32 tagCount = this->GetUns32 ( &rawTag->count );

		InternalTagMap::value_type mapValue ( tagID, InternalTagInfo ( tagID, tagType, tagCount, kIsMemoryBased ) );
		InternalTagMap::iterator newPos = ifdInfo.tagMap.insert ( ifdInfo.tagMap.end(), mapValue );
		InternalTagInfo& mapTag = newPos->second;

		mapTag.dataLen = mapTag.origDataLen = mapTag.count * (XMP_Uns32)kTIFF_TypeSizes[mapTag.type];
#if SUNOS_SPARC
		mapTag.smallValue = IE.getUns32(&rawTag->dataOrOffset);
#else
		mapTag.smallValue = GetUns32AsIs ( &rawTag->dataOrOffset );	// Keep the value or offset in stream byte ordering.
#endif //#if SUNOS_SPARC

		if ( mapTag.dataLen <= 4 ) {
			mapTag.origDataOffset = ifdOffset + 2 + (12 * (XMP_Uns32)i) + 8;	// Compute the data offset.
		} else {
			mapTag.origDataOffset = this->GetUns32 ( &rawTag->dataOrOffset );	// Extract the data offset.
			// printf ( "FW_ProcessMemoryIFD tag %d large value @ %.8X\n", mapTag.id, mapTag.dataPtr );
			if ( (mapTag.origDataOffset < 8) || (mapTag.origDataOffset >= this->tiffLength) ) {
				mapTag.count = mapTag.dataLen = mapTag.origDataLen = mapTag.smallValue = 0;
				mapTag.origDataOffset = ifdOffset + 2 + (12 * (XMP_Uns32)i) + 8;	// Make this bad tag look empty
			}
			if ( mapTag.dataLen > (this->tiffLength - mapTag.origDataOffset) ) {
				mapTag.count = mapTag.dataLen = mapTag.origDataLen = mapTag.smallValue = 0;
				mapTag.origDataOffset = ifdOffset + 2 + (12 * (XMP_Uns32)i) + 8;	// Make this bad tag look empty
			}
		}
		mapTag.dataPtr = this->memStream + mapTag.origDataOffset;

	}

	ifdPtr += (2 + tagCount*12);
	ifdInfo.origNextIFD = this->GetUns32 ( ifdPtr );

	return ifdInfo.origNextIFD;

}	// TIFF_FileWriter::ProcessMemoryIFD

// =================================================================================================
// TIFF_FileWriter::ParseFileStream
// ================================
//
// The buffered I/O model is worth the logic complexity - as opposed to a simple seek/read for each
// part of the TIFF stream. The vast majority of real-world TIFFs have the primary IFD, Exif IFD,
// and all of their interesting tag values within the first 64K of the file. Well, at least before
// we get around to our edit-by-append approach.

void TIFF_FileWriter::ParseFileStream ( XMP_IO* fileRef )
{

	this->DeleteExistingInfo();
	this->fileParsed = true;
	this->tiffLength = (XMP_Uns32) fileRef->Length();
	if ( this->tiffLength < 8 ) return;	// Ignore empty or impossibly short.
	fileRef->Rewind ( );

	XMP_Uns32 ifdLimit = this->tiffLength - 6;	// An IFD must start before this offset.

	// Find and process the primary, Exif, GPS, and Interoperability IFDs.

	XMP_Uns8 tiffHeader [8];
	fileRef->ReadAll ( tiffHeader, sizeof(tiffHeader) );
	XMP_Uns32 primaryIFDOffset = this->CheckTIFFHeader ( tiffHeader, this->tiffLength );

	if ( primaryIFDOffset == 0 ) {
		return;
	} else {
		XMP_Uns32 tnailOffset = this->ProcessFileIFD ( kTIFF_PrimaryIFD, primaryIFDOffset, fileRef );
		if ( tnailOffset != 0 ) {
			if ( IsOffsetValid ( tnailOffset, 8, ifdLimit ) ) {
				( void ) this->ProcessFileIFD ( kTIFF_TNailIFD, tnailOffset, fileRef );
			} else {
				XMP_Error error ( kXMPErr_BadTIFF, "Bad IFD offset" );
				this->NotifyClient ( kXMPErrSev_Recoverable, error );
				this->DeleteTag( kTIFF_PrimaryIFD, kTIFF_TNailIFD );
			}
		}
	}

	const InternalTagInfo* exifIFDTag = this->FindTagInIFD ( kTIFF_PrimaryIFD, kTIFF_ExifIFDPointer );
	if ( (exifIFDTag != 0) && (exifIFDTag->type == kTIFF_LongType) && (exifIFDTag->count == 1) ) {
		XMP_Uns32 exifOffset = this->GetUns32 ( exifIFDTag->dataPtr );
		(void) this->ProcessFileIFD ( kTIFF_ExifIFD, exifOffset, fileRef );
	}

	const InternalTagInfo* gpsIFDTag = this->FindTagInIFD ( kTIFF_PrimaryIFD, kTIFF_GPSInfoIFDPointer );
	if ( (gpsIFDTag != 0) && (gpsIFDTag->type == kTIFF_LongType) && (gpsIFDTag->count == 1) ) {
		XMP_Uns32 gpsOffset = this->GetUns32 ( gpsIFDTag->dataPtr );
		if ( IsOffsetValid (gpsOffset, 8, ifdLimit ) ) {	// Remove a bad GPS IFD offset.
			(void) this->ProcessFileIFD ( kTIFF_GPSInfoIFD, gpsOffset, fileRef );
		} else {
			XMP_Error error ( kXMPErr_BadTIFF, "Bad IFD offset" );
			this->NotifyClient ( kXMPErrSev_Recoverable, error );
			this->DeleteTag ( kTIFF_PrimaryIFD, kTIFF_GPSInfoIFDPointer );
		}
	}

	const InternalTagInfo* interopIFDTag = this->FindTagInIFD ( kTIFF_ExifIFD, kTIFF_InteroperabilityIFDPointer );
	if ( (interopIFDTag != 0) && (interopIFDTag->type == kTIFF_LongType) && (interopIFDTag->dataLen == 4) ) {
		XMP_Uns32 interopOffset = this->GetUns32 ( interopIFDTag->dataPtr );
		if ( IsOffsetValid (interopOffset, 8, ifdLimit ) ) {	// Remove a bad Interoperability IFD offset.
			(void) this->ProcessFileIFD ( kTIFF_InteropIFD, interopOffset, fileRef );
		} else {
			XMP_Error error ( kXMPErr_BadTIFF, "Bad IFD offset" );
			this->NotifyClient ( kXMPErrSev_Recoverable, error );
			this->DeleteTag ( kTIFF_ExifIFD, kTIFF_InteroperabilityIFDPointer );
		}
	}

	#if 0
	{
		printf ( "\nExiting TIFF_FileWriter::ParseFileStream\n" );
		for ( int ifd = 0; ifd < kTIFF_KnownIFDCount; ++ifd ) {
			InternalIFDInfo & thisIFD = this->containedIFDs[ifd];
			printf ( "\n   IFD %d, count %d, mapped %d, offset %d (0x%X), next IFD %d (0x%X)\n",
					 ifd, thisIFD.origCount, thisIFD.tagMap.size(),
					 thisIFD.origDataOffset, thisIFD.origDataOffset, thisIFD.origNextIFD, thisIFD.origNextIFD );
			InternalTagMap::iterator tagPos;
			InternalTagMap::iterator tagEnd = thisIFD.tagMap.end();
			for ( tagPos = thisIFD.tagMap.begin(); tagPos != tagEnd; ++tagPos ) {
				InternalTagInfo & thisTag = tagPos->second;
				printf ( "      Tag %d, smallValue 0x%X, origDataLen %d, origDataOffset %d (0x%X)\n",
						 thisTag.id, thisTag.smallValue, thisTag.origDataLen, thisTag.origDataOffset, thisTag.origDataOffset );
			}
		}
		printf ( "\n" );
	}
	#endif

}	// TIFF_FileWriter::ParseFileStream

// =================================================================================================
// TIFF_FileWriter::ProcessFileIFD
// ===============================
//
// Each IFD has a UInt16 count of IFD entries, a sequence of 12 byte IFD entries, then a UInt32
// offset to the next IFD. The integer byte order is determined by the II or MM at the TIFF start.

XMP_Uns32 TIFF_FileWriter::ProcessFileIFD ( XMP_Uns8 ifd, XMP_Uns32 ifdOffset, XMP_IO* fileRef )
{
	static const size_t ifdBufferSize = 12*65536;	// Enough for the largest possible IFD.
    std::vector<XMP_Uns8> ifdBuffer(ifdBufferSize);
	XMP_Uns8 intBuffer [4];	// For the IFD count and offset to next IFD.
	
	InternalIFDInfo& ifdInfo ( this->containedIFDs[ifd] );

	if ( (ifdOffset < 8) || (ifdOffset > (this->tiffLength - kEmptyIFDLength)) ) {
		XMP_Throw ( "Bad IFD offset", kXMPErr_BadTIFF );
	}

	fileRef->Seek ( ifdOffset, kXMP_SeekFromStart );
	if ( ! XIO::CheckFileSpace ( fileRef, 2 ) ) return 0;	// Bail for a truncated file.
	fileRef->ReadAll ( intBuffer, 2 );

	XMP_Uns16 tagCount = this->GetUns16 ( intBuffer );
	if ( tagCount >= 0x8000 ) return 0;	// Maybe wrong byte order.
	if ( ! XIO::CheckFileSpace ( fileRef, 12*tagCount ) ) return 0;	// Bail for a truncated file.
	fileRef->ReadAll ( &ifdBuffer[0], 12*tagCount );

	if ( ! XIO::CheckFileSpace ( fileRef, 4 ) ) {
        ifdInfo.origNextIFD = 0;	// Tolerate a trncated file, do the remaining processing.
	} else {
		fileRef->ReadAll ( intBuffer, 4 );
        ifdInfo.origNextIFD = this->GetUns32 ( intBuffer );
	}

	ifdInfo.origIFDOffset = ifdOffset;
	ifdInfo.origCount  = tagCount;

	// ---------------------------------------------------------------------------------------------
	// First create all of the IFD map entries, capturing short values, and get the next IFD offset.
	// We're using a std::map for storage, it automatically eliminates duplicates and provides
	// sorted output. Plus the "map[key] = value" assignment conveniently keeps the last encountered
	// value, following Photoshop's behavior.

	XMP_Uns8* ifdPtr = &ifdBuffer[0];	// Move to the first IFD entry.

	for ( XMP_Uns16 i = 0; i < tagCount; ++i, ifdPtr += 12 ) {

		RawIFDEntry* rawTag = (RawIFDEntry*)ifdPtr;
		XMP_Uns16    tagType = this->GetUns16 ( &rawTag->type );
		if ( (tagType < kTIFF_ByteType) || (tagType > kTIFF_LastType) ) continue;	// Bad type, skip this tag.

		XMP_Uns16 tagID    = this->GetUns16 ( &rawTag->id );
		XMP_Uns32 tagCount = this->GetUns32 ( &rawTag->count );

		InternalTagMap::value_type mapValue ( tagID, InternalTagInfo ( tagID, tagType, tagCount, kIsFileBased ) );
		InternalTagMap::iterator newPos = ifdInfo.tagMap.insert ( ifdInfo.tagMap.end(), mapValue );
		InternalTagInfo& mapTag = newPos->second;

		mapTag.dataLen = mapTag.origDataLen = mapTag.count * (XMP_Uns32)kTIFF_TypeSizes[mapTag.type];
		mapTag.smallValue = GetUns32AsIs ( &rawTag->dataOrOffset );	// Keep the value or offset in stream byte ordering.

		if ( mapTag.dataLen <= 4 ) {
			mapTag.dataPtr = (XMP_Uns8*) &mapTag.smallValue;
			mapTag.origDataOffset = ifdOffset + 2 + (12 * i) + 8;	// Compute the data offset.
		} else {
			mapTag.origDataOffset = this->GetUns32 ( &rawTag->dataOrOffset );	// Extract the data offset.
			if ( (mapTag.origDataOffset < 8) || (mapTag.origDataOffset >= this->tiffLength) ) {
				mapTag.dataPtr = (XMP_Uns8*) &mapTag.smallValue;	// Make this bad tag look empty.
				mapTag.origDataOffset = ifdOffset + 2 + (12 * i) + 8;
				mapTag.count = mapTag.dataLen = mapTag.origDataLen = mapTag.smallValue = 0;
			}
			if ( mapTag.dataLen > (this->tiffLength - mapTag.origDataOffset) ) {
				mapTag.dataPtr = (XMP_Uns8*) &mapTag.smallValue;	// Make this bad tag look empty.
				mapTag.origDataOffset = ifdOffset + 2 + (12 * i) + 8;
				mapTag.count = mapTag.dataLen = mapTag.origDataLen = mapTag.smallValue = 0;
			}
		}

	}

	// ------------------------------------------------------------------------
	// Go back over the tag map and extract the data for large recognized tags.

	InternalTagMap::iterator tagPos = ifdInfo.tagMap.begin();
	InternalTagMap::iterator tagEnd = ifdInfo.tagMap.end();

	const XMP_Uns16* knownTagPtr = sKnownTags[ifd];	// Points into the ordered recognized tag list.

	for ( ; tagPos != tagEnd; ++tagPos ) {

		InternalTagInfo* currTag = &tagPos->second;

		if ( currTag->dataLen <= 4 ) continue;	// Short values are already in the smallValue field.

		while ( *knownTagPtr < currTag->id ) ++knownTagPtr;
		if ( *knownTagPtr != currTag->id ) continue;	// Skip unrecognized tags.

		fileRef->Seek ( currTag->origDataOffset, kXMP_SeekFromStart );
		currTag->dataPtr = (XMP_Uns8*) malloc ( currTag->dataLen );
		if ( currTag->dataPtr == 0 ) XMP_Throw ( "No data block", kXMPErr_NoMemory );
		fileRef->ReadAll ( currTag->dataPtr, currTag->dataLen );

	}

	// Done, return the next IFD offset.

	return ifdInfo.origNextIFD;

}	// TIFF_FileWriter::ProcessFileIFD

// =================================================================================================
// TIFF_FileWriter::IntegrateFromPShop6
// ====================================
//
// See comments for ProcessPShop6IFD.

void TIFF_FileWriter::IntegrateFromPShop6 ( const void * buriedPtr, size_t buriedLen )
{
	TIFF_MemoryReader buriedExif;
	buriedExif.ParseMemoryStream ( buriedPtr, (XMP_Uns32) buriedLen );

	this->ProcessPShop6IFD ( buriedExif, kTIFF_PrimaryIFD );
	this->ProcessPShop6IFD ( buriedExif, kTIFF_ExifIFD );
	this->ProcessPShop6IFD ( buriedExif, kTIFF_GPSInfoIFD );

}	// TIFF_FileWriter::IntegrateFromPShop6

// =================================================================================================
// TIFF_FileWriter::CopyTagToMasterIFD
// ===================================
//
// Create a new master IFD entry from a buried Photoshop 6 IFD entry. Don't try to get clever with
// large values, just create a new copy. This preserves a clean separation between the memory-based
// and file-based TIFF processing.

void* TIFF_FileWriter::CopyTagToMasterIFD ( const TagInfo & ps6Tag, InternalIFDInfo * masterIFD )
{
	InternalTagMap::value_type mapValue ( ps6Tag.id, InternalTagInfo ( ps6Tag.id, ps6Tag.type, ps6Tag.count, this->fileParsed ) );
	InternalTagMap::iterator newPos = masterIFD->tagMap.insert ( masterIFD->tagMap.end(), mapValue );
	InternalTagInfo& newTag = newPos->second;

	newTag.dataLen = ps6Tag.dataLen;

	if ( newTag.dataLen <= 4 ) {
		newTag.dataPtr = (XMP_Uns8*) &newTag.smallValue;
		newTag.smallValue = *((XMP_Uns32*)ps6Tag.dataPtr);
	} else {
		newTag.dataPtr = (XMP_Uns8*) malloc ( newTag.dataLen );
		if ( newTag.dataPtr == 0 ) XMP_Throw ( "Out of memory", kXMPErr_NoMemory );
		memcpy ( newTag.dataPtr, ps6Tag.dataPtr, newTag.dataLen );	// AUDIT: Safe, malloc'ed dataLen bytes above.
	}

	newTag.changed = true;	// ! See comments with ProcessPShop6IFD.
	XMP_Assert ( (newTag.origDataLen == 0) && (newTag.origDataOffset == 0) );

	masterIFD->changed = true;

	return newPos->second.dataPtr;	// ! Return the address within the map entry for small values.

}	// TIFF_FileWriter::CopyTagToMasterIFD

// =================================================================================================
// FlipCFATable
// ============
//
// The CFA pattern table is trivial, a pair of short counts followed by n*m bytes.

static bool FlipCFATable ( void* voidPtr, XMP_Uns32 tagLen, GetUns16_Proc GetUns16 )
{
	if ( tagLen < 4 ) return false;

	XMP_Uns16* u16Ptr = (XMP_Uns16*)voidPtr;

	Flip2 ( &u16Ptr[0] );	// Flip the counts to match the master TIFF.
	Flip2 ( &u16Ptr[1] );

	XMP_Uns16 columns = GetUns16 ( &u16Ptr[0] );	// Fetch using the master TIFF's routine.
	XMP_Uns16 rows    = GetUns16 ( &u16Ptr[1] );

	if ( tagLen != (XMP_Uns32)(4 + columns*rows) ) return false;

	return true;

}	// FlipCFATable

// =================================================================================================
// FlipDSDTable
// ============
//
// The device settings description table is trivial, a pair of short counts followed by UTF-16
// strings. So the whole value should be flipped as a sequence of 16 bit items.

// ! The Exif 2.2 description is a bit garbled. It might be wrong. It would be nice to have a real example.

static bool FlipDSDTable ( void* voidPtr, XMP_Uns32 tagLen, GetUns16_Proc GetUns16 )
{
	if ( tagLen < 4 ) return false;

	XMP_Uns16* u16Ptr = (XMP_Uns16*)voidPtr;
	for ( size_t i = tagLen/2; i > 0; --i, ++u16Ptr ) Flip2 ( u16Ptr );

	return true;

}	// FlipDSDTable

// =================================================================================================
// FlipOECFSFRTable
// ================
//
// The OECF and SFR tables have the same layout:
//    2 short counts, columns and rows
//    c ASCII strings, null terminated, column names
//    c*r rationals

static bool FlipOECFSFRTable ( void* voidPtr, XMP_Uns32 tagLen, GetUns16_Proc GetUns16 )
{
	XMP_Uns16* u16Ptr = (XMP_Uns16*)voidPtr;

	Flip2 ( &u16Ptr[0] );	// Flip the data to match the master TIFF.
	Flip2 ( &u16Ptr[1] );

	XMP_Uns16 columns = GetUns16 ( &u16Ptr[0] );	// Fetch using the master TIFF's routine.
	XMP_Uns16 rows    = GetUns16 ( &u16Ptr[1] );

	XMP_Uns32 minLen = 4 + columns + (8 * columns * rows);	// Minimum legit tag size.
	if ( tagLen < minLen ) return false;

	// Compute the start of the rationals from the end of value. No need to walk through the names.
	XMP_Uns32* u32Ptr = (XMP_Uns32*) ((XMP_Uns8*)voidPtr + tagLen - (8 * columns * rows));

	for ( size_t i = 2*columns*rows; i > 0; --i, ++u32Ptr ) Flip4 ( u32Ptr );

	return true;

}	// FlipOECFSFRTable

// =================================================================================================
// TIFF_FileWriter::ProcessPShop6IFD
// =================================
//
// Photoshop 6 wrote wacky TIFF files that have much of the Exif metadata buried inside of image
// resource 1058, which is itself within tag 34377 in the 0th IFD. This routine moves the buried
// tags up to the parent file. Existing tags are not replaced.
//
// While it is tempting to try to directly use the TIFF_MemoryReader's tweaked IFD info, making that
// visible would compromise implementation separation. Better to pay the modest runtime cost of
// using the official GetIFD method, letting it build the map.
//
// The tags that get moved are marked as being changed, as is the IFD they are moved into, but the
// overall TIFF_FileWriter object is not. We don't want this integration on its own to force a file
// update, but a file update should include these changes.

// ! Be careful to not move tags that are the nasty Exif explicit offsets, e.g. the Exif or GPS IFD
// ! "pointers". These are tags with a LONG type and count of 1, whose value is an offset into the
// ! buried TIFF stream. We can't reliably plant that offset into the outer IFD structure.

// ! To make things even more fun, the buried Exif might not have the same endianness as the outer!

void TIFF_FileWriter::ProcessPShop6IFD ( const TIFF_MemoryReader& buriedExif, XMP_Uns8 ifd )
{
	bool ok, found;
	TagInfoMap ps6IFD;

	found = buriedExif.GetIFD ( ifd, &ps6IFD );
	if ( ! found ) return;

	bool needsFlipping = (this->bigEndian != buriedExif.IsBigEndian());

	InternalIFDInfo* masterIFD = &this->containedIFDs[ifd];

	TagInfoMap::const_iterator ps6Pos = ps6IFD.begin();
	TagInfoMap::const_iterator ps6End = ps6IFD.end();

	for ( ; ps6Pos != ps6End; ++ps6Pos ) {

		// Copy buried tags to the master IFD if they don't already exist there.

		const TagInfo& ps6Tag = ps6Pos->second;

		if ( this->FindTagInIFD ( ifd, ps6Tag.id ) != 0 ) continue;	// Keep existing master tags.
		if ( needsFlipping && (ps6Tag.id == 37500) ) continue;	// Don't copy an unflipped MakerNote.
		if ( (ps6Tag.id == kTIFF_ExifIFDPointer) ||	// Skip the tags that are explicit offsets.
			 (ps6Tag.id == kTIFF_GPSInfoIFDPointer) ||
			 (ps6Tag.id == kTIFF_JPEGInterchangeFormat) ||
			 (ps6Tag.id == kTIFF_InteroperabilityIFDPointer) ) continue;

		void* voidPtr = this->CopyTagToMasterIFD ( ps6Tag, masterIFD );

		if ( needsFlipping ) {
			switch ( ps6Tag.type ) {

				case kTIFF_ByteType:
				case kTIFF_SByteType:
				case kTIFF_ASCIIType:
					// Nothing more to do.
					break;

				case kTIFF_ShortType:
				case kTIFF_SShortType:
					{
						XMP_Uns16* u16Ptr = (XMP_Uns16*)voidPtr;
						for ( size_t i = ps6Tag.count; i > 0; --i, ++u16Ptr ) Flip2 ( u16Ptr );
					}
					break;

				case kTIFF_LongType:
				case kTIFF_SLongType:
				case kTIFF_FloatType:
					{
						XMP_Uns32* u32Ptr = (XMP_Uns32*)voidPtr;
						for ( size_t i = ps6Tag.count; i > 0; --i, ++u32Ptr ) Flip4 ( u32Ptr );
					}
					break;

				case kTIFF_RationalType:
				case kTIFF_SRationalType:
					{
						XMP_Uns32* ratPtr = (XMP_Uns32*)voidPtr;
						for ( size_t i = (2 * ps6Tag.count); i > 0; --i, ++ratPtr ) Flip4 ( ratPtr );
					}
					break;

				case kTIFF_DoubleType:
					{
						XMP_Uns64* u64Ptr = (XMP_Uns64*)voidPtr;
						for ( size_t i = ps6Tag.count; i > 0; --i, ++u64Ptr ) Flip8 ( u64Ptr );
					}
					break;

				case kTIFF_UndefinedType:
					// Fix up the few kinds of special tables that Exif 2.2 defines.
					ok = true;	// Keep everything that isn't a special table.
					if ( ps6Tag.id == kTIFF_CFAPattern ) {
						ok = FlipCFATable ( voidPtr, ps6Tag.dataLen, this->GetUns16 );
					} else if ( ps6Tag.id == kTIFF_DeviceSettingDescription ) {
						ok = FlipDSDTable ( voidPtr, ps6Tag.dataLen, this->GetUns16 );
					} else if ( (ps6Tag.id == kTIFF_OECF) || (ps6Tag.id == kTIFF_SpatialFrequencyResponse) ) {
						ok = FlipOECFSFRTable ( voidPtr, ps6Tag.dataLen, this->GetUns16 );
					}
					if ( ! ok ) this->DeleteTag ( ifd, ps6Tag.id );
					break;

				default:
					// ? XMP_Throw ( "Unexpected tag type", kXMPErr_InternalFailure );
					this->DeleteTag ( ifd, ps6Tag.id );
					break;

			}
		}

	}

}	// TIFF_FileWriter::ProcessPShop6IFD

// =================================================================================================
// TIFF_FileWriter::PreflightIFDLinkage
// ====================================
//
// Preflight special cases for the linkage between IFDs. Three of the IFDs are found through an
// explicit tag, the Exif, GPS, and Interop IFDs. The presence or absence of those IFDs affects the
// presence or absence of the linkage tag, which can affect the IFD containing the linkage tag. The
// thumbnail IFD is chained from the primary IFD, so if the thumbnail IFD is present we make sure
// that the primary IFD isn't empty.

void TIFF_FileWriter::PreflightIFDLinkage()
{

	// Do the tag-linked IFDs bottom up, Interop then GPS then Exif.

	if ( this->containedIFDs[kTIFF_InteropIFD].tagMap.empty() ) {
		this->DeleteTag ( kTIFF_ExifIFD, kTIFF_InteroperabilityIFDPointer );
	} else if ( ! this->GetTag ( kTIFF_ExifIFD, kTIFF_InteroperabilityIFDPointer, 0 ) ) {
		this->SetTag_Long ( kTIFF_ExifIFD, kTIFF_InteroperabilityIFDPointer, 0xABADABAD );
	}

	if ( this->containedIFDs[kTIFF_GPSInfoIFD].tagMap.empty() ) {
		this->DeleteTag ( kTIFF_PrimaryIFD, kTIFF_GPSInfoIFDPointer );
	} else if ( ! this->GetTag ( kTIFF_PrimaryIFD, kTIFF_GPSInfoIFDPointer, 0 ) ) {
		this->SetTag_Long ( kTIFF_PrimaryIFD, kTIFF_GPSInfoIFDPointer, 0xABADABAD );
	}

	if ( this->containedIFDs[kTIFF_ExifIFD].tagMap.empty() ) {
		this->DeleteTag ( kTIFF_PrimaryIFD, kTIFF_ExifIFDPointer );
	} else if ( ! this->GetTag ( kTIFF_PrimaryIFD, kTIFF_ExifIFDPointer, 0 ) ) {
		this->SetTag_Long ( kTIFF_PrimaryIFD, kTIFF_ExifIFDPointer, 0xABADABAD );
	}

	// Make sure that the primary IFD is not empty if the thumbnail IFD is not empty.

	if ( this->containedIFDs[kTIFF_PrimaryIFD].tagMap.empty() &&
		 (! this->containedIFDs[kTIFF_TNailIFD].tagMap.empty()) ) {
		this->SetTag_Short ( kTIFF_PrimaryIFD, kTIFF_ResolutionUnit, 2 );	// Set Resolution unit to inches.
	}

}	// TIFF_FileWriter::PreflightIFDLinkage

// =================================================================================================
// TIFF_FileWriter::DetermineVisibleLength
// =======================================

XMP_Uns32 TIFF_FileWriter::DetermineVisibleLength()
{
	XMP_Uns32 visibleLength = 8;	// Start with the TIFF header size.

	for ( int ifd = 0; ifd < kTIFF_KnownIFDCount; ++ifd ) {

		InternalIFDInfo& ifdInfo ( this->containedIFDs[ifd] );
		size_t tagCount = ifdInfo.tagMap.size();
		if ( tagCount == 0 ) continue;

		visibleLength += (XMP_Uns32)( 6 + (12 * tagCount) );

		InternalTagMap::iterator tagPos = ifdInfo.tagMap.begin();
		InternalTagMap::iterator tagEnd = ifdInfo.tagMap.end();

		for ( ; tagPos != tagEnd; ++tagPos ) {
			InternalTagInfo & currTag ( tagPos->second );
			if ( currTag.dataLen > 4 ) visibleLength += ((currTag.dataLen + 1) & 0xFFFFFFFE);	// ! Round to even lengths.
		}

	}

	return visibleLength;

}	// TIFF_FileWriter::DetermineVisibleLength

// =================================================================================================
// TIFF_FileWriter::DetermineAppendInfo
// ====================================

#ifndef Trace_DetermineAppendInfo
	#define Trace_DetermineAppendInfo 0
#endif

// An IFD grows if it has more tags than before.
#define DoesIFDGrow(ifd)	(this->containedIFDs[ifd].origCount < this->containedIFDs[ifd].tagMap.size())

XMP_Uns32 TIFF_FileWriter::DetermineAppendInfo ( XMP_Uns32 appendedOrigin,
												 bool      appendedIFDs[kTIFF_KnownIFDCount],
												 XMP_Uns32 newIFDOffsets[kTIFF_KnownIFDCount],
												 bool      appendAll /* = false */ )
{
	XMP_Uns32 appendedLength = 0;
	XMP_Assert ( (appendedOrigin & 1) == 0 );	// Make sure it is even.

	#if Trace_DetermineAppendInfo
	{
		printf ( "\nEntering TIFF_FileWriter::DetermineAppendInfo%s\n", (appendAll ? ", append all" : "") );
		for ( int ifd = 0; ifd < kTIFF_KnownIFDCount; ++ifd ) {
			InternalIFDInfo & thisIFD = this->containedIFDs[ifd];
			printf ( "\n   IFD %d, origCount %d, map.size %d, origIFDOffset %d (0x%X), origNextIFD %d (0x%X)",
					 ifd, thisIFD.origCount, thisIFD.tagMap.size(),
					 thisIFD.origIFDOffset, thisIFD.origIFDOffset, thisIFD.origNextIFD, thisIFD.origNextIFD );
			if ( thisIFD.changed ) printf ( ", changed" );
			if ( thisIFD.origCount < thisIFD.tagMap.size() ) printf ( ", should get appended" );
			printf ( "\n" );
			InternalTagMap::iterator tagPos;
			InternalTagMap::iterator tagEnd = thisIFD.tagMap.end();
			for ( tagPos = thisIFD.tagMap.begin(); tagPos != tagEnd; ++tagPos ) {
				InternalTagInfo & thisTag = tagPos->second;
				printf ( "      Tag %d, smallValue 0x%X, origDataLen %d, origDataOffset %d (0x%X)",
						 thisTag.id, thisTag.smallValue, thisTag.origDataLen, thisTag.origDataOffset, thisTag.origDataOffset );
				if ( thisTag.changed ) printf ( ", changed" );
				if ( (thisTag.dataLen > thisTag.origDataLen) && (thisTag.dataLen > 4) ) printf ( ", should get appended" );
				printf ( "\n" );
			}
		}
		printf ( "\n" );
	}
	#endif

	// Determine which of the IFDs will be appended. If the Exif, GPS, or Interoperability IFDs are
	// appended, set dummy values for their offsets in the "owning" IFD. This must be done first
	// since this might cause the owning IFD to grow.

	if ( ! appendAll ) {
		for ( int i = 0; i < kTIFF_KnownIFDCount ;++i ) appendedIFDs[i] = false;
	} else {
		for ( int i = 0; i < kTIFF_KnownIFDCount ;++i ) appendedIFDs[i] = (this->containedIFDs[i].tagMap.size() > 0);
	}

	appendedIFDs[kTIFF_InteropIFD] |= DoesIFDGrow ( kTIFF_InteropIFD );
	if ( appendedIFDs[kTIFF_InteropIFD] ) {
		this->SetTag_Long ( kTIFF_ExifIFD, kTIFF_InteroperabilityIFDPointer, 0xABADABAD );
	}

	appendedIFDs[kTIFF_GPSInfoIFD] |= DoesIFDGrow ( kTIFF_GPSInfoIFD );
	if ( appendedIFDs[kTIFF_GPSInfoIFD] ) {
		this->SetTag_Long ( kTIFF_PrimaryIFD, kTIFF_GPSInfoIFDPointer, 0xABADABAD );
	}

	appendedIFDs[kTIFF_ExifIFD] |= DoesIFDGrow ( kTIFF_ExifIFD );
	if ( appendedIFDs[kTIFF_ExifIFD] ) {
		this->SetTag_Long ( kTIFF_PrimaryIFD, kTIFF_ExifIFDPointer, 0xABADABAD );
	}

	appendedIFDs[kTIFF_PrimaryIFD] |= DoesIFDGrow ( kTIFF_PrimaryIFD );

	// The appended data (if any) will be a sequence of an IFD followed by its large values.
	// Determine the new offsets for the appended IFDs and tag values, and the total amount of
	// appended stuff. The final IFD offset is set in newIFDOffsets for all IFDs, changed or not.
	// This makes it easier to set the offsets to the primary and thumbnail IFDs when writing.

	for ( int ifd = 0; ifd < kTIFF_KnownIFDCount ;++ifd ) {

		InternalIFDInfo& ifdInfo ( this->containedIFDs[ifd] );
		size_t tagCount = ifdInfo.tagMap.size();

		newIFDOffsets[ifd] = ifdInfo.origIFDOffset;	// Make the new offset valid for unchanged IFDs.

		if ( ! (appendAll | ifdInfo.changed) ) continue;
		if ( tagCount == 0 ) continue;

		if ( appendedIFDs[ifd] ) {
			newIFDOffsets[ifd] = appendedOrigin + appendedLength;
			appendedLength += (XMP_Uns32)( 6 + (12 * tagCount) );
		}

		InternalTagMap::iterator tagPos = ifdInfo.tagMap.begin();
		InternalTagMap::iterator tagEnd = ifdInfo.tagMap.end();

		for ( ; tagPos != tagEnd; ++tagPos ) {

			InternalTagInfo & currTag ( tagPos->second );
			if ( (! (appendAll | currTag.changed)) || (currTag.dataLen <= 4) ) continue;

			if ( (currTag.dataLen <= currTag.origDataLen) && (! appendAll) ) {
				this->PutUns32 ( currTag.origDataOffset, &currTag.smallValue );	// Reuse the old space.
			} else {
				this->PutUns32 ( (appendedOrigin + appendedLength), &currTag.smallValue );	// Set the appended offset.
				appendedLength += ((currTag.dataLen + 1) & 0xFFFFFFFEUL);	// Round to an even size.
			}

		}

	}

	// If the Exif, GPS, or Interoperability IFDs get appended, update the tag values for their new offsets.

	if ( appendedIFDs[kTIFF_ExifIFD] ) {
		this->SetTag_Long ( kTIFF_PrimaryIFD, kTIFF_ExifIFDPointer, newIFDOffsets[kTIFF_ExifIFD] );
	}
	if ( appendedIFDs[kTIFF_GPSInfoIFD] ) {
		this->SetTag_Long ( kTIFF_PrimaryIFD, kTIFF_GPSInfoIFDPointer, newIFDOffsets[kTIFF_GPSInfoIFD] );
	}
	if ( appendedIFDs[kTIFF_InteropIFD] ) {
		this->SetTag_Long ( kTIFF_ExifIFD, kTIFF_InteroperabilityIFDPointer, newIFDOffsets[kTIFF_InteropIFD] );
	}

	#if Trace_DetermineAppendInfo
	{
		printf ( "Exiting TIFF_FileWriter::DetermineAppendInfo\n" );
		for ( int ifd = 0; ifd < kTIFF_KnownIFDCount; ++ifd ) {
			InternalIFDInfo & thisIFD = this->containedIFDs[ifd];
			printf ( "\n   IFD %d, origCount %d, map.size %d, origIFDOffset %d (0x%X), origNextIFD %d (0x%X)",
					 ifd, thisIFD.origCount, thisIFD.tagMap.size(),
					 thisIFD.origIFDOffset, thisIFD.origIFDOffset, thisIFD.origNextIFD, thisIFD.origNextIFD );
			if ( thisIFD.changed ) printf ( ", changed" );
			if ( appendedIFDs[ifd] ) printf ( ", will be appended at %d (0x%X)", newIFDOffsets[ifd], newIFDOffsets[ifd] );
			printf ( "\n" );
			InternalTagMap::iterator tagPos;
			InternalTagMap::iterator tagEnd = thisIFD.tagMap.end();
			for ( tagPos = thisIFD.tagMap.begin(); tagPos != tagEnd; ++tagPos ) {
				InternalTagInfo & thisTag = tagPos->second;
				printf ( "      Tag %d, smallValue 0x%X, origDataLen %d, origDataOffset %d (0x%X)",
						 thisTag.id, thisTag.smallValue, thisTag.origDataLen, thisTag.origDataOffset, thisTag.origDataOffset );
				if ( thisTag.changed ) printf ( ", changed" );
				if ( (thisTag.dataLen > thisTag.origDataLen) && (thisTag.dataLen > 4) ) {
					XMP_Uns32 newOffset = this->GetUns32 ( &thisTag.smallValue );
					printf ( ", will be appended at %d (0x%X)", newOffset, newOffset );
				}
				printf ( "\n" );
			}
		}
		printf ( "\n" );
	}
	#endif

	return appendedLength;

}	// TIFF_FileWriter::DetermineAppendInfo

// =================================================================================================
// TIFF_FileWriter::UpdateMemByAppend
// ==================================
//
// Normally we update TIFF in a conservative "by-append" manner. Changes are written in-place where
// they fit, anything requiring growth is appended to the end and the old space is abandoned. The
// end for memory-based TIFF is the end of the data block, the end for file-based TIFF is the end of
// the file. This update-by-append model has the advantage of not perturbing any hidden offsets, a
// common feature of proprietary MakerNotes.
//
// When doing the update-by-append we're only going to be modifying things that have changed. This
// means IFDs with changed, added, or deleted tags, and large values for changed or added tags. The
// IFDs and tag values are updated in-place if they fit, leaving holes in the stream if the new
// value is smaller than the old.

// ** Someday we might want to use the FreeOffsets and FreeByteCounts tags to track free space.
// ** Probably not a huge win in practice though, and the TIFF spec says they are not recommended
// ** for general interchange use.

void TIFF_FileWriter::UpdateMemByAppend ( XMP_Uns8** newStream_out, XMP_Uns32* newLength_out,
										  bool appendAll /* = false */, XMP_Uns32 extraSpace /* = 0 */ )
{
	bool appendedIFDs[kTIFF_KnownIFDCount];
	XMP_Uns32 newIFDOffsets[kTIFF_KnownIFDCount];
	XMP_Uns32 appendedOrigin = ((this->tiffLength + 1) & 0xFFFFFFFEUL);	// Start at an even offset.
	XMP_Uns32 appendedLength = DetermineAppendInfo ( appendedOrigin, appendedIFDs, newIFDOffsets, appendAll );

	// Allocate the new block of memory for the full stream. Copy the original stream. Write the
	// modified IFDs and values. Finally rebuild the internal IFD info and tag map.

	XMP_Uns32 newLength = appendedOrigin + appendedLength;
	XMP_Uns8* newStream = (XMP_Uns8*) malloc ( newLength + extraSpace );
	if ( newStream == 0 ) XMP_Throw ( "Out of memory", kXMPErr_NoMemory );

	memcpy ( newStream, this->memStream, this->tiffLength );	// AUDIT: Safe, malloc'ed newLength bytes above.
	if ( this->tiffLength < appendedOrigin ) {
		XMP_Assert ( appendedOrigin == (this->tiffLength + 1) );
		newStream[this->tiffLength] = 0;	// Clear the pad byte.
	}

	try {	// We might get exceptions from the next part and must delete newStream on the way out.

		// Write the modified IFDs and values. Rewrite the full IFD from scratch to make sure the
		// tags are now unique and sorted. Copy large changed values to their appropriate location.

		XMP_Uns32 appendedOffset = appendedOrigin;

		for ( int ifd = 0; ifd < kTIFF_KnownIFDCount; ++ifd ) {

			InternalIFDInfo& ifdInfo ( this->containedIFDs[ifd] );
			size_t tagCount = ifdInfo.tagMap.size();

			if ( ! (appendAll | ifdInfo.changed) ) continue;
			if ( tagCount == 0 ) continue;

			XMP_Uns8* ifdPtr = newStream + newIFDOffsets[ifd];

			if ( appendedIFDs[ifd] ) {
				XMP_Assert ( newIFDOffsets[ifd] == appendedOffset );
				appendedOffset += (XMP_Uns32)( 6 + (12 * tagCount) );
			}

			this->PutUns16 ( (XMP_Uns16)tagCount, ifdPtr );
			ifdPtr += 2;

			InternalTagMap::iterator tagPos = ifdInfo.tagMap.begin();
			InternalTagMap::iterator tagEnd = ifdInfo.tagMap.end();

			for ( ; tagPos != tagEnd; ++tagPos ) {

				InternalTagInfo & currTag ( tagPos->second );

				this->PutUns16 ( currTag.id, ifdPtr );
				ifdPtr += 2;
				this->PutUns16 ( currTag.type, ifdPtr );
				ifdPtr += 2;
				this->PutUns32 ( currTag.count, ifdPtr );
				ifdPtr += 4;

				PutUns32AsIs ( currTag.smallValue, ifdPtr );

				if ( (appendAll | currTag.changed) && (currTag.dataLen > 4) ) {

					XMP_Uns32 valueOffset = this->GetUns32 ( &currTag.smallValue );

					if ( (currTag.dataLen <= currTag.origDataLen) && (! appendAll) ) {
						XMP_Assert ( valueOffset == currTag.origDataOffset );
					} else {
						XMP_Assert ( valueOffset == appendedOffset );
						appendedOffset += ((currTag.dataLen + 1) & 0xFFFFFFFEUL);
					}

					XMP_Assert ( valueOffset <= newLength );	// Provably true, valueOffset is in the old span, newLength is the new bigger span.
					if ( currTag.dataLen > (newLength - valueOffset) ) XMP_Throw ( "Buffer overrun", kXMPErr_InternalFailure );
					memcpy ( (newStream + valueOffset), currTag.dataPtr, currTag.dataLen );	// AUDIT: Protected by the above check.
					if ( (currTag.dataLen & 1) != 0 ) newStream[valueOffset+currTag.dataLen] = 0;

				}

				ifdPtr += 4;

			}

			this->PutUns32 ( ifdInfo.origNextIFD, ifdPtr );
			ifdPtr += 4;

		}

		XMP_Assert ( appendedOffset == newLength );

		// Back fill the offsets for the primary and thumnbail IFDs, if they are now appended.

		if ( appendedIFDs[kTIFF_PrimaryIFD] ) {
			this->PutUns32 ( newIFDOffsets[kTIFF_PrimaryIFD], (newStream + 4) );
		}

		if ( appendedIFDs[kTIFF_TNailIFD] ) {
			size_t primaryTagCount = this->containedIFDs[kTIFF_PrimaryIFD].tagMap.size();
			if ( primaryTagCount > 0 ) {
				XMP_Uns32 tnailLinkOffset = newIFDOffsets[kTIFF_PrimaryIFD] + 2 + (12 * primaryTagCount);
				this->PutUns32 ( newIFDOffsets[kTIFF_TNailIFD], (newStream + tnailLinkOffset) );
			}
		}

	} catch ( ... ) {

		free ( newStream );
		throw;

	}

	*newStream_out = newStream;
	*newLength_out = newLength;

}	// TIFF_FileWriter::UpdateMemByAppend

// =================================================================================================
// TIFF_FileWriter::UpdateMemByRewrite
// ===================================
//
// Normally we update TIFF in a conservative "by-append" manner. Changes are written in-place where
// they fit, anything requiring growth is appended to the end and the old space is abandoned. The
// end for memory-based TIFF is the end of the data block, the end for file-based TIFF is the end of
// the file. This update-by-append model has the advantage of not perturbing any hidden offsets, a
// common feature of proprietary MakerNotes.
//
// The condenseStream parameter can be used to rewrite the full stream instead of appending. This
// will discard any MakerNote tag and risks breaking offsets that are hidden. This can be necessary
// though to try to make the TIFF fit in a JPEG file.
//
// We don't do most of the actual rewrite here. We set things up so that UpdateMemByAppend can be
// called to append onto a bare TIFF header. Additional hidden offsets are then handled here.
//
// The hidden offsets for the Exif, GPS, and Interoperability IFDs (tags 34665, 34853, and 40965)
// are handled by the code in DetermineAppendInfo, which is called from UpdateMemByAppend, which is
// called from here.
//
// These other tags are recognized as being hidden offsets when composing a condensed stream:
//    273 - StripOffsets, lengths in tag 279
//    288 - FreeOffsets, lengths in tag 289
//    324 - TileOffsets, lengths in tag 325
//    330 - SubIFDs, lengths within the IFDs (Plus subIFD values and possible chaining!)
//    513 - JPEGInterchangeFormat, length in tag 514
//    519 - JPEGQTables, each table is 64 bytes
//    520 - JPEGDCTables, lengths ???
//    521 - JPEGACTables, lengths ???
//
// Some of these will handled and kept, some will be thrown out, some will cause the rewrite to fail.
// At this time only the JPEG thumbnail tags, 513 and 514, contain hidden data that is kept. The
// final stream layout is whatever UpdateMemByAppend does for the visible content, followed by the
// hidden offset data. The Exif, GPS, and Interoperability IFDs are visible to UpdateMemByAppend.

// ! So far, a memory-based TIFF rewrite would only be done for the Exif portion of a JPEG file.
// ! In which case we're probably OK to handle JPEGInterchangeFormat (used for compressed thumbnails)
// ! and complain about any of the other hidden offset tags.

// tag	count	type

// 273		n	short or long
// 279		n	short or long
// 288		n	long
// 289		n	long
// 324		n	long
// 325		n	short or long

// 330		n	long

// 513		1	long
// 514		1	long

// 519		n	long
// 520		n	long
// 521		n	long

static XMP_Uns16 kNoGoTags[] =
	{
		kTIFF_StripOffsets,		// 273	*** Should be handled?
		kTIFF_StripByteCounts,	// 279	*** Should be handled?
		kTIFF_FreeOffsets,		// 288	*** Should be handled?
		kTIFF_FreeByteCounts,	// 289	*** Should be handled?
		kTIFF_TileOffsets,		// 324	*** Should be handled?
		kTIFF_TileByteCounts,	// 325	*** Should be handled?
		kTIFF_SubIFDs,			// 330	*** Should be handled?
		kTIFF_JPEGQTables,		// 519
		kTIFF_JPEGDCTables,		// 520
		kTIFF_JPEGACTables,		// 521
		0xFFFF	// Must be last as a sentinel.
	};

static XMP_Uns16 kBanishedTags[] =
	{
		kTIFF_MakerNote,	// *** Should someday support MakerNoteSafety.
		0xFFFF	// Must be last as a sentinel.
	};

struct SimpleHiddenContentInfo {
	XMP_Uns8  ifd;
	XMP_Uns16 offsetTag, lengthTag;
};

struct SimpleHiddenContentLocations {
	XMP_Uns32 length, oldOffset, newOffset;
	SimpleHiddenContentLocations() : length(0), oldOffset(0), newOffset(0) {};
};

enum { kSimpleHiddenContentCount = 1 };

static const SimpleHiddenContentInfo kSimpleHiddenContentInfo [kSimpleHiddenContentCount] =
	{
		{ kTIFF_TNailIFD, kTIFF_JPEGInterchangeFormat, kTIFF_JPEGInterchangeFormatLength }
	};

// -------------------------------------------------------------------------------------------------

void TIFF_FileWriter::UpdateMemByRewrite ( XMP_Uns8** newStream_out, XMP_Uns32* newLength_out )
{
	const InternalTagInfo* tagInfo;

	// Check for tags that we don't tolerate because they have data we can't (or refuse to) find.

	for ( XMP_Uns8 ifd = 0; ifd < kTIFF_KnownIFDCount; ++ifd ) {
		for ( int i = 0; kNoGoTags[i] != 0xFFFF; ++i ) {
			tagInfo = this->FindTagInIFD ( ifd, kNoGoTags[i] );
			if ( tagInfo != 0 ) XMP_Throw ( "Tag not tolerated for TIFF rewrite", kXMPErr_Unimplemented );
		}
	}

	// Delete unwanted tags.

	for ( XMP_Uns8 ifd = 0; ifd < kTIFF_KnownIFDCount; ++ifd ) {
		for ( int i = 0; kBanishedTags[i] != 0xFFFF; ++i ) {
			this->DeleteTag ( ifd, kBanishedTags[i] );
		}
	}

	// Determine the offsets and additional size for the hidden offset content. Set the offset tags
	// to the new offset so that UpdateMemByAppend writes the new offsets.

	XMP_Uns32 hiddenContentLength = 0;
	XMP_Uns32 hiddenContentOrigin = this->DetermineVisibleLength();

	SimpleHiddenContentLocations hiddenLocations [kSimpleHiddenContentCount];

	for ( int i = 0; i < kSimpleHiddenContentCount; ++i ) {

		const SimpleHiddenContentInfo & hiddenInfo ( kSimpleHiddenContentInfo[i] );

		bool haveLength = this->GetTag_Integer ( hiddenInfo.ifd, hiddenInfo.lengthTag, &hiddenLocations[i].length );
		bool haveOffset = this->GetTag_Integer ( hiddenInfo.ifd, hiddenInfo.offsetTag, &hiddenLocations[i].oldOffset );
		if ( haveLength != haveOffset ) XMP_Throw ( "Unpaired simple hidden content tag", kXMPErr_BadTIFF );
		if ( (! haveLength) || (hiddenLocations[i].length == 0) ) continue;

		hiddenLocations[i].newOffset = hiddenContentOrigin + hiddenContentLength;
		this->SetTag_Long ( hiddenInfo.ifd, hiddenInfo.offsetTag, hiddenLocations[i].newOffset );
		hiddenContentLength += ((hiddenLocations[i].length + 1) & 0xFFFFFFFE);	// ! Round up for even offsets.

	}

	// Save any old memory stream for the content behind hidden offsets. Setup a bare TIFF header.

	XMP_Uns8* oldStream = this->memStream;
	bool ownedOldStream = this->ownedStream;

	XMP_Uns8 bareTIFF [8];
	if ( this->bigEndian ) {
		bareTIFF[0] = 0x4D; bareTIFF[1] = 0x4D; bareTIFF[2] = 0x00; bareTIFF[3] = 0x2A;
	} else {
		bareTIFF[0] = 0x49; bareTIFF[1] = 0x49; bareTIFF[2] = 0x2A; bareTIFF[3] = 0x00;
	}
	*((XMP_Uns32*)&bareTIFF[4]) = 0;

	this->memStream = &bareTIFF[0];
	this->tiffLength = sizeof ( bareTIFF );
	this->ownedStream = false;

	// Call UpdateMemByAppend to write the new stream, telling it to append everything.

	this->UpdateMemByAppend ( newStream_out, newLength_out, true, hiddenContentLength );

	// Copy the hidden content and update the output stream length;

	XMP_Assert ( *newLength_out == hiddenContentOrigin );
	*newLength_out += hiddenContentLength;

	for ( int i = 0; i < kSimpleHiddenContentCount; ++i ) {

		if ( hiddenLocations[i].length == 0 ) continue;

		XMP_Uns8* srcPtr  = oldStream + hiddenLocations[i].oldOffset;
		XMP_Uns8* destPtr = *newStream_out + hiddenLocations[i].newOffset;
		memcpy ( destPtr, srcPtr, hiddenLocations[i].length );	// AUDIT: Safe copy, not user data, computed length.

	}
	
	// Delete the old stream if appropriate.
	
	if ( ownedOldStream ) delete ( oldStream );

}	// TIFF_FileWriter::UpdateMemByRewrite

// =================================================================================================
// TIFF_FileWriter::UpdateMemoryStream
// ===================================
//
// Normally we update TIFF in a conservative "by-append" manner. Changes are written in-place where
// they fit, anything requiring growth is appended to the end and the old space is abandoned. The
// end for memory-based TIFF is the end of the data block, the end for file-based TIFF is the end of
// the file. This update-by-append model has the advantage of not perturbing any hidden offsets, a
// common feature of proprietary MakerNotes.
//
// The condenseStream parameter can be used to rewrite the full stream instead of appending. This
// will discard any MakerNote tags and risks breaking offsets that are hidden. This can be necessary
// though to try to make the TIFF fit in a JPEG file.

XMP_Uns32 TIFF_FileWriter::UpdateMemoryStream ( void** dataPtr, bool condenseStream /* = false */ )
{
	if ( this->fileParsed ) XMP_Throw ( "Not memory based", kXMPErr_EnforceFailure );

	this->changed |= condenseStream;	// Make sure a compaction request takes effect.
	if ( ! this->changed ) {
		if ( dataPtr != 0 ) *dataPtr = this->memStream;
		return this->tiffLength;
	}

	this->PreflightIFDLinkage();

	bool nowEmpty = true;
	for ( size_t i = 0; i < kTIFF_KnownIFDCount; ++i ) {
		if ( ! this->containedIFDs[i].tagMap.empty() ) {
			nowEmpty = false;
			break;
		}
	}

	XMP_Uns8* newStream = 0;
	XMP_Uns32 newLength = 0;

	if ( nowEmpty ) {

		this->DeleteExistingInfo();	// Prepare for an empty reparse.

	} else {

		if ( this->tiffLength == 0 ) {	// ! An empty parse does set this->memParsed.
			condenseStream = true;		// Makes "conjured" TIFF take the full rewrite path.
		}

		if ( condenseStream ) {
			this->UpdateMemByRewrite ( &newStream, &newLength );
		} else {
			this->UpdateMemByAppend ( &newStream, &newLength );
		}

	}

	// Parse the revised stream. This is the cleanest way to rebuild the tag map.

	this->ParseMemoryStream ( newStream, newLength, kDoNotCopyData );
	XMP_Assert ( this->tiffLength == newLength );
	this->ownedStream = (newLength > 0);	// ! We really do own the new stream, if not empty.

	if ( dataPtr != 0 ) *dataPtr = this->memStream;
	return newLength;

}	// TIFF_FileWriter::UpdateMemoryStream

// =================================================================================================
// TIFF_FileWriter::UpdateFileStream
// =================================
//
// Updating a file stream is done in the same general manner as updating a memory stream, the intro
// comments for UpdateMemoryStream largely apply. The file update happens in 3 phases:
//   1. Determine which IFDs will be appended, and the new offsets for the appended IFDs and tags.
//   2. Do the in-place update for the IFDs and tag values that fit.
//   3. Append the IFDs and tag values that grow.
//
// The file being updated must match the file that was previously parsed. Offsets and lengths saved
// when parsing are used to decide if something can be updated in-place or must be appended.

// *** The general linked structure of TIFF makes it very difficult to process the file in a single
// *** sequential pass. This implementation uses a simple seek/write model for the in-place updates.
// *** In the future we might want to consider creating a map of what gets updated, allowing use of
// *** a possibly more efficient buffered model.

// ** Someday we might want to use the FreeOffsets and FreeByteCounts tags to track free space.
// ** Probably not a huge win in practice though, and the TIFF spec says they are not recommended
// ** for general interchange use.

#ifndef Trace_UpdateFileStream
	#define Trace_UpdateFileStream 0
#endif

void TIFF_FileWriter::UpdateFileStream ( XMP_IO* fileRef, XMP_ProgressTracker* progressTracker )
{
	if ( this->memParsed ) XMP_Throw ( "Not file based", kXMPErr_EnforceFailure );
	if ( ! this->changed ) return;

	XMP_Int64 origDataLength = fileRef->Length();
	if ( (origDataLength >> 32) != 0 ) XMP_Throw ( "TIFF files can't exceed 4GB", kXMPErr_BadTIFF );

	bool appendedIFDs[kTIFF_KnownIFDCount];
	XMP_Uns32 newIFDOffsets[kTIFF_KnownIFDCount];

	#if Trace_UpdateFileStream
		printf ( "\nStarting update of TIFF file stream\n" );
	#endif

	XMP_Uns32 appendedOrigin = (XMP_Uns32)origDataLength;
	if ( (appendedOrigin & 1) != 0 ) {
		++appendedOrigin;	// Start at an even offset.
		fileRef->Seek ( 0, kXMP_SeekFromEnd  );
		fileRef->Write ( "\0", 1 );
	}

	this->PreflightIFDLinkage();

	XMP_Uns32 appendedLength = DetermineAppendInfo ( appendedOrigin, appendedIFDs, newIFDOffsets );
	if ( appendedLength > (0xFFFFFFFFUL - appendedOrigin) ) XMP_Throw ( "TIFF files can't exceed 4GB", kXMPErr_BadTIFF );

	if ( progressTracker != 0 ) {

		float filesize=0;

		for ( int ifd = 0; ifd < kTIFF_KnownIFDCount; ++ifd ) {

			InternalIFDInfo & thisIFD = this->containedIFDs[ifd];
			if ( ! thisIFD.changed ) continue;

			filesize += (thisIFD.tagMap.size() * sizeof(RawIFDEntry) + 6);

			InternalTagMap::iterator tagPos;
			InternalTagMap::iterator tagEnd = thisIFD.tagMap.end();

			for ( tagPos = thisIFD.tagMap.begin(); tagPos != tagEnd; ++tagPos ) {
				InternalTagInfo & thisTag = tagPos->second;
				if ( (! thisTag.changed) || (thisTag.dataLen <= 4)  ) continue;
				filesize += (thisTag.dataLen) ;
			}
		}

		if ( appendedIFDs[kTIFF_PrimaryIFD] ) filesize += 4;
		XMP_Assert ( progressTracker->WorkInProgress() );
		progressTracker->AddTotalWork ( filesize );

	}

	// Do the in-place update for the IFDs and tag values that fit. This part does separate seeks
	// and writes for the IFDs and values. Things to be updated can be anywhere in the file.

	// *** This might benefit from a map of the in-place updates. This would allow use of a possibly
	// *** more efficient sequential I/O model. Could even incorporate the safe update file copying.

	for ( int ifd = 0; ifd < kTIFF_KnownIFDCount; ++ifd ) {

		InternalIFDInfo & thisIFD = this->containedIFDs[ifd];
		if ( ! thisIFD.changed ) continue;

		// In order to get a little bit of locality, write the IFD first then the changed tags that
		// have large values and fit in-place.

		if ( ! appendedIFDs[ifd] ) {
			#if Trace_UpdateFileStream
				printf ( "  Updating IFD %d in-place at offset %d (0x%X)\n", ifd, thisIFD.origIFDOffset, thisIFD.origIFDOffset );
			#endif
			fileRef->Seek ( thisIFD.origIFDOffset, kXMP_SeekFromStart  );
			this->WriteFileIFD ( fileRef, thisIFD );
		}

		InternalTagMap::iterator tagPos;
		InternalTagMap::iterator tagEnd = thisIFD.tagMap.end();

		for ( tagPos = thisIFD.tagMap.begin(); tagPos != tagEnd; ++tagPos ) {
			InternalTagInfo & thisTag = tagPos->second;
			if ( (! thisTag.changed) || (thisTag.dataLen <= 4) || (thisTag.dataLen > thisTag.origDataLen) ) continue;
			#if Trace_UpdateFileStream
				printf ( "    Updating tag %d in IFD %d in-place at offset %d (0x%X)\n", thisTag.id, ifd, thisTag.origDataOffset, thisTag.origDataOffset );
			#endif
			fileRef->Seek ( thisTag.origDataOffset, kXMP_SeekFromStart  );
			fileRef->Write ( thisTag.dataPtr, thisTag.dataLen );
		}

	}

	// Append the IFDs and tag values that grow.

	XMP_Int64 fileEnd = fileRef->Seek ( 0, kXMP_SeekFromEnd  );
	XMP_Assert ( fileEnd == appendedOrigin );

	for ( int ifd = 0; ifd < kTIFF_KnownIFDCount; ++ifd ) {

		InternalIFDInfo & thisIFD = this->containedIFDs[ifd];
		if ( ! thisIFD.changed ) continue;

		if ( appendedIFDs[ifd] ) {
			#if Trace_UpdateFileStream
				printf ( "  Updating IFD %d by append at offset %d (0x%X)\n", ifd, newIFDOffsets[ifd], newIFDOffsets[ifd] );
			#endif
			XMP_Assert ( newIFDOffsets[ifd] == fileRef->Length() );
			this->WriteFileIFD ( fileRef, thisIFD );
		}

		InternalTagMap::iterator tagPos;
		InternalTagMap::iterator tagEnd = thisIFD.tagMap.end();

		for ( tagPos = thisIFD.tagMap.begin(); tagPos != tagEnd; ++tagPos ) {
			InternalTagInfo & thisTag = tagPos->second;
			if ( (! thisTag.changed) || (thisTag.dataLen <= 4) || (thisTag.dataLen <= thisTag.origDataLen) ) continue;
			#if Trace_UpdateFileStream
				XMP_Uns32 newOffset = this->GetUns32(&thisTag.origDataOffset);
				printf ( "    Updating tag %d in IFD %d by append at offset %d (0x%X)\n", thisTag.id, ifd, newOffset, newOffset );
			#endif
			XMP_Assert ( this->GetUns32(&thisTag.smallValue) == fileRef->Length() );
			fileRef->Write ( thisTag.dataPtr, thisTag.dataLen );
			if ( (thisTag.dataLen & 1) != 0 ) fileRef->Write ( "\0", 1 );
		}

	}

	// Back-fill the offset for the primary IFD, if it is now appended.

	XMP_Uns32 newOffset;

	if ( appendedIFDs[kTIFF_PrimaryIFD] ) {
		this->PutUns32 ( newIFDOffsets[kTIFF_PrimaryIFD], &newOffset );
		#if TraceUpdateFileStream
			printf ( "  Back-filling offset of primary IFD, pointing to %d (0x%X)\n", newOffset, newOffset );
		#endif
		fileRef->Seek ( 4, kXMP_SeekFromStart  );
		fileRef->Write ( &newOffset, 4 );
	}

	// Reset the changed flags and original length/offset values. This simulates a reparse of the
	// updated file.

	for ( int ifd = 0; ifd < kTIFF_KnownIFDCount; ++ifd ) {

		InternalIFDInfo & thisIFD = this->containedIFDs[ifd];
		if ( ! thisIFD.changed ) continue;

		thisIFD.changed = false;
		thisIFD.origCount  = (XMP_Uns16)( thisIFD.tagMap.size() );
		thisIFD.origIFDOffset = newIFDOffsets[ifd];

		InternalTagMap::iterator tagPos;
		InternalTagMap::iterator tagEnd = thisIFD.tagMap.end();

		for ( tagPos = thisIFD.tagMap.begin(); tagPos != tagEnd; ++tagPos ) {
			InternalTagInfo & thisTag = tagPos->second;
			if ( ! thisTag.changed ) continue;
			thisTag.changed = false;
			thisTag.origDataLen = thisTag.dataLen;
			if ( thisTag.origDataLen > 4 ) thisTag.origDataOffset = this->GetUns32 ( &thisTag.smallValue );
		}

	}

	this->tiffLength = (XMP_Uns32) fileRef->Length();
	fileRef->Seek ( 0, kXMP_SeekFromEnd  );	// Can't hurt.

	#if Trace_UpdateFileStream
		printf ( "\nFinished update of TIFF file stream\n" );
	#endif

}	// TIFF_FileWriter::UpdateFileStream

// =================================================================================================
// TIFF_FileWriter::WriteFileIFD
// =============================

void TIFF_FileWriter::WriteFileIFD ( XMP_IO* fileRef, InternalIFDInfo & thisIFD )
{
	XMP_Uns16 tagCount;
	this->PutUns16 ( (XMP_Uns16)thisIFD.tagMap.size(), &tagCount );
	fileRef->Write ( &tagCount, 2 );

	InternalTagMap::iterator tagPos;
	InternalTagMap::iterator tagEnd = thisIFD.tagMap.end();

	for ( tagPos = thisIFD.tagMap.begin(); tagPos != tagEnd; ++tagPos ) {

		InternalTagInfo & thisTag = tagPos->second;
		RawIFDEntry ifdEntry;

		this->PutUns16 ( thisTag.id, &ifdEntry.id );
		this->PutUns16 ( thisTag.type, &ifdEntry.type );
		this->PutUns32 ( thisTag.count, &ifdEntry.count );
		ifdEntry.dataOrOffset = thisTag.smallValue;	// ! Already in stream endianness.

		fileRef->Write ( &ifdEntry, sizeof(ifdEntry) );
		XMP_Assert ( sizeof(ifdEntry) == 12 );

	}

	XMP_Uns32 nextIFD;
	this->PutUns32 ( thisIFD.origNextIFD, &nextIFD );
	fileRef->Write ( &nextIFD, 4 );

}	// TIFF_FileWriter::WriteFileIFD
