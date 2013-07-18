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
#include "source/EndianUtils.hpp"
#include "source/XIO.hpp"

#include "source/UnicodeConversions.hpp"

// =================================================================================================
/// \file TIFF_Support.cpp
/// \brief Manager for parsing and serializing TIFF streams.
///
// =================================================================================================

// =================================================================================================
// TIFF_Manager::TIFF_Manager
// ==========================

static bool sFirstCTor = true;

TIFF_Manager::TIFF_Manager()
	: bigEndian(false), nativeEndian(false), errorCallbackPtr( NULL ),
	  GetUns16(0), GetUns32(0), GetFloat(0), GetDouble(0),
	  PutUns16(0), PutUns32(0), PutFloat(0), PutDouble(0)
{

	if ( sFirstCTor ) {
		sFirstCTor = false;
		#if XMP_DebugBuild
			for ( int ifd = 0; ifd < kTIFF_KnownIFDCount; ++ifd ) {	// Make sure the known tag arrays are sorted.
				for ( const XMP_Uns16* idPtr = sKnownTags[ifd]; *idPtr != 0xFFFF; ++idPtr ) {
					XMP_Assert ( *idPtr < *(idPtr+1) );
				}
			}
		#endif
	}

}	// TIFF_Manager::TIFF_Manager

// =================================================================================================
// TIFF_Manager::CheckTIFFHeader
// =============================
//
// Checks the 4 byte TIFF prefix for validity and endianness. Sets the endian flags and the Get
// function pointers. Returns the 0th IFD offset.

XMP_Uns32 TIFF_Manager::CheckTIFFHeader ( const XMP_Uns8* tiffPtr, XMP_Uns32 length )
{
	if ( length < kEmptyTIFFLength ) XMP_Throw ( "The TIFF is too small", kXMPErr_BadTIFF );

	XMP_Uns32 tiffPrefix = (tiffPtr[0] << 24) | (tiffPtr[1] << 16) | (tiffPtr[2] << 8) | (tiffPtr[3]);

	if ( tiffPrefix == kBigEndianPrefix ) {
		this->bigEndian = true;
	} else if ( tiffPrefix == kLittleEndianPrefix ) {
		this->bigEndian = false;
	} else {
		XMP_Throw ( "Unrecognized TIFF prefix", kXMPErr_BadTIFF );
	}

	this->nativeEndian = (this->bigEndian == kBigEndianHost);

	if ( this->bigEndian ) {

		this->GetUns16  = GetUns16BE;
		this->GetUns32  = GetUns32BE;
		this->GetFloat  = GetFloatBE;
		this->GetDouble = GetDoubleBE;

		this->PutUns16  = PutUns16BE;
		this->PutUns32  = PutUns32BE;
		this->PutFloat  = PutFloatBE;
		this->PutDouble = PutDoubleBE;

	} else {

		this->GetUns16  = GetUns16LE;
		this->GetUns32  = GetUns32LE;
		this->GetFloat  = GetFloatLE;
		this->GetDouble = GetDoubleLE;

		this->PutUns16  = PutUns16LE;
		this->PutUns32  = PutUns32LE;
		this->PutFloat  = PutFloatLE;
		this->PutDouble = PutDoubleLE;

	}

	XMP_Uns32 mainIFDOffset = this->GetUns32 ( tiffPtr+4 );	// ! Do this after setting the Get/Put procs!

	if ( mainIFDOffset != 0 ) {	// Tolerate empty TIFF even though formally invalid.
		if ( (length < (kEmptyTIFFLength + kEmptyIFDLength)) ||
			 (mainIFDOffset < kEmptyTIFFLength) || (mainIFDOffset > (length - kEmptyIFDLength)) ) {
			XMP_Throw ( "Invalid primary IFD offset", kXMPErr_BadTIFF );
		}
	}

	return mainIFDOffset;

}	// TIFF_Manager::CheckTIFFHeader

// =================================================================================================
// TIFF_Manager::SetTag_Integer
// ============================

