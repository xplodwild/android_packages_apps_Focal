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

#include "XMPFiles/source/FileHandlers/FLV_Handler.hpp"

#include "source/XIO.hpp"
#include "third-party/zuid/interfaces/MD5.h"

using namespace std;

// =================================================================================================
/// \file FLV_Handler.cpp
/// \brief File format handler for FLV.
///
/// FLV is a fairly simple format, with a strong orientation to streaming use. It consists of a
/// small file header then a sequence of tags that can contain audio data, video data, or
/// ActionScript data. All integers in FLV are big endian.
///
/// For FLV version 1, the file header contains:
///
///   UI24 signature - the characters "FLV"
///   UI8 version - 1
///   UI8 flags - 0x01 = has video tags, 0x04 = has audio tags
///   UI32 length in bytes of file header
///
/// For FLV version 1, each tag begins with an 11 byte header:
///
///   UI8 tag type - 8 = audio tag, 9 = video tag, 18 = script data tag
///   UI24 content length in bytes
///   UI24 time - low order 3 bytes
///   UI8 time - high order byte
///   UI24 stream ID
///
/// This is followed by the tag's content, then a UI32 "back pointer" which is the header size plus
/// the content size. A UI32 zero is placed between the file header and the first tag as a
/// terminator for backward scans. The time in a tag header is the start of playback for that tag.
/// The tags must be in ascending time order. For a given time it is preferred that script data tags
/// precede audio and video tags.
///
/// For metadata purposes only the script data tags are of interest. Script data information becomes
/// accessible to ActionScript at the playback moment of the script data tag through a call to a
/// registered data handler. The content of a script data tag contains a string and an ActionScript
/// data value. The string is the name of the handler to be invoked, the data value is passed as an
/// ActionScript Object parameter to the handler.
///
/// The XMP is placed in a script data tag with the name "onXMPData". A variety of legacy metadata
/// is contained in a script data tag with the name "onMetaData". This contains only "internal"
/// information (like duration or width/height), nothing that is user or author editiable (like
/// title or description). Some of these legacy items are imported into the XMP, none are updated
/// from the XMP.
///
/// A script data tag's content is:
///
///   UI8 0x02
///   UI16 name length - includes nul terminator if present
///   UI8n object name - UTF-8, possibly with nul terminator
///   ...  object value - serialized ActionScript value (SCRIPTDATAVALUE in FLV spec)
///
/// The onXMPData and onMetaData values are both ECMA arrays. These have more in common with XMP
/// structs than arrays, the items have arbitrary string names. The serialized form is:
///
///   UI8 0x08
///   UI32 array length - need not be exact, an optimization hint
///   array items
///      UI16 name length - includes nul terminator if present
///      UI8n item name - UTF-8, possibly with nul terminator
///      ...  object value - serialized ActionScript value (SCRIPTDATAVALUE in FLV spec)
///   UI24 0x000009 - array terminator
///
/// The object names and array item names in sample files do not have a nul terminator. The policy
/// here is to treat them as optional when reading, and to omit them when writing.
///
/// The onXMPData array typically has one item named "liveXML". The value of this is a short or long
/// string as necessary:
///
///   UI8 type - 2 for a short string, 12 for a long string
///   UIx value length - UI16 for a short string, UI32 for a long string, includes nul terminator
///   UI8n value - UTF-8 with nul terminator
///
// =================================================================================================

static inline XMP_Uns32 GetUns24BE ( const void * addr )
{
	return (GetUns32BE(addr) >> 8);
}

static inline void PutUns24BE ( XMP_Uns32 value, void * addr )
{
	XMP_Uns8 * bytes = (XMP_Uns8*)addr;
	bytes[0] = (XMP_Uns8)(value >> 16);
	bytes[1] = (XMP_Uns8)(value >> 8);
	bytes[2] = (XMP_Uns8)(value);
}

// =================================================================================================
// FLV_CheckFormat
// ===============
//
// Check for "FLV" and 1 in the first 4 bytes, that the header length is at least 9, that the file
// size is at least as big as the header, and that the leading 0 back pointer is present if the file
// is bigger than the header.

#define kFLV1 0x464C5601UL

