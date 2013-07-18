// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2004 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! This must be the first include.
#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "source/XIO.hpp"

#include "XMPFiles/source/FileHandlers/JPEG_Handler.hpp"

#include "XMPFiles/source/FormatSupport/TIFF_Support.hpp"
#include "XMPFiles/source/FormatSupport/PSIR_Support.hpp"
#include "XMPFiles/source/FormatSupport/IPTC_Support.hpp"
#include "XMPFiles/source/FormatSupport/ReconcileLegacy.hpp"
#include "XMPFiles/source/FormatSupport/Reconcile_Impl.hpp"

#include "third-party/zuid/interfaces/MD5.h"

using namespace std;

// =================================================================================================
/// \file JPEG_Handler.cpp
/// \brief File format handler for JPEG.
///
/// This handler ...
///
// =================================================================================================

static const char * kExifSignatureString = "Exif\0\x00";	// There are supposed to be two zero bytes,
static const char * kExifSignatureAltStr = "Exif\0\xFF";	// but files have been seen with just one.
static const size_t kExifSignatureLength = 6;
static const size_t kExifMaxDataLength   = 0xFFFF - 2 - kExifSignatureLength;

static const char * kPSIRSignatureString = "Photoshop 3.0\0";
static const size_t kPSIRSignatureLength = 14;
static const size_t kPSIRMaxDataLength   = 0xFFFF - 2 - kPSIRSignatureLength;

static const char * kMainXMPSignatureString = "http://ns.adobe.com/xap/1.0/\0";
static const size_t kMainXMPSignatureLength = 29;

static const char * kExtXMPSignatureString = "http://ns.adobe.com/xmp/extension/\0";
static const size_t kExtXMPSignatureLength = 35;
static const size_t kExtXMPPrefixLength    = kExtXMPSignatureLength + 32 + 4 + 4;

typedef std::map < XMP_Uns32 /* offset */, std::string /* portion */ > ExtXMPPortions;

struct ExtXMPContent {
	XMP_Uns32 length;
	ExtXMPPortions portions;
	ExtXMPContent() : length(0) {};
	ExtXMPContent ( XMP_Uns32 _length ) : length(_length) {};
};

typedef std::map < JPEG_MetaHandler::GUID_32 /* guid */, ExtXMPContent /* content */ > ExtendedXMPInfo;

#ifndef Trace_UnlimitedJPEG
	#define Trace_UnlimitedJPEG 0
#endif

// =================================================================================================
// JPEG_MetaHandlerCTor
// ====================

XMPFileHandler * JPEG_MetaHandlerCTor ( XMPFiles * parent )
{
	return new JPEG_MetaHandler ( parent );

}	// JPEG_MetaHandlerCTor

// =================================================================================================
// JPEG_CheckFormat
// ================

// For JPEG we just check for the initial SOI standalone marker followed by any of the other markers
// that might, well, follow it. A more aggressive check might be to read 4KB then check for legit
// marker segments within that portion. Probably won't buy much, and thrashes the dCache more. We
// tolerate only a small amount of 0xFF padding between the SOI and following marker. This formally
// violates the rules of JPEG, but in practice there won't be any padding anyway.
//
// ! The CheckXyzFormat routines don't track the filePos, that is left to ScanXyzFile.

bool JPEG_CheckFormat ( XMP_FileFormat format,
	                    XMP_StringPtr  filePath,
                        XMP_IO *       fileRef,
                        XMPFiles *     parent )
{
	IgnoreParam(format); IgnoreParam(filePath); IgnoreParam(parent);
	XMP_Assert ( format == kXMP_JPEGFile );

	XMP_Uns8 buffer [100];
	XMP_Uns16 marker;

	fileRef->Rewind();
	if ( fileRef->Length() < 2 ) return false;	// Need at least the SOI marker.
	size_t bufferLen = fileRef->Read ( buffer, sizeof(buffer) );

	marker = GetUns16BE ( &buffer[0] );
	if ( marker != 0xFFD8 ) return false;	// Offset 0 must have the SOI marker.
	
	// Skip 0xFF padding and high order 0xFF of next marker.
	size_t bufferPos = 2;
	while ( (bufferPos < bufferLen) && (buffer[bufferPos] == 0xFF) ) bufferPos += 1;
	if ( bufferPos == bufferLen ) return true;	// Nothing but 0xFF bytes, close enough.

	XMP_Uns8 id = buffer[bufferPos];	// Check the ID of the second marker.
	if ( id >= 0xDD ) return true;	// The most probable cases.
	if ( (id < 0xC0) || ((id & 0xF8) == 0xD0) || (id == 0xD8) || (id == 0xDA) || (id == 0xDC) ) return false;
	return true;

}	// JPEG_CheckFormat

// =================================================================================================
// JPEG_MetaHandler::JPEG_MetaHandler
// ==================================

JPEG_MetaHandler::JPEG_MetaHandler ( XMPFiles * _parent )
	: exifMgr(0), psirMgr(0), iptcMgr(0), skipReconcile(false)
{
	this->parent = _parent;
	this->handlerFlags = kJPEG_HandlerFlags;
	this->stdCharForm  = kXMP_Char8Bit;

}	// JPEG_MetaHandler::JPEG_MetaHandler

// =================================================================================================
// JPEG_MetaHandler::~JPEG_MetaHandler
// ===================================

JPEG_MetaHandler::~JPEG_MetaHandler()
{

	if ( exifMgr != 0 ) delete ( exifMgr );
	if ( psirMgr != 0 ) delete ( psirMgr );
	if ( iptcMgr != 0 ) delete ( iptcMgr );

}	// JPEG_MetaHandler::~JPEG_MetaHandler

// =================================================================================================
// CacheExtendedXMP
// ================