void TIFF_Manager::SetTag_Integer ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns32 data32 )
{

	if ( data32 > 0xFFFF ) {
		this->SetTag ( ifd, id, kTIFF_LongType, 1, &data32 );
	} else {
		XMP_Uns16 data16 = (XMP_Uns16)data32;
		this->SetTag ( ifd, id, kTIFF_ShortType, 1, &data16 );
	}

}	// TIFF_Manager::SetTag_Integer

// =================================================================================================
// TIFF_Manager::SetTag_Byte
// =========================

void TIFF_Manager::SetTag_Byte ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns8 data )
{

	this->SetTag ( ifd, id, kTIFF_ByteType, 1, &data );

}	// TIFF_Manager::SetTag_Byte

// =================================================================================================
// TIFF_Manager::SetTag_SByte
// ==========================

void TIFF_Manager::SetTag_SByte ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Int8 data )
{

	this->SetTag ( ifd, id, kTIFF_SByteType, 1, &data );

}	// TIFF_Manager::SetTag_SByte

// =================================================================================================
// TIFF_Manager::SetTag_Short
// ==========================

void TIFF_Manager::SetTag_Short ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns16 clientData )
{
	XMP_Uns16 streamData;

	this->PutUns16 ( clientData, &streamData );
	this->SetTag ( ifd, id, kTIFF_ShortType, 1, &streamData );

}	// TIFF_Manager::SetTag_Short

// =================================================================================================
// TIFF_Manager::SetTag_SShort
// ===========================

 void TIFF_Manager::SetTag_SShort ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Int16 clientData )
{
	XMP_Int16 streamData;

	this->PutUns16 ( (XMP_Uns16)clientData, &streamData );
	this->SetTag ( ifd, id, kTIFF_SShortType, 1, &streamData );

}	// TIFF_Manager::SetTag_SShort

// =================================================================================================
// TIFF_Manager::SetTag_Long
// =========================

 void TIFF_Manager::SetTag_Long ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns32 clientData )
{
	XMP_Uns32 streamData;

	this->PutUns32 ( clientData, &streamData );
	this->SetTag ( ifd, id, kTIFF_LongType, 1, &streamData );

}	// TIFF_Manager::SetTag_Long

// =================================================================================================
// TIFF_Manager::SetTag_SLong
// ==========================

 void TIFF_Manager::SetTag_SLong ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Int32 clientData )
{
	XMP_Int32 streamData;

	this->PutUns32 ( (XMP_Uns32)clientData, &streamData );
	this->SetTag ( ifd, id, kTIFF_SLongType, 1, &streamData );

}	// TIFF_Manager::SetTag_SLong

// =================================================================================================
// TIFF_Manager::SetTag_Rational
// =============================

struct StreamRational { XMP_Uns32 num, denom; };

 void TIFF_Manager::SetTag_Rational ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Uns32 clientNum, XMP_Uns32 clientDenom )
{
	StreamRational streamData;

	this->PutUns32 ( clientNum, &streamData.num );
	this->PutUns32 ( clientDenom, &streamData.denom );
	this->SetTag ( ifd, id, kTIFF_RationalType, 1, &streamData );

}	// TIFF_Manager::SetTag_Rational

// =================================================================================================
// TIFF_Manager::SetTag_SRational
// ==============================

 void TIFF_Manager::SetTag_SRational ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_Int32 clientNum, XMP_Int32 clientDenom )
{
	StreamRational streamData;

	this->PutUns32 ( (XMP_Uns32)clientNum, &streamData.num );
	this->PutUns32 ( (XMP_Uns32)clientDenom, &streamData.denom );
	this->SetTag ( ifd, id, kTIFF_SRationalType, 1, &streamData );

}	// TIFF_Manager::SetTag_SRational