bool FLV_CheckFormat ( XMP_FileFormat format,
					   XMP_StringPtr  filePath,
					   XMP_IO*    fileRef,
					   XMPFiles *     parent )
{
	XMP_Uns8 buffer [9];

	fileRef->Rewind();
	XMP_Uns32 ioCount = fileRef->Read ( buffer, 9 );
	if ( ioCount != 9 ) return false;

	XMP_Uns32 fileSignature = GetUns32BE ( &buffer[0] );
	if ( fileSignature != kFLV1 ) return false;

	XMP_Uns32 headerSize = GetUns32BE ( &buffer[5] );
	XMP_Uns64 fileSize   = fileRef->Length();
	if ( (fileSize < (headerSize + 4)) && (fileSize != headerSize) ) return false;

	if ( fileSize >= (headerSize + 4) ) {
		XMP_Uns32 bpZero;
		fileRef->Seek ( headerSize, kXMP_SeekFromStart );
		ioCount = fileRef->Read ( &bpZero, 4 );
		if ( (ioCount != 4) || (bpZero != 0) ) return false;
	}

	return true;

}	// FLV_CheckFormat

// =================================================================================================
// FLV_MetaHandlerCTor
// ===================

XMPFileHandler * FLV_MetaHandlerCTor ( XMPFiles * parent )
{

	return new FLV_MetaHandler ( parent );

}	// FLV_MetaHandlerCTor

// =================================================================================================
// FLV_MetaHandler::FLV_MetaHandler
// ================================

FLV_MetaHandler::FLV_MetaHandler ( XMPFiles * _parent )
	: flvHeaderLen(0), longXMP(false), xmpTagPos(0), omdTagPos(0), xmpTagLen(0), omdTagLen(0)
{

	this->parent = _parent;	// Inherited, can't set in the prefix.
	this->handlerFlags = kFLV_HandlerFlags;
	this->stdCharForm  = kXMP_Char8Bit;

}	// FLV_MetaHandler::FLV_MetaHandler

// =================================================================================================
// FLV_MetaHandler::~FLV_MetaHandler
// =================================

FLV_MetaHandler::~FLV_MetaHandler()
{

	// Nothing to do yet.

}	// FLV_MetaHandler::~FLV_MetaHandler

// =================================================================================================
// GetTagInfo
// ==========
//
// Seek to the start of a tag and extract the type, data size, and timestamp. Leave the file
// positioned at the first byte of data.

struct TagInfo {
	XMP_Uns8  type;
	XMP_Uns32 time;
	XMP_Uns32 dataSize;
};

static void GetTagInfo ( XMP_IO* fileRef, XMP_Uns64 tagPos, TagInfo * info )
{
	XMP_Uns8 buffer [11];

	fileRef->Seek ( tagPos, kXMP_SeekFromStart );
	fileRef->ReadAll ( buffer, 11 );

	info->type = buffer[0];
	info->time = GetUns24BE ( &buffer[4] ) || (buffer[7] << 24);
	info->dataSize = GetUns24BE ( &buffer[1] );

}	// GetTagInfo

// =================================================================================================
// GetASValueLen
// =============
//
// Return the full length of a serialized ActionScript value, including the type byte, zero if unknown.