static void CacheExtendedXMP ( ExtendedXMPInfo * extXMP, XMP_Uns8 * buffer, size_t bufferLen )
{

	// Have a portion of the extended XMP, cache the contents. This is complicated by the need to
	// tolerate files where the extension portions are not in order. The local ExtendedXMPInfo map
	// uses the GUID as the key and maps that to a struct that has the full length and a map of the
	// known portions. This known portion map uses the offset of the portion as the key and maps
	// that to a string. Only fully seen extended XMP streams are kept, the right one gets picked in
	// ProcessXMP.

	// The extended XMP JPEG marker segment content holds:
	//	- a signature string, "http://ns.adobe.com/xmp/extension/\0", already verified
	//	- a 128 bit GUID stored as a 32 byte ASCII hex string
	//	- a UInt32 full length of the entire extended XMP
	//	- a UInt32 offset for this portion of the extended XMP
	//	- the UTF-8 text for this portion of the extended XMP
	
	if ( bufferLen < kExtXMPPrefixLength ) return;	// Ignore bad input.
	XMP_Assert ( CheckBytes ( &buffer[0], kExtXMPSignatureString, kExtXMPSignatureLength ) );

	XMP_Uns8 * bufferPtr = buffer + kExtXMPSignatureLength;	// Start at the GUID.
	
	JPEG_MetaHandler::GUID_32 guid;
	XMP_Assert ( sizeof(guid.data) == 32 );
	memcpy ( &guid.data[0], bufferPtr, sizeof(guid.data) );	// AUDIT: Use of sizeof(guid.data) is safe.

	bufferPtr += sizeof(guid.data);	// Move to the length and offset.
	XMP_Uns32 fullLen = GetUns32BE ( bufferPtr );
	XMP_Uns32 offset  = GetUns32BE ( bufferPtr+4 );

	bufferPtr += 8;	// Move to the XMP stream portion.
	size_t xmpLen = bufferLen - kExtXMPPrefixLength;

	#if Trace_UnlimitedJPEG
		printf ( "New extended XMP portion: fullLen %d, offset %d, GUID %.32s\n", fullLen, offset, guid.data );
	#endif

	// Find the ExtXMPContent for this GUID, and the string for this portion's offset.

	ExtendedXMPInfo::iterator guidPos = extXMP->find ( guid );
	if ( guidPos == extXMP->end() ) {
		ExtXMPContent newExtContent ( fullLen );
		guidPos = extXMP->insert ( extXMP->begin(), ExtendedXMPInfo::value_type ( guid, newExtContent ) );
	}

	ExtXMPPortions::iterator offsetPos;
	ExtXMPContent & extContent = guidPos->second;

	if ( extContent.portions.empty() ) {
		// When new create a full size offset 0 string, to which all in-order portions will get appended.
		offsetPos = extContent.portions.insert ( extContent.portions.begin(),
												 ExtXMPPortions::value_type ( 0, std::string() ) );
		offsetPos->second.reserve ( extContent.length );
	}

	// Try to append this portion to a logically contiguous preceeding one.

	if ( offset == 0 ) {
		offsetPos = extContent.portions.begin();
		XMP_Assert ( (offsetPos->first == 0) && (offsetPos->second.size() == 0) );
	} else {
		offsetPos = extContent.portions.lower_bound ( offset );
		--offsetPos;	// Back up to the portion whose offset is less than the new offset.
		if ( (offsetPos->first + offsetPos->second.size()) != offset ) {
			// Can't append, create a new portion.
			offsetPos = extContent.portions.insert ( extContent.portions.begin(),
													 ExtXMPPortions::value_type ( offset, std::string() ) );
		}
	}

	// Cache this portion of the extended XMP.

	std::string & extPortion = offsetPos->second;
	extPortion.append ( (XMP_StringPtr)bufferPtr, xmpLen );

}	// CacheExtendedXMP

// =================================================================================================
// JPEG_MetaHandler::CacheFileData
// ===============================
//
// Look for the Exif metadata, Photoshop image resources, and XMP in a JPEG (JFIF) file. The native
// thumbnail is inside the Exif. The general layout of a JPEG file is:
//    SOI marker, 2 bytes, 0xFFD8
//    Marker segments for tables and metadata
//    SOFn marker segment
//    Image data
//    EOI marker, 2 bytes, 0xFFD9
//
// Each marker segment begins with a 2 byte big endian marker and a 2 byte big endian length. The
// length includes the 2 bytes of the length field but not the marker. The high order byte of a
// marker is 0xFF, the low order byte tells what kind of marker. A marker can be preceeded by any
// number of 0xFF fill bytes, however there are no alignment constraints.
//
// There are virtually no constraints on the order of the marker segments before the SOFn. A reader
// must be prepared to handle any order.
//
// The Exif metadata is in an APP1 marker segment with a 6 byte signature string of "Exif\0\0" at
// the start of the data. The rest of the data is a TIFF stream.
//
// The Photoshop image resources are in an APP13 marker segment with a 14 byte signature string of
// "Photoshop 3.0\0". The rest of the data is a sequence of image resources.
//
// The main XMP is in an APP1 marker segment with a 29 byte signature string of
// "http://ns.adobe.com/xap/1.0/\0". The rest of the data is the serialized XMP packet. This is the
// only XMP if everything fits within the 64KB limit for marker segment data. If not, there will be
// a series of XMP extension segments.
//
// Each XMP extension segment is an APP1 marker segment whose data contains:
//    - A 35 byte signature string of "http://ns.adobe.com/xmp/extension/\0".
//    - A 128 bit GUID stored as 32 ASCII hex digits, capital A-F, no nul termination.
//    - A 32 bit unsigned integer length for the full extended XMP serialization.
//    - A 32 bit unsigned integer offset for this portion of the extended XMP serialization.
//    - A portion of the extended XMP serialization, up to about 65400 bytes (at most 65458).
//
// A reader must be prepared to encounter the extended XMP portions out of order. Also to encounter
// defective files that have differing extended XMP according to the GUID. The main XMP contains the
// GUID for the associated extended XMP.