// =================================================================================================
// TIFF_Manager::SetTag_Float
// ==========================

 void TIFF_Manager::SetTag_Float ( XMP_Uns8 ifd, XMP_Uns16 id, float clientData )
{
	float streamData;

	this->PutFloat ( clientData, &streamData );
	this->SetTag ( ifd, id, kTIFF_FloatType, 1, &streamData );

}	// TIFF_Manager::SetTag_Float

// =================================================================================================
// TIFF_Manager::SetTag_Double
// ===========================

 void TIFF_Manager::SetTag_Double ( XMP_Uns8 ifd, XMP_Uns16 id, double clientData )
{
	double streamData;

	this->PutDouble ( clientData, &streamData );
	this->SetTag ( ifd, id, kTIFF_DoubleType, 1, &streamData );

}	// TIFF_Manager::SetTag_Double

// =================================================================================================
// TIFF_Manager::SetTag_ASCII
// ===========================

void TIFF_Manager::SetTag_ASCII ( XMP_Uns8 ifd, XMP_Uns16 id, XMP_StringPtr data )
{

	this->SetTag ( ifd, id, kTIFF_ASCIIType, (XMP_Uns32)(strlen(data) + 1), data );	// ! Include trailing nul.

}	// TIFF_Manager::SetTag_ASCII

// =================================================================================================
// UTF16_to_UTF8
// =============

static void UTF16_to_UTF8 ( const UTF16Unit * utf16Ptr, size_t utf16Len, bool bigEndian, std::string * outStr )
{
	UTF16_to_UTF8_Proc ToUTF8 = 0;
	if ( bigEndian ) {
		ToUTF8 = UTF16BE_to_UTF8;
	} else {
		ToUTF8 = UTF16LE_to_UTF8;
	}

	UTF8Unit buffer [1000];
	size_t inCount, outCount;

	outStr->erase();
	outStr->reserve ( utf16Len * 2 );	// As good a guess as any.

	while ( utf16Len > 0 ) {
		ToUTF8 ( utf16Ptr, utf16Len, buffer, sizeof(buffer), &inCount, &outCount );
		outStr->append ( (XMP_StringPtr)buffer, outCount );
		utf16Ptr += inCount;
		utf16Len -= inCount;
	}

}	// UTF16_to_UTF8

// =================================================================================================
// TIFF_Manager::DecodeString
// ==========================
//
// Convert an explicitly encoded string to UTF-8. The input must be encoded according to table 6 of
// the Exif 2.2 specification. The input pointer is to the start of the 8 byte header, the length is
// the full length. In other words, the pointer and length from the IFD entry. Returns true if the
// encoding is supported and the conversion succeeds.

// *** JIS encoding is not supported yet. Need a static mapping table from JIS X 208-1990 to Unicode.

bool TIFF_Manager::DecodeString ( const void * encodedPtr, size_t encodedLen, std::string* utf8Str ) const
{
	utf8Str->erase();
	if ( encodedLen < 8 ) return false;	// Need to have at least the 8 byte header.
	
	XMP_StringPtr typePtr  = (XMP_StringPtr)encodedPtr;
	XMP_StringPtr valuePtr = typePtr + 8;
	size_t valueLen = encodedLen - 8;

	if ( *typePtr == 'A' ) {

		utf8Str->assign ( valuePtr, valueLen );
		return true;

	} else if ( *typePtr == 'U' ) {

		try {

			const UTF16Unit * utf16Ptr = (const UTF16Unit *) valuePtr;
			size_t utf16Len = valueLen >> 1;	// The number of UTF-16 storage units, not bytes.
			if ( utf16Len == 0 ) return false;
			bool isBigEndian = this->bigEndian;	// Default to stream endian, unless there is a BOM ...
			if ( (*utf16Ptr == 0xFEFF) || (*utf16Ptr == 0xFFFE) ) {	// Check for an explicit BOM
				isBigEndian = (*((XMP_Uns8*)utf16Ptr) == 0xFE);
				utf16Ptr += 1;	// Don't translate the BOM.
				utf16Len -= 1;
				if ( utf16Len == 0 ) return false;
			}
			UTF16_to_UTF8 ( utf16Ptr, utf16Len, isBigEndian, utf8Str );
			return true;

		} catch ( ... ) {
			return false; // Ignore the tag if there are conversion errors.
		}

	} else if ( *typePtr == 'J' ) {

		return false;	// ! Ignore JIS for now.

	}

	return false;	// ! Ignore all other encodings.

}	// TIFF_Manager::DecodeString