static XMP_Uns32 GetASValueLen ( const XMP_Uns8 * asValue, const XMP_Uns8 * asLimit )
{
	XMP_Uns32 valueLen = 0;
	const XMP_Uns8 * itemPtr;
	XMP_Uns32 arrayCount;

	switch ( asValue[0] ) {

		case 0 :	// IEEE double
			valueLen = 1 + 8;
			break;

		case 1 :	// UI8 Boolean
			valueLen = 1 + 1;
			break;

		case 2 :	// Short string
			valueLen = 1 + 2 + GetUns16BE ( &asValue[1] );
			break;

		case 3 :	// ActionScript object, a name and value.
			itemPtr = &asValue[1];
			itemPtr += 2 + GetUns16BE ( itemPtr );	// Move past the name portion.
			itemPtr += GetASValueLen ( itemPtr, asLimit );	// And past the data portion.
			valueLen = (XMP_Uns32) (itemPtr - asValue);
			break;

		case 4 :	// Short string (movie clip path)
			valueLen = 1 + 2 + GetUns16BE ( &asValue[1] );
			break;

		case 5 :	// Null
			valueLen = 1;
			break;

		case 6 :	// Undefined
			valueLen = 1;
			break;

		case 7 :	// UI16 reference ID
			valueLen = 1 + 2;
			break;

		case 8 :	// ECMA array, ignore the count, look for the 0x000009 terminator.
			itemPtr = &asValue[5];
			while ( itemPtr < asLimit ) {
				XMP_Uns16 nameLen = GetUns16BE ( itemPtr );
				itemPtr += 2 + nameLen;	// Move past the name portion.
				if ( (nameLen == 0) && (*itemPtr == 9) ) {
					itemPtr += 1;
					break;	// Done, found the 0x000009 terminator.
				}
				itemPtr += GetASValueLen ( itemPtr, asLimit );	// And past the data portion.
			}
			valueLen = (XMP_Uns32) (itemPtr - asValue);
			break;

		case 10 :	// Strict array, has an exact count.
			arrayCount = GetUns32BE ( &asValue[1] );
			itemPtr = &asValue[5];
			for ( ; (arrayCount > 0) && (itemPtr < asLimit); --arrayCount ) {
				itemPtr += 2 + GetUns16BE ( itemPtr );	// Move past the name portion.
				itemPtr += GetASValueLen ( itemPtr, asLimit );	// And past the data portion.
			}
			valueLen = (XMP_Uns32) (itemPtr - asValue);
			break;

		case 11 :	// Date
			valueLen = 1 + 8 + 2;
			break;

		case  12:	// Long string
			valueLen = 1 + 4 + GetUns32BE ( &asValue[1] );
			break;

	}

	return valueLen;

}	// GetASValueLen

// =================================================================================================
// CheckName
// =========
//
// Check for the name portion of a script data tag or array item, with optional nul terminator. The
// wantedLen must not count the terminator.

static inline bool CheckName ( XMP_StringPtr inputName,  XMP_Uns16 inputLen,
							   XMP_StringPtr wantedName, XMP_Uns16 wantedLen )
{

	if ( inputLen == wantedLen+1 ) {
		if ( inputName[wantedLen] != 0 ) return false;	// Extra byte must be terminating nul.
		--inputLen;
	}

	if ( (inputLen == wantedLen) && XMP_LitNMatch ( inputName, wantedName, wantedLen ) ) return true;
	return false;

}	// CheckName

// =================================================================================================
// FLV_MetaHandler::CacheFileData
// ==============================
//
// Look for the onXMPData and onMetaData script data tags at time 0. Cache all of onMetaData, it
// shouldn't be that big and this removes a need to know what is reconciled. It can't be more than
// 16MB anyway, the size field is only 24 bits.