// *** This implementation simply returns when invalid JPEG is encountered. Should we throw instead?

void JPEG_MetaHandler::CacheFileData()
{
	XMP_IO* fileRef = this->parent->ioRef;
	XMP_PacketInfo & packetInfo = this->packetInfo;

	static const size_t kBufferSize = 64*1024;	// Enough for maximum segment contents.
	XMP_Uns8 buffer [kBufferSize];

	XMP_AbortProc abortProc  = this->parent->abortProc;
	void *        abortArg   = this->parent->abortArg;
	const bool    checkAbort = (abortProc != 0);

	ExtendedXMPInfo extXMP;

	XMP_Assert ( ! this->containsXMP );
	// Set containsXMP to true here only if the standard XMP packet is found.

	XMP_Assert ( kPSIRSignatureLength == (strlen(kPSIRSignatureString) + 1) );
	XMP_Assert ( kMainXMPSignatureLength == (strlen(kMainXMPSignatureString) + 1) );
	XMP_Assert ( kExtXMPSignatureLength == (strlen(kExtXMPSignatureString) + 1) );

	// -------------------------------------------------------------------------------------------
	// Look for any of the Exif, PSIR, main XMP, or extended XMP marker segments. Quit when we hit
	// an SOFn, EOI, or invalid/unexpected marker.

	fileRef->Seek ( 2, kXMP_SeekFromStart  );	// Skip the SOI, CheckFormat made sure it is present.

	while ( true ) {

		if ( checkAbort && abortProc(abortArg) ) {
			XMP_Throw ( "JPEG_MetaHandler::CacheFileData - User abort", kXMPErr_UserAbort );
		}

		if ( ! XIO::CheckFileSpace ( fileRef, 2 ) ) return;	// Quit, don't throw, if the file ends unexpectedly.
		
		XMP_Uns16 marker = XIO::ReadUns16_BE ( fileRef );	// Read the next marker.
		if ( marker == 0xFFFF ) {
			// Have a pad byte, skip it. These are almost unheard of, so efficiency isn't critical.
			fileRef->Seek ( -1, kXMP_SeekFromCurrent );	// Skip the first 0xFF, read the second again.
			continue;
		}

		if ( (marker == 0xFFDA) || (marker == 0xFFD9) ) break;	// Quit reading at the first SOS marker or at EOI.

		if ( (marker == 0xFF01) ||	// Ill-formed file if we encounter a TEM or RSTn marker.
			 ((0xFFD0 <= marker) && (marker <= 0xFFD7)) ) return;

		XMP_Uns16 contentLen = XIO::ReadUns16_BE ( fileRef );	// Read this segment's length.
		if ( contentLen < 2 ) XMP_Throw ( "Invalid JPEG segment length", kXMPErr_BadJPEG );
		contentLen -= 2;	// Reduce to just the content length.
		
		XMP_Int64 contentOrigin = fileRef->Offset();
		size_t signatureLen;

		if ( (marker == 0xFFED) && (contentLen >= kPSIRSignatureLength) ) {

			// This is an APP13 marker, is it the Photoshop image resources?

			signatureLen = fileRef->Read ( buffer, kPSIRSignatureLength );
			if ( (signatureLen == kPSIRSignatureLength) &&
				 CheckBytes ( &buffer[0], kPSIRSignatureString, kPSIRSignatureLength ) ) {

				size_t psirLen = contentLen - kPSIRSignatureLength;
				fileRef->Seek ( (contentOrigin + kPSIRSignatureLength), kXMP_SeekFromStart );
				fileRef->ReadAll ( buffer, psirLen );
				this->psirContents.assign ( (char*)buffer, psirLen );
				continue;	// Move on to the next marker.

			}

		} else if ( (marker == 0xFFE1) && (contentLen >= kExifSignatureLength) ) {	// Check for the shortest signature.

			// This is an APP1 marker, is it the Exif, main XMP, or extended XMP?
			// ! Check in that order, which is in increasing signature string length.
			
			XMP_Assert ( (kExifSignatureLength < kMainXMPSignatureLength) &&
						 (kMainXMPSignatureLength < kExtXMPSignatureLength) );
			signatureLen = fileRef->Read ( buffer, kExtXMPSignatureLength );	// Read for the longest signature.

			if ( (signatureLen >= kExifSignatureLength) &&
				 (CheckBytes ( &buffer[0], kExifSignatureString, kExifSignatureLength ) ||
				  CheckBytes ( &buffer[0], kExifSignatureAltStr, kExifSignatureLength )) ) {

				size_t exifLen = contentLen - kExifSignatureLength;
				fileRef->Seek ( (contentOrigin + kExifSignatureLength), kXMP_SeekFromStart );
				fileRef->ReadAll ( buffer, exifLen );
				this->exifContents.assign ( (char*)buffer, exifLen );
				continue;	// Move on to the next marker.

			}
			
			if ( (signatureLen >= kMainXMPSignatureLength) &&
				 CheckBytes ( &buffer[0], kMainXMPSignatureString, kMainXMPSignatureLength ) ) {

				this->containsXMP = true;	// Found the standard XMP packet.
				size_t xmpLen = contentLen - kMainXMPSignatureLength;
				fileRef->Seek ( (contentOrigin + kMainXMPSignatureLength), kXMP_SeekFromStart );
				fileRef->ReadAll ( buffer, xmpLen );
				this->xmpPacket.assign ( (char*)buffer, xmpLen );
				this->packetInfo.offset = contentOrigin + kMainXMPSignatureLength;
				this->packetInfo.length = (XMP_Int32)xmpLen;
				this->packetInfo.padSize   = 0;	// Assume the rest for now, set later in ProcessXMP.
				this->packetInfo.charForm  = kXMP_CharUnknown;
				this->packetInfo.writeable = true;
				continue;	// Move on to the next marker.

			}
			
			if ( (signatureLen >= kExtXMPSignatureLength) &&
				 CheckBytes ( &buffer[0], kExtXMPSignatureString, kExtXMPSignatureLength ) ) {

				fileRef->Seek ( contentOrigin, kXMP_SeekFromStart );
				fileRef->ReadAll ( buffer, contentLen );
				CacheExtendedXMP ( &extXMP, buffer, contentLen );
				continue;	// Move on to the next marker.

			}

		}
		
		// None of the above, seek to the next marker.
		fileRef->Seek ( (contentOrigin + contentLen) , kXMP_SeekFromStart );

	}

	if ( ! extXMP.empty() ) {

		// We have extended XMP. Find out which ones are complete, collapse them into a single
		// string, and save them for ProcessXMP.

		ExtendedXMPInfo::iterator guidPos = extXMP.begin();
		ExtendedXMPInfo::iterator guidEnd = extXMP.end();

		for ( ; guidPos != guidEnd; ++guidPos ) {

			ExtXMPContent & thisContent = guidPos->second;
			ExtXMPPortions::iterator partZero = thisContent.portions.begin();
			ExtXMPPortions::iterator partEnd  = thisContent.portions.end();
			ExtXMPPortions::iterator partPos  = partZero;

			#if Trace_UnlimitedJPEG
				printf ( "Extended XMP portions for GUID %.32s, full length %d\n",
					     guidPos->first.data, guidPos->second.length );
				printf ( "  Offset %d, length %d, next offset %d\n",
						 partZero->first, partZero->second.size(), (partZero->first + partZero->second.size()) );
			#endif

			for ( ++partPos; partPos != partEnd; ++partPos ) {
				#if Trace_UnlimitedJPEG
					printf ( "  Offset %d, length %d, next offset %d\n",
							 partPos->first, partPos->second.size(), (partPos->first + partPos->second.size()) );
				#endif
				if ( partPos->first != partZero->second.size() ) break;	// Quit if not contiguous.
				partZero->second.append ( partPos->second );
			}

			if ( (partPos == partEnd) && (partZero->first == 0) && (partZero->second.size() == thisContent.length) ) {
				// This is a complete extended XMP stream.
				this->extendedXMP.insert ( ExtendedXMPMap::value_type ( guidPos->first, partZero->second ) );
				#if Trace_UnlimitedJPEG
					printf ( "Full extended XMP for GUID %.32s, full length %d\n",
							 guidPos->first.data, partZero->second.size() );
				#endif
			}

		}

	}

}	// JPEG_MetaHandler::CacheFileData