// =================================================================================================
// UTF8_to_UTF16
// =============

static void UTF8_to_UTF16 ( const UTF8Unit * utf8Ptr, size_t utf8Len, bool bigEndian, std::string * outStr )
{
	UTF8_to_UTF16_Proc ToUTF16 = 0;
	if ( bigEndian ) {
		ToUTF16 = UTF8_to_UTF16BE;
	} else {
		ToUTF16 = UTF8_to_UTF16LE;
	}

	enum { kUTF16Len = 1000 };
	UTF16Unit buffer [kUTF16Len];
	size_t inCount, outCount;

	outStr->erase();
	outStr->reserve ( utf8Len * 2 );	// As good a guess as any.

	while ( utf8Len > 0 ) {
		ToUTF16 ( utf8Ptr, utf8Len, buffer, kUTF16Len, &inCount, &outCount );
		outStr->append ( (XMP_StringPtr)buffer, outCount*2 );
		utf8Ptr += inCount;
		utf8Len -= inCount;
	}

}	// UTF8_to_UTF16

XMP_Bool IsOffsetValid( XMP_Uns32 offset, XMP_Uns32 lowerBound, XMP_Uns32 upperBound )
{
	if ( (lowerBound <= offset) && (offset < upperBound) )
		return true;
	return false;
}

// =================================================================================================
// TIFF_Manager::EncodeString
// ==========================
//
// Convert a UTF-8 string to an explicitly encoded form according to table 6 of the Exif 2.2
// specification. Returns true if the encoding is supported and the conversion succeeds.

// *** JIS encoding is not supported yet. Need a static mapping table to JIS X 208-1990 from Unicode.

bool TIFF_Manager::EncodeString ( const std::string& utf8Str, XMP_Uns8 encoding, std::string* encodedStr )
{
	encodedStr -> erase();

	if ( encoding == kTIFF_EncodeASCII ) {

		encodedStr->assign ( "ASCII\0\0\0", 8 );
		XMP_Assert (encodedStr->size() == 8 );

		encodedStr->append ( utf8Str );	// ! Let the caller filter the value. (?)

		return true;

	} else if ( encoding == kTIFF_EncodeUnicode ) {

		encodedStr->assign ( "UNICODE\0", 8 );
		XMP_Assert (encodedStr->size() == 8 );

		try {
			std::string temp;
			UTF8_to_UTF16 ( (const UTF8Unit*)utf8Str.c_str(), utf8Str.size(), this->bigEndian, &temp );
			encodedStr->append ( temp );
			return true;
		} catch ( ... ) {
			return false; // Ignore the tag if there are conversion errors.
		}

	} else if ( encoding == kTIFF_EncodeJIS ) {

		XMP_Throw ( "Encoding to JIS is not implemented", kXMPErr_Unimplemented );

		// encodedStr->assign ( "JIS\0\0\0\0\0", 8 );
		// XMP_Assert (encodedStr->size() == 8 );

		// ...

		// return true;

	} else {

		XMP_Throw ( "Invalid TIFF string encoding", kXMPErr_BadParam );

	}

	return false;	// ! Should never get here.

}	// TIFF_Manager::EncodeString

void TIFF_Manager::NotifyClient( XMP_ErrorSeverity severity, XMP_Error & error )
{
	if (this->errorCallbackPtr)
		this->errorCallbackPtr->NotifyClient( severity, error );
	else {
		if ( severity != kXMPErrSev_Recoverable )
			throw error;
	}
}

// =================================================================================================