void FLV_MetaHandler::CacheFileData()
{
	XMP_Assert ( ! this->containsXMP );

	XMP_AbortProc abortProc  = this->parent->abortProc;
	void *        abortArg   = this->parent->abortArg;
	const bool    checkAbort = (abortProc != 0);

	XMP_IO* fileRef  = this->parent->ioRef;
	XMP_Uns64   fileSize = fileRef->Length();

	XMP_Uns8  buffer [16];	// Enough for 1+2+"onMetaData"+nul.
	XMP_Uns32 ioCount;
	TagInfo   info;

	fileRef->Seek ( 5, kXMP_SeekFromStart );
	fileRef->ReadAll ( buffer, 4 );

	this->flvHeaderLen = GetUns32BE ( &buffer[0] );
	XMP_Uns32 firstTagPos = this->flvHeaderLen + 4;	// Include the initial zero back pointer.

	if ( firstTagPos >= fileSize ) return;	// Quit now if the file is just a header.

	for ( XMP_Uns64 tagPos = firstTagPos; tagPos < fileSize; tagPos += (11 + info.dataSize + 4) ) {

		if ( checkAbort && abortProc(abortArg) ) {
			XMP_Throw ( "FLV_MetaHandler::LookForMetadata - User abort", kXMPErr_UserAbort );
		}

		GetTagInfo ( fileRef, tagPos, &info );	// ! GetTagInfo seeks to the tag offset.
		if ( info.time != 0 ) break;
		if ( info.type != 18 ) continue;

		XMP_Assert ( sizeof(buffer) >= (1+2+10+1) );	// 02 000B onMetaData 00
		ioCount = fileRef->Read ( buffer, sizeof(buffer) );
		if ( (ioCount < 4) || (buffer[0] != 0x02) ) continue;

		XMP_Uns16     nameLen = GetUns16BE ( &buffer[1] );
		XMP_StringPtr namePtr = (XMP_StringPtr)(&buffer[3]);

		if ( this->onXMP.empty() && CheckName ( namePtr, nameLen, "onXMPData", 9 ) ) {

			// ! Put the raw data in onXMPData, analyze the value in ProcessXMP.

			this->xmpTagPos = tagPos;
			this->xmpTagLen = 11 + info.dataSize + 4;	// ! Includes the trailing back pointer.

			this->packetInfo.offset = tagPos + 11 + 1+2+nameLen;	// ! Not the real offset yet, the offset of the onXMPData value.

			ioCount = info.dataSize - (1+2+nameLen);	// Just the onXMPData value portion.
			this->onXMP.reserve ( ioCount );
			this->onXMP.assign ( ioCount, ' ' );
			fileRef->Seek ( this->packetInfo.offset, kXMP_SeekFromStart );
			fileRef->ReadAll ( (void*)this->onXMP.data(), ioCount );

			if ( ! this->onMetaData.empty() ) break;	// Done if we've found both.

		} else if ( this->onMetaData.empty() && CheckName ( namePtr, nameLen, "onMetaData", 10 ) ) {

			this->omdTagPos  = tagPos;
			this->omdTagLen  = 11 + info.dataSize + 4;	// ! Includes the trailing back pointer.

			ioCount = info.dataSize - (1+2+nameLen);	// Just the onMetaData value portion.
			this->onMetaData.reserve ( ioCount );
			this->onMetaData.assign ( ioCount, ' ' );
			fileRef->Seek ( (tagPos + 11 + 1+2+nameLen), kXMP_SeekFromStart );
			fileRef->ReadAll ( (void*)this->onMetaData.data(), ioCount );

			if ( ! this->onXMP.empty() ) break;	// Done if we've found both.

		}

	}

}	// FLV_MetaHandler::CacheFileData

// =================================================================================================
// FLV_MetaHandler::MakeLegacyDigest
// =================================

#define kHexDigits "0123456789ABCDEF"

void FLV_MetaHandler::MakeLegacyDigest ( std::string * digestStr )
{
	MD5_CTX    context;
	unsigned char digestBin [16];

	MD5Init ( &context );
	MD5Update ( &context, (XMP_Uns8*)this->onMetaData.data(), (unsigned int)this->onMetaData.size() );
	MD5Final ( digestBin, &context );

	char buffer [40];
	for ( int in = 0, out = 0; in < 16; in += 1, out += 2 ) {
		XMP_Uns8 byte = digestBin[in];
		buffer[out]   = kHexDigits [ byte >> 4 ];
		buffer[out+1] = kHexDigits [ byte & 0xF ];
	}
	buffer[32] = 0;
	digestStr->erase();
	digestStr->append ( buffer, 32 );

}	// FLV_MetaHandler::MakeLegacyDigest

// =================================================================================================
// FLV_MetaHandler::ExtractLiveXML
// ===============================
//
// Extract the XMP packet from the cached onXMPData ECMA array's "liveXMP" item.

void FLV_MetaHandler::ExtractLiveXML()
{
	if ( this->onXMP[0] != 0x08 ) return;	// Make sure onXMPData is an ECMA array.
	const XMP_Uns8 * ecmaArray = (const XMP_Uns8 *) this->onXMP.c_str();
	const XMP_Uns8 * ecmaLimit = ecmaArray + this->onXMP.size();

	if ( this->onXMP.size() >= 3 ) {	// Omit the 0x000009 terminator, simplifies the loop.
		if ( GetUns24BE ( ecmaLimit-3 ) == 9 ) ecmaLimit -= 3;
	}

	for ( const XMP_Uns8 * itemPtr = ecmaArray + 5; itemPtr < ecmaLimit; /* internal increment */ ) {

		// Find the "liveXML" array item, make sure it is a short or long string.

		XMP_Uns16 nameLen = GetUns16BE ( itemPtr );
		const XMP_Uns8 * namePtr = itemPtr + 2;

		itemPtr += (2 + nameLen);	// Move to the value portion.
		XMP_Uns32 valueLen = GetASValueLen ( itemPtr, ecmaLimit );
		if ( valueLen == 0 ) return;	// ! Unknown value type, can't look further.

		if ( CheckName ( (char*)namePtr, nameLen, "liveXML", 7 ) ) {

			XMP_Uns32 lenLen = 2;	// Assume a short string.
			if ( *itemPtr == 12 ) {
				lenLen = 4;
				this->longXMP = true;
			} else if ( *itemPtr != 2 ) {
				return;	// Not a short or long string.
			}

			valueLen -= (1 + lenLen);
			itemPtr += (1 + lenLen);

			this->packetInfo.offset += (itemPtr - ecmaArray);
			this->packetInfo.length += valueLen;

			this->xmpPacket.reserve ( valueLen );
			this->xmpPacket.assign ( (char*)itemPtr, valueLen );

			return;

		}

		itemPtr += valueLen;	// Move past the value portion.

	}

}	// FLV_MetaHandler::ExtractLiveXML