// =================================================================================================
// TrimFullExifAPP1
// ================
//
// Try to trim trailing padding from full Exif APP1 segment written by some Nikon cameras. Do a
// temporary read-only parse of the Exif APP1 contents, determine the highest used offset, trim the
// padding if all zero bytes.

static const char * IFDNames[] = { "Primary", "TNail", "Exif", "GPS", "Interop", };

static void TrimFullExifAPP1 ( std::string * exifContents )
{
	TIFF_MemoryReader tempMgr;
	TIFF_MemoryReader::TagInfo tagInfo;
	bool tagFound, isNikon;

	// ! Make a copy of the data to parse! The RO memory TIFF manager will flip bytes in-place!
	tempMgr.ParseMemoryStream ( exifContents->data(), (XMP_Uns32)exifContents->size(), true /* copy data */ );

	// Only trim the Exif APP1 from Nikon cameras.
	tagFound = tempMgr.GetTag ( kTIFF_PrimaryIFD, kTIFF_Make, &tagInfo );
	isNikon = tagFound && (tagInfo.type == kTIFF_ASCIIType) && (tagInfo.count >= 5) &&
						  (memcmp ( tagInfo.dataPtr, "NIKON", 5) == 0);
	if ( ! isNikon ) return;

	// Find the start of the padding, one past the highest used offset. Look at the IFD structure,
	// and the thumbnail info. Ignore the MakerNote tag, Nikon says they are self-contained.

	XMP_Uns32 padOffset = 0;

	for ( XMP_Uns8 ifd = 0; ifd < kTIFF_KnownIFDCount; ++ifd ) {

		TIFF_MemoryReader::TagInfoMap tagMap;
		bool ifdFound = tempMgr.GetIFD ( ifd, &tagMap );
		if ( ! ifdFound ) continue;

		TIFF_MemoryReader::TagInfoMap::const_iterator mapPos = tagMap.begin();
		TIFF_MemoryReader::TagInfoMap::const_iterator mapEnd = tagMap.end();

		for ( ; mapPos != mapEnd; ++mapPos ) {
			const TIFF_MemoryReader::TagInfo & tagInfo = mapPos->second;
			XMP_Uns32 tagEnd = tempMgr.GetValueOffset ( ifd, tagInfo.id ) + tagInfo.dataLen;
			if ( tagEnd > padOffset ) padOffset = tagEnd;
		}

	}

	tagFound = tempMgr.GetTag ( kTIFF_TNailIFD, kTIFF_JPEGInterchangeFormat, &tagInfo );
	if ( tagFound ) {
		XMP_Uns32 tnailOffset = tempMgr.GetUns32 ( tagInfo.dataPtr );
		tagFound = tempMgr.GetTag ( kTIFF_TNailIFD, kTIFF_JPEGInterchangeFormatLength, &tagInfo );
		if ( ! tagFound ) return;	// Don't trim if there is a TNail offset but no length.
		tnailOffset += tempMgr.GetUns32 ( tagInfo.dataPtr );
		if ( tnailOffset > padOffset ) padOffset = tnailOffset;
	}

	// Decide if it is OK to trim the Exif segment. It is OK if the padding is all zeros. It is OK
	// if the last non-zero byte is no more than 64 bytes into the padding and there are at least
	// an additional 64 bytes of padding after it.
	
	if ( padOffset >= exifContents->size() ) return;	// Sanity check for an OK last used offset.

	size_t lastNonZero = exifContents->size() - 1;
	while ( (lastNonZero >= padOffset) && ((*exifContents)[lastNonZero] == 0) ) --lastNonZero;
	
	bool ok = lastNonZero < padOffset;
	if ( ! ok ) {
		size_t nzSize = lastNonZero - padOffset + 1;
		size_t finalSize = (exifContents->size() - 1) - lastNonZero;
		if ( (nzSize < 64) && (finalSize > 64) ) {
			padOffset = lastNonZero + 64;
			assert ( padOffset < exifContents->size() );
			ok = true;
		}
	}
	
	if ( ok ) exifContents->erase ( padOffset );

}	// TrimFullExifAPP1