// =================================================================================================
// FLV_MetaHandler::ProcessXMP
// ===========================

void FLV_MetaHandler::ProcessXMP()
{
	if ( this->processedXMP ) return;
	this->processedXMP = true;	// Make sure only called once.

	if ( ! this->onXMP.empty() ) {	// Look for the XMP packet.

		this->ExtractLiveXML();
		if ( ! this->xmpPacket.empty() ) {
			FillPacketInfo ( this->xmpPacket, &this->packetInfo );
			this->xmpObj.ParseFromBuffer ( this->xmpPacket.c_str(), (XMP_StringLen)this->xmpPacket.size() );
			this->containsXMP = true;
		}

	}

	// Now process the legacy, if necessary.

	if ( this->onMetaData.empty() ) return;	// No legacy, we're done.

	std::string oldDigest;
	bool oldDigestFound = this->xmpObj.GetStructField ( kXMP_NS_XMP, "NativeDigests", kXMP_NS_XMP, "FLV", &oldDigest, 0 );

	if ( oldDigestFound ) {
		std::string newDigest;
		this->MakeLegacyDigest ( &newDigest );
		if ( oldDigest == newDigest ) return;	// No legacy changes.
	}

	// *** No spec yet for what legacy to reconcile.

}	// FLV_MetaHandler::ProcessXMP

// =================================================================================================
// FLV_MetaHandler::UpdateFile
// ===========================

void FLV_MetaHandler::UpdateFile ( bool doSafeUpdate )
{
	if ( ! this->needsUpdate ) return;
	XMP_Assert ( ! doSafeUpdate );	// This should only be called for "unsafe" updates.

	XMP_AbortProc abortProc  = this->parent->abortProc;
	void *        abortArg   = this->parent->abortArg;
	const bool    checkAbort = (abortProc != 0);

	XMP_IO* fileRef  = this->parent->ioRef;
	XMP_Uns64   fileSize = fileRef->Length();

	// Make sure the XMP has a legacy digest if appropriate.

	if ( ! this->onMetaData.empty() ) {

		std::string newDigest;
		this->MakeLegacyDigest ( &newDigest );
		this->xmpObj.SetStructField ( kXMP_NS_XMP, "NativeDigests",
									  kXMP_NS_XMP, "FLV", newDigest.c_str(), kXMP_DeleteExisting );

		try {
			XMP_StringLen xmpLen = (XMP_StringLen)this->xmpPacket.size();
			this->xmpObj.SerializeToBuffer ( &this->xmpPacket, (kXMP_UseCompactFormat | kXMP_ExactPacketLength), xmpLen );
		} catch ( ... ) {
			this->xmpObj.SerializeToBuffer ( &this->xmpPacket, kXMP_UseCompactFormat );
		}

	}

	// Rewrite the packet in-place if it fits. Otherwise rewrite the whole file.

	if ( this->xmpPacket.size() == (size_t)this->packetInfo.length ) {
		XMP_ProgressTracker* progressTracker = this->parent->progressTracker;
		if ( progressTracker != 0 ) progressTracker->BeginWork ( (float)this->xmpPacket.size() );
		fileRef->Seek ( this->packetInfo.offset, kXMP_SeekFromStart );
		fileRef->Write ( this->xmpPacket.data(), (XMP_Int32)this->xmpPacket.size() );
		if ( progressTracker != 0 ) progressTracker->WorkComplete();


	} else {

		XMP_IO* tempRef = fileRef->DeriveTemp();
		if ( tempRef == 0 ) XMP_Throw ( "Failure creating FLV temp file", kXMPErr_InternalFailure );

		this->WriteTempFile ( tempRef );
		fileRef->AbsorbTemp();

	}

	this->needsUpdate = false;

}	// FLV_MetaHandler::UpdateFile