// =================================================================================================
// JPEG_MetaHandler::ProcessXMP
// ============================
//
// Process the raw XMP and legacy metadata that was previously cached.

void JPEG_MetaHandler::ProcessXMP()
{

	XMP_Assert ( ! this->processedXMP );
	this->processedXMP = true;	// Make sure we only come through here once.

	// Create the PSIR and IPTC handlers, even if there is no legacy. They might be needed for updates.

	XMP_Assert ( (this->psirMgr == 0) && (this->iptcMgr == 0) );	// ProcessTNail might create the exifMgr.

	bool readOnly = ((this->parent->openFlags & kXMPFiles_OpenForUpdate) == 0);

	if ( readOnly ) {
		if ( this->exifMgr == 0 ) this->exifMgr = new TIFF_MemoryReader();
		this->psirMgr = new PSIR_MemoryReader();
		this->iptcMgr = new IPTC_Reader();	// ! Parse it later.
	} else {
		if ( this->exifContents.size() == (65534 - 2 - 6) ) TrimFullExifAPP1 ( &this->exifContents );
		if ( this->exifMgr == 0 ) this->exifMgr = new TIFF_FileWriter();
		this->psirMgr = new PSIR_FileWriter();
		this->iptcMgr = new IPTC_Writer();	// ! Parse it later.
	}
	if ( this->parent )
		exifMgr->SetErrorCallback( &this->parent->errorCallback );

	// Set up everything for the legacy import, but don't do it yet. This lets us do a forced legacy
	// import if the XMP packet gets parsing errors.

	TIFF_Manager & exif = *this->exifMgr;	// Give the compiler help in recognizing non-aliases.
	PSIR_Manager & psir = *this->psirMgr;
	IPTC_Manager & iptc = *this->iptcMgr;

	bool haveExif = (! this->exifContents.empty());
	if ( haveExif ) {
		exif.ParseMemoryStream ( this->exifContents.c_str(), (XMP_Uns32)this->exifContents.size() );
	}

	bool havePSIR = (! this->psirContents.empty());
	if ( havePSIR ) {
		psir.ParseMemoryResources ( this->psirContents.c_str(), (XMP_Uns32)this->psirContents.size() );
	}

	PSIR_Manager::ImgRsrcInfo iptcInfo;
	bool haveIPTC = false;
	if ( havePSIR ) haveIPTC = psir.GetImgRsrc ( kPSIR_IPTC, &iptcInfo );;
	int iptcDigestState = kDigestMatches;

	if ( haveIPTC ) {

		bool haveDigest = false;
		PSIR_Manager::ImgRsrcInfo digestInfo;
		if ( havePSIR ) haveDigest = psir.GetImgRsrc ( kPSIR_IPTCDigest, &digestInfo );
		if ( digestInfo.dataLen != 16 ) haveDigest = false;

		if ( ! haveDigest ) {
			iptcDigestState = kDigestMissing;
		} else {
			iptcDigestState = PhotoDataUtils::CheckIPTCDigest ( iptcInfo.dataPtr, iptcInfo.dataLen, digestInfo.dataPtr );
		}

	}

	XMP_OptionBits options = 0;
	if ( this->containsXMP ) options |= k2XMP_FileHadXMP;
	if ( haveExif ) options |= k2XMP_FileHadExif;
	if ( haveIPTC ) options |= k2XMP_FileHadIPTC;

	// Process the main XMP packet. If it fails to parse, do a forced legacy import but still throw
	// an exception. This tells the caller that an error happened, but gives them recovered legacy
	// should they want to proceed with that.

	bool haveXMP = false;

	if ( ! this->xmpPacket.empty() ) {
		XMP_Assert ( this->containsXMP );
		// Common code takes care of packetInfo.charForm, .padSize, and .writeable.
		XMP_StringPtr packetStr = this->xmpPacket.c_str();
		XMP_StringLen packetLen = (XMP_StringLen)this->xmpPacket.size();
		try {
			this->xmpObj.ParseFromBuffer ( packetStr, packetLen );
		} catch ( ... ) { /* Ignore parsing failures, someday we hope to get partial XMP back. */ }
		haveXMP = true;
	}

	// Process the extended XMP if it has a matching GUID.

	if ( ! this->extendedXMP.empty() ) {

		bool found;
		GUID_32 g32;
		std::string extGUID, extPacket;
		ExtendedXMPMap::iterator guidPos = this->extendedXMP.end();

		found = this->xmpObj.GetProperty ( kXMP_NS_XMP_Note, "HasExtendedXMP", &extGUID, 0 );
		if ( found && (extGUID.size() == sizeof(g32.data)) ) {
			XMP_Assert ( sizeof(g32.data) == 32 );
			memcpy ( g32.data, extGUID.c_str(), sizeof(g32.data) );	// AUDIT: Use of sizeof(g32.data) is safe.
			guidPos = this->extendedXMP.find ( g32 );
			this->xmpObj.DeleteProperty ( kXMP_NS_XMP_Note, "HasExtendedXMP" );	// ! Must only be in the file.
			#if Trace_UnlimitedJPEG
				printf ( "%s extended XMP for GUID %s\n",
					     ((guidPos != this->extendedXMP.end()) ? "Found" : "Missing"), extGUID.c_str() );
			#endif
		}

		if ( guidPos != this->extendedXMP.end() ) {
			try {
				XMP_StringPtr extStr = guidPos->second.c_str();
				XMP_StringLen extLen = (XMP_StringLen)guidPos->second.size();
				SXMPMeta extXMP ( extStr, extLen );
				SXMPUtils::MergeFromJPEG ( &this->xmpObj, extXMP );
			} catch ( ... ) {
				// Ignore failures, let the rest of the XMP and legacy be kept.
			}
		}

	}

	// Process the legacy metadata.

	if ( haveIPTC && (! haveXMP) && (iptcDigestState == kDigestMatches) ) iptcDigestState = kDigestMissing;
	bool parseIPTC = (iptcDigestState != kDigestMatches) || (! readOnly);
	if ( parseIPTC ) iptc.ParseMemoryDataSets ( iptcInfo.dataPtr, iptcInfo.dataLen );
	ImportPhotoData ( exif, iptc, psir, iptcDigestState, &this->xmpObj, options );

	this->containsXMP = true;	// Assume we had something for the XMP.

}	// JPEG_MetaHandler::ProcessXMP