// =================================================================================================
// WriteOnXMP
// ==========
//
// Write the XMP packet wrapped up in an ECMA array script data tag:
//
//    0 UI8  tag type : 18
//    1 UI24 content length : 1+2+9+1+4+2+7+1 + <2 or 4> + XMP packet size + 1 + 3
//    4 UI24 time low : 0
//    7 UI8  time high : 0
//    8 UI24 stream ID : 0
//
//   11 UI8  0x02
//   12 UI16 name length : 9
//   14 str9 tag name : "onXMPData", no nul terminator
//   23 UI8  value type : 8
//   24 UI32 array count : 1
//   28 UI16 name length : 7
//   30 str7 item name : "liveXML", no nul terminator
//
//   37 UI8  value type : 2 for a short string, 12 for a long string
//   38 UIn  XMP packet size + 1, UI16 or UI32 as needed
//   -- str  XMP packet, with nul terminator
//
//   -- UI24 array terminator : 0x000009
//   -- UI32 back pointer : content length + 11

static void WriteOnXMP ( XMP_IO* fileRef, const std::string & xmpPacket )
{
	char buffer [64];
	bool longXMP = false;
	XMP_Uns32 tagLen = 1+2+9+1+4+2+7+1 + 2 + (XMP_Uns32)xmpPacket.size() + 1 + 3;

	if ( xmpPacket.size() > 0xFFFE ) {
		longXMP = true;
		tagLen += 2;
	}

	if ( tagLen > 16*1024*1024 ) XMP_Throw ( "FLV tags can't be larger than 16MB", kXMPErr_TBD );

	// Fill in the script data tag header.

	buffer[0] = 18;
	PutUns24BE ( tagLen, &buffer[1] );
	PutUns24BE ( 0, &buffer[4] );
	buffer[7] = 0;
	PutUns24BE ( 0, &buffer[8] );

	// Fill in the "onXMPData" name, ECMA array start, and "liveXML" name.

	buffer[11] = 2;
	PutUns16BE ( 9, &buffer[12] );
	memcpy ( &buffer[14], "onXMPData", 9 );	// AUDIT: Safe, buffer has 64 chars.
	buffer[23] = 8;
	PutUns32BE ( 1, &buffer[24] );
	PutUns16BE ( 7, &buffer[28] );
	memcpy ( &buffer[30], "liveXML", 7 );	// AUDIT: Safe, buffer has 64 chars.

	// Fill in the XMP packet string type and length, write what we have so far.

	fileRef->ToEOF();
	if ( ! longXMP ) {
		buffer[37] = 2;
		PutUns16BE ( (XMP_Uns16)xmpPacket.size()+1, &buffer[38] );
		fileRef->Write ( buffer, 40 );
	} else {
		buffer[37] = 12;
		PutUns32BE ( (XMP_Uns32)xmpPacket.size()+1, &buffer[38] );
		fileRef->Write ( buffer, 42 );
	}

	//  Write the XMP packet, nul terminator, array terminator, and back pointer.

	fileRef->Write ( xmpPacket.c_str(), (XMP_Int32)xmpPacket.size()+1 );
	PutUns24BE ( 9, &buffer[0] );
	PutUns32BE ( tagLen+11, &buffer[3] );
	fileRef->Write ( buffer, 7 );

}	// WriteOnXMP

// =================================================================================================
// FLV_MetaHandler::WriteTempFile
// ==============================
//
// Use a source (old) file and the current XMP to build a destination (new) file. All of the source
// file is copied except for previous XMP. The current XMP is inserted after onMetaData, or at least
// before the first time 0 audio or video tag.

// ! We do not currently update anything in onMetaData.