// =================================================================================================
// JPEG_MetaHandler::UpdateFile
// ============================

void JPEG_MetaHandler::UpdateFile ( bool doSafeUpdate )
{
	XMP_Assert ( ! doSafeUpdate );	// This should only be called for "unsafe" updates.

	XMP_Int64 oldPacketOffset = this->packetInfo.offset;
	XMP_Int32 oldPacketLength = this->packetInfo.length;

	if ( oldPacketOffset == kXMPFiles_UnknownOffset ) oldPacketOffset = 0;	// ! Simplify checks.
	if ( oldPacketLength == kXMPFiles_UnknownLength ) oldPacketLength = 0;

	bool fileHadXMP = ((oldPacketOffset != 0) && (oldPacketLength != 0));

	// Update the IPTC-IIM and native TIFF/Exif metadata. ExportPhotoData also trips the tiff: and
	// exif: copies from the XMP, so reserialize the now final XMP packet.

	ExportPhotoData ( kXMP_JPEGFile, &this->xmpObj, this->exifMgr, this->iptcMgr, this->psirMgr );

	try {
		XMP_OptionBits options = kXMP_UseCompactFormat;
		if ( fileHadXMP ) options |= kXMP_ExactPacketLength;
		this->xmpObj.SerializeToBuffer ( &this->xmpPacket, options, oldPacketLength );
	} catch ( ... ) {
		this->xmpObj.SerializeToBuffer ( &this->xmpPacket, kXMP_UseCompactFormat );
	}

	// Decide whether to do an in-place update. This can only happen if all of the following are true:
	//	- There is a standard packet in the file.
	//	- There is no extended XMP in the file.
	//	- The are no changes to the legacy Exif or PSIR portions. (The IPTC is in the PSIR.)
	//	- The new XMP can fit in the old space, without extensions.

	bool doInPlace = (fileHadXMP && (this->xmpPacket.size() <= (size_t)oldPacketLength));

	if ( ! this->extendedXMP.empty() ) doInPlace = false;

	if ( (this->exifMgr != 0) && (this->exifMgr->IsLegacyChanged()) ) doInPlace = false;
	if ( (this->psirMgr != 0) && (this->psirMgr->IsLegacyChanged()) ) doInPlace = false;

	if ( doInPlace ) {

		#if GatherPerformanceData
			sAPIPerf->back().extraInfo += ", JPEG in-place update";
		#endif

		if ( this->xmpPacket.size() < (size_t)this->packetInfo.length ) {
			// They ought to match, cheap to be sure.
			size_t extraSpace = (size_t)this->packetInfo.length - this->xmpPacket.size();
			this->xmpPacket.append ( extraSpace, ' ' );
		}

		XMP_IO* liveFile = this->parent->ioRef;
		std::string & newPacket = this->xmpPacket;

		XMP_Assert ( newPacket.size() == (size_t)oldPacketLength );	// ! Done by common PutXMP logic.

		liveFile->Seek ( oldPacketOffset, kXMP_SeekFromStart  );
		liveFile->Write ( newPacket.c_str(), (XMP_Int32)newPacket.size() );

	} else {

		#if GatherPerformanceData
			sAPIPerf->back().extraInfo += ", JPEG copy update";
		#endif

		XMP_IO* origRef = this->parent->ioRef;
		XMP_IO* tempRef = origRef->DeriveTemp();

		try {
			XMP_Assert ( ! this->skipReconcile );
			this->skipReconcile = true;
			this->WriteTempFile ( tempRef );
			this->skipReconcile = false;
		} catch ( ... ) {
			this->skipReconcile = false;
			origRef->DeleteTemp();
			throw;
		}

		origRef->AbsorbTemp();

	}

	this->needsUpdate = false;

}	// JPEG_MetaHandler::UpdateFile