void FLV_MetaHandler::WriteTempFile ( XMP_IO* tempRef )
{
	if ( ! this->needsUpdate ) return;

	XMP_AbortProc abortProc  = this->parent->abortProc;
	void *        abortArg   = this->parent->abortArg;
	const bool    checkAbort = (abortProc != 0);

	XMP_IO* originalRef = this->parent->ioRef;

	XMP_Uns64 sourceLen = originalRef->Length();
	XMP_Uns64 sourcePos = 0;

	originalRef->Rewind();
	tempRef->Rewind();
	tempRef->Truncate ( 0 );
	XMP_ProgressTracker* progressTracker = this->parent->progressTracker;
	if ( progressTracker != 0 ) {
		float fileSize=(float)(this->xmpPacket.size()+48);
		if ( this->omdTagPos == 0 ) {
			sourcePos=(this->flvHeaderLen+4);
			fileSize+=sourcePos;
		} else {
			if ( this->xmpTagPos < this->omdTagPos ) {
				fileSize+=this->xmpTagPos;
			}
			fileSize+=(this->omdTagPos + this->omdTagLen-
				((this->xmpTagPos!=0 && this->xmpTagPos < this->omdTagPos)?
				(this->xmpTagPos + this->xmpTagLen):0));
			sourcePos =this->omdTagPos + this->omdTagLen;
		}
		if ( (this->xmpTagPos != 0) && (this->xmpTagPos >= sourcePos) ) {
			fileSize+=(this->xmpTagPos - sourcePos);
			sourcePos=this->xmpTagPos + this->xmpTagLen;
		}
		fileSize+=(sourceLen - sourcePos);
		sourcePos=0;
		progressTracker->BeginWork ( fileSize );
	}
	// First do whatever is needed to put the new XMP after any existing onMetaData tag, or as the
	// first time 0 tag.

	if ( this->omdTagPos == 0 ) {

		// There is no onMetaData tag. Copy the file header, then write the new XMP as the first tag.
		// Allow the degenerate case of a file with just a header, no initial back pointer or tags.

		originalRef->Seek ( sourcePos, kXMP_SeekFromStart );
		XIO::Copy ( originalRef, tempRef, this->flvHeaderLen, abortProc, abortArg );

		XMP_Uns32 zero = 0;	// Ensure that the initial back offset really is zero.
		tempRef->Write ( &zero, 4 );
		sourcePos = this->flvHeaderLen + 4;

		WriteOnXMP ( tempRef, this->xmpPacket );

	} else {

		// There is an onMetaData tag. Copy the front of the file through the onMetaData tag,
		// skipping any XMP that happens to be in the way. The XMP should not be before onMetaData,
		// but let's be robust. Write the new XMP immediately after onMetaData, at the same time.

		XMP_Uns64 omdEnd = this->omdTagPos + this->omdTagLen;

		if ( (this->xmpTagPos != 0) && (this->xmpTagPos < this->omdTagPos) ) {
			// The XMP tag was in front of the onMetaData tag. Copy up to it, then skip it.
			originalRef->Seek ( sourcePos, kXMP_SeekFromStart );
			XIO::Copy ( originalRef, tempRef, this->xmpTagPos, abortProc, abortArg );
			sourcePos = this->xmpTagPos + this->xmpTagLen;	// The tag length includes the trailing size field.
		}

		// Copy through the onMetaData tag, then write the XMP.
		originalRef->Seek ( sourcePos, kXMP_SeekFromStart );
		XIO::Copy ( originalRef, tempRef, (omdEnd - sourcePos), abortProc, abortArg );
		sourcePos = omdEnd;

		WriteOnXMP ( tempRef, this->xmpPacket );

	}

	// Copy the rest of the file, skipping any XMP that is in the way.

	if ( (this->xmpTagPos != 0) && (this->xmpTagPos >= sourcePos) ) {
		originalRef->Seek ( sourcePos, kXMP_SeekFromStart );
		XIO::Copy ( originalRef, tempRef, (this->xmpTagPos - sourcePos), abortProc, abortArg );
		sourcePos = this->xmpTagPos + this->xmpTagLen;
	}

	originalRef->Seek ( sourcePos, kXMP_SeekFromStart );
	XIO::Copy ( originalRef, tempRef, (sourceLen - sourcePos), abortProc, abortArg );

	this->needsUpdate = false;
	
	if (  progressTracker != 0  ) progressTracker->WorkComplete();

}	// FLV_MetaHandler::WriteTempFile

// =================================================================================================