// =================================================================================================
// JPEG_MetaHandler::WriteTempFile
// ===============================
//
// The metadata parts of a JPEG file are APP1 marker segments for Exif and XMP, and an APP13 marker
// segment for Photoshop image resources which contain the IPTC. Corresponding marker segments in
// the source file are ignored, other parts of the source file are copied. Any initial APP0 marker
// segments are copied first. Then the new Exif, XMP, and PSIR marker segments are written. Then the
// rest of the file is copied, skipping the old Exif, XMP, and PSIR. The checking for old metadata
// stops at the first SOFn marker.

void JPEG_MetaHandler::WriteTempFile ( XMP_IO* tempRef )
{
	XMP_IO* origRef = this->parent->ioRef;

	XMP_AbortProc abortProc  = this->parent->abortProc;
	void *        abortArg   = this->parent->abortArg;
	const bool    checkAbort = (abortProc != 0);

	XMP_Uns16 marker, contentLen;

	static const size_t kBufferSize = 64*1024;	// Enough for a segment with maximum contents.
	XMP_Uns8 buffer [kBufferSize];
	
	XMP_Int64 origLength = origRef->Length();
	if ( origLength == 0 ) return;	// Tolerate empty files.
	if ( origLength < 4 ) {
		XMP_Throw ( "JPEG must have at least SOI and EOI markers", kXMPErr_BadJPEG );
	}

	if ( ! skipReconcile ) {
		// Update the IPTC-IIM and native TIFF/Exif metadata, and reserialize the now final XMP packet.
		ExportPhotoData ( kXMP_JPEGFile, &this->xmpObj, this->exifMgr, this->iptcMgr, this->psirMgr );
		this->xmpObj.SerializeToBuffer ( &this->xmpPacket, kXMP_UseCompactFormat );
	}

	origRef->Rewind();
	tempRef->Truncate ( 0 );

	marker = XIO::ReadUns16_BE ( origRef );	// Just read the SOI marker.
	if ( marker != 0xFFD8 ) XMP_Throw ( "Missing SOI marker", kXMPErr_BadJPEG );
	XIO::WriteUns16_BE ( tempRef, marker );

	// Copy any leading APP0 marker segments.

	while ( true ) {

		if ( checkAbort && abortProc(abortArg) ) {
			XMP_Throw ( "JPEG_MetaHandler::WriteFile - User abort", kXMPErr_UserAbort );
		}
		
		if ( ! XIO::CheckFileSpace ( origRef, 2 ) ) break;	// Tolerate a file that ends abruptly.
		
		marker = XIO::ReadUns16_BE ( origRef );	// Read the next marker.
		if ( marker == 0xFFFF ) {
			// Have a pad byte, skip it. These are almost unheard of, so efficiency isn't critical.
			origRef->Seek ( -1, kXMP_SeekFromCurrent );	// Skip the first 0xFF, read the second again.
			continue;
		}

		if ( marker != 0xFFE0 ) break;	// Have a non-APP0 marker.
		XIO::WriteUns16_BE ( tempRef, marker );	// Write the APP0 marker.
		
		contentLen = XIO::ReadUns16_BE ( origRef );	// Copy the APP0 segment's length.
		XIO::WriteUns16_BE ( tempRef, contentLen );

		if ( contentLen < 2 ) XMP_Throw ( "Invalid JPEG segment length", kXMPErr_BadJPEG );
		contentLen -= 2;	// Reduce to just the content length.
		origRef->ReadAll ( buffer, contentLen );	// Copy the APP0 segment's content.
		tempRef->Write ( buffer, contentLen );

	}

	// Write the new Exif APP1 marker segment.

	XMP_Uns32 first4;

	if ( this->exifMgr != 0 ) {

		void* exifPtr;
		XMP_Uns32 exifLen = this->exifMgr->UpdateMemoryStream ( &exifPtr );
		if ( exifLen > kExifMaxDataLength ) exifLen = this->exifMgr->UpdateMemoryStream ( &exifPtr, true /* compact */ );
		if ( exifLen > kExifMaxDataLength ) {
			// XMP_Throw ( "Overflow of Exif APP1 data", kXMPErr_BadJPEG );		** Used to throw, now rewrite original Exif.
			exifPtr = (void*)this->exifContents.c_str();
			exifLen = this->exifContents.size();
		}

		if ( exifLen > 0 ) {
			first4 = MakeUns32BE ( 0xFFE10000 + 2 + kExifSignatureLength + exifLen );
			tempRef->Write ( &first4, 4 );
			tempRef->Write ( kExifSignatureString, kExifSignatureLength );
			tempRef->Write ( exifPtr, exifLen );
		}

	}

	// Write the new XMP APP1 marker segment, with possible extension marker segments.

	std::string mainXMP, extXMP, extDigest;
	SXMPUtils::PackageForJPEG ( this->xmpObj, &mainXMP, &extXMP, &extDigest );
	XMP_Assert ( (extXMP.size() == 0) || (extDigest.size() == 32) );

	first4 = MakeUns32BE ( 0xFFE10000 + 2 + kMainXMPSignatureLength + (XMP_Uns32)mainXMP.size() );
	tempRef->Write ( &first4, 4 );
	tempRef->Write ( kMainXMPSignatureString, kMainXMPSignatureLength );
	tempRef->Write ( mainXMP.c_str(), (XMP_Int32)mainXMP.size() );

	size_t extPos = 0;
	size_t extLen = extXMP.size();

	while ( extLen > 0 ) {

		size_t partLen = extLen;
		if ( partLen > 65000 ) partLen = 65000;

		first4 = MakeUns32BE ( 0xFFE10000 + 2 + kExtXMPPrefixLength + (XMP_Uns32)partLen );
		tempRef->Write ( &first4, 4 );

		tempRef->Write ( kExtXMPSignatureString, kExtXMPSignatureLength );
		tempRef->Write ( extDigest.c_str(), (XMP_Int32)extDigest.size() );

		first4 = MakeUns32BE ( (XMP_Int32)extXMP.size() );
		tempRef->Write ( &first4, 4 );
		first4 = MakeUns32BE ( (XMP_Int32)extPos );
		tempRef->Write ( &first4, 4 );

		tempRef->Write ( &extXMP[extPos], (XMP_Int32)partLen );

		extPos += partLen;
		extLen -= partLen;

	}

	// Write the new PSIR APP13 marker segment.

	if ( this->psirMgr != 0 ) {

		void* psirPtr;
		XMP_Uns32 psirLen = this->psirMgr->UpdateMemoryResources ( &psirPtr );
		if ( psirLen > kPSIRMaxDataLength ) XMP_Throw ( "Overflow of PSIR APP13 data", kXMPErr_BadJPEG );

		if ( psirLen > 0 ) {
			first4 = MakeUns32BE ( 0xFFED0000 + 2 + kPSIRSignatureLength + psirLen );
			tempRef->Write ( &first4, 4 );
			tempRef->Write ( kPSIRSignatureString, kPSIRSignatureLength );
			tempRef->Write ( psirPtr, psirLen );
		}

	}

	// Copy remaining marker segments, skipping old metadata, to the first SOS marker or to EOI.

	origRef->Seek ( -2, kXMP_SeekFromCurrent );	// Back up to the marker from the end of the APP0 copy loop.
	
	while ( true ) {

		if ( checkAbort && abortProc(abortArg) ) {
			XMP_Throw ( "JPEG_MetaHandler::WriteFile - User abort", kXMPErr_UserAbort );
		}

		if ( ! XIO::CheckFileSpace ( origRef, 2 ) ) break;	// Tolerate a file that ends abruptly.
		
		marker = XIO::ReadUns16_BE ( origRef );	// Read the next marker.
		if ( marker == 0xFFFF ) {
			// Have a pad byte, skip it. These are almost unheard of, so efficiency isn't critical.
			origRef->Seek ( -1, kXMP_SeekFromCurrent );	// Skip the first 0xFF, read the second again.
			continue;
		}

		if ( (marker == 0xFFDA) || (marker == 0xFFD9) ) {	// Quit at the first SOS marker or at EOI.
			origRef->Seek ( -2, kXMP_SeekFromCurrent );	// The tail copy must include this marker.
			break;
		}

		if ( (marker == 0xFF01) ||	// Ill-formed file if we encounter a TEM or RSTn marker.
			 ((0xFFD0 <= marker) && (marker <= 0xFFD7)) ) {
			XMP_Throw ( "Unexpected TEM or RSTn marker", kXMPErr_BadJPEG );
		}

		contentLen = XIO::ReadUns16_BE ( origRef );	// Read this segment's length.
		if ( contentLen < 2 ) XMP_Throw ( "Invalid JPEG segment length", kXMPErr_BadJPEG );
		contentLen -= 2;	// Reduce to just the content length.
		
		XMP_Int64 contentOrigin = origRef->Offset();
		bool copySegment = true;
		size_t signatureLen;

		if ( (marker == 0xFFED) && (contentLen >= kPSIRSignatureLength) ) {

			// This is an APP13 segment, skip if it is the old PSIR.
			signatureLen = origRef->Read ( buffer, kPSIRSignatureLength );
			if ( (signatureLen == kPSIRSignatureLength) &&
				 CheckBytes ( &buffer[0], kPSIRSignatureString, kPSIRSignatureLength ) ) {
				copySegment = false;
			}

		} else if ( (marker == 0xFFE1) && (contentLen >= kExifSignatureLength) ) {	// Check for the shortest signature.

			// This is an APP1 segment, skip if it is the old Exif or XMP.
			
			XMP_Assert ( (kExifSignatureLength < kMainXMPSignatureLength) &&
						 (kMainXMPSignatureLength < kExtXMPSignatureLength) );
			signatureLen = origRef->Read ( buffer, kExtXMPSignatureLength );	// Read for the longest signature.

			if ( (signatureLen >= kExifSignatureLength) &&
				 (CheckBytes ( &buffer[0], kExifSignatureString, kExifSignatureLength ) ||
				  CheckBytes ( &buffer[0], kExifSignatureAltStr, kExifSignatureLength )) ) {
				copySegment = false;
			}
			
			if ( copySegment && (signatureLen >= kMainXMPSignatureLength) &&
				 CheckBytes ( &buffer[0], kMainXMPSignatureString, kMainXMPSignatureLength ) ) {
				copySegment = false;
			}
			
			if ( copySegment && (signatureLen == kExtXMPSignatureLength) &&
				 CheckBytes ( &buffer[0], kExtXMPSignatureString, kExtXMPPrefixLength ) ) {
				copySegment = false;
			}
			
		}
		
		if ( ! copySegment ) {
			origRef->Seek ( (contentOrigin + contentLen), kXMP_SeekFromStart );
		} else {
			XIO::WriteUns16_BE ( tempRef, marker );
			XIO::WriteUns16_BE ( tempRef, (contentLen + 2) );
			origRef->Seek ( contentOrigin, kXMP_SeekFromStart );
			origRef->ReadAll ( buffer, contentLen );
			tempRef->Write ( buffer, contentLen );
		}

	}

	// Copy the remainder of the source file.

	XIO::Copy ( origRef, tempRef, (origLength - origRef->Offset()) );
	this->needsUpdate = false;

}	// JPEG_MetaHandler::WriteTempFile
