// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2004 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.
#include "public/include/XMP_Const.h"

#include "XMPFiles/source/FileHandlers/InDesign_Handler.hpp"

#include "source/XIO.hpp"

using namespace std;

// =================================================================================================
/// \file InDesign_Handler.cpp
/// \brief File format handler for InDesign files.
///
/// This header ...
///
/// The layout of an InDesign file in terms of the Basic_MetaHandler model is:
///
/// \li The front of the file. This is everything up to the XMP contiguous object section. The file
/// starts with a pair of master pages, followed by the data pages, followed by contiguous object
/// sections, finished with padding to a page boundary.
///
/// \li A prefix for the XMP section. This is the contiguous object header. The offset is
/// (this->packetInfo.offset - this->xmpPrefixSize).
///
/// \li The XMP packet. The offset is this->packetInfo.offset.
///
/// \li A suffix for the XMP section. This is the contiguous object header. The offset is
/// (this->packetInfo.offset + this->packetInfo.length).
///
/// \li Trailing file content. This is the contiguous objects that follow the XMP. The offset is
/// (this->packetInfo.offset + this->packetInfo.length + this->xmpSuffixSize).
///
/// \li The back of the file. This is the final padding to a page boundary. The offset is
/// (this->packetInfo.offset + this->packetInfo.length + this->xmpSuffixSize + this->trailingContentSize).
///
// =================================================================================================

// *** Add PutXMP overrides that throw if the file does not contain XMP.

#ifndef TraceInDesignHandler
	#define TraceInDesignHandler 0
#endif

enum { kInDesignGUIDSize = 16 };

struct InDesignMasterPage {
	XMP_Uns8  fGUID [kInDesignGUIDSize];
	XMP_Uns8  fMagicBytes [8];
	XMP_Uns8  fObjectStreamEndian;
	XMP_Uns8  fIrrelevant1 [239];
	XMP_Uns64 fSequenceNumber;
	XMP_Uns8  fIrrelevant2 [8];
	XMP_Uns32 fFilePages;
	XMP_Uns8  fIrrelevant3 [3812];
};

enum {
	kINDD_PageSize     = 4096,
	kINDD_PageMask     = (kINDD_PageSize - 1),
	kINDD_LittleEndian = 1,
	kINDD_BigEndian    = 2 };

struct InDesignContigObjMarker {
	XMP_Uns8  fGUID [kInDesignGUIDSize];
	XMP_Uns32 fObjectUID;
	XMP_Uns32 fObjectClassID;
	XMP_Uns32 fStreamLength;
	XMP_Uns32 fChecksum;
};

static const XMP_Uns8 * kINDD_MasterPageGUID =
	(const XMP_Uns8 *) "\x06\x06\xED\xF5\xD8\x1D\x46\xE5\xBD\x31\xEF\xE7\xFE\x74\xB7\x1D";

static const XMP_Uns8 * kINDDContigObjHeaderGUID =
	(const XMP_Uns8 *) "\xDE\x39\x39\x79\x51\x88\x4B\x6C\x8E\x63\xEE\xF8\xAE\xE0\xDD\x38";

static const XMP_Uns8 * kINDDContigObjTrailerGUID =
	(const XMP_Uns8 *) "\xFD\xCE\xDB\x70\xF7\x86\x4B\x4F\xA4\xD3\xC7\x28\xB3\x41\x71\x06";

// =================================================================================================
// InDesign_MetaHandlerCTor
// ========================

XMPFileHandler * InDesign_MetaHandlerCTor ( XMPFiles * parent )
{
	return new InDesign_MetaHandler ( parent );

}	// InDesign_MetaHandlerCTor

// =================================================================================================
// InDesign_CheckFormat
// ====================
//
// For InDesign we check that the pair of master pages begin with the 16 byte GUID.

bool InDesign_CheckFormat ( XMP_FileFormat format,
	                        XMP_StringPtr  filePath,
	                        XMP_IO*    fileRef,
	                        XMPFiles *     parent )
{
	IgnoreParam(format); IgnoreParam(filePath); IgnoreParam(parent);
	XMP_Assert ( format == kXMP_InDesignFile );
	XMP_Assert ( strlen ( (const char *) kINDD_MasterPageGUID ) == kInDesignGUIDSize );

	enum { kBufferSize = 2*kINDD_PageSize };
	XMP_Uns8 buffer [kBufferSize];

	XMP_Int64 filePos   = 0;
	XMP_Uns8 * bufPtr   = buffer;
	XMP_Uns8 * bufLimit = bufPtr + kBufferSize;

	fileRef->Rewind();
	size_t bufLen = fileRef->Read ( buffer, kBufferSize );
	if ( bufLen != kBufferSize ) return false;

	if ( ! CheckBytes ( bufPtr, kINDD_MasterPageGUID, kInDesignGUIDSize ) ) return false;
	if ( ! CheckBytes ( bufPtr+kINDD_PageSize, kINDD_MasterPageGUID, kInDesignGUIDSize ) ) return false;

	return true;

}	// InDesign_CheckFormat

// =================================================================================================
// InDesign_MetaHandler::InDesign_MetaHandler
// ==========================================

InDesign_MetaHandler::InDesign_MetaHandler ( XMPFiles * _parent ) : streamBigEndian(0), xmpObjID(0), xmpClassID(0)
{
	this->parent = _parent;
	this->handlerFlags = kInDesign_HandlerFlags;
	this->stdCharForm  = kXMP_Char8Bit;

}	// InDesign_MetaHandler::InDesign_MetaHandler

// =================================================================================================
// InDesign_MetaHandler::~InDesign_MetaHandler
// ===========================================

InDesign_MetaHandler::~InDesign_MetaHandler()
{
	// Nothing to do here.

}	// InDesign_MetaHandler::~InDesign_MetaHandler

// =================================================================================================
// InDesign_MetaHandler::CacheFileData
// ===================================
//
// Look for the XMP in an InDesign database file. This is a paged database using 4K byte pages,
// followed by redundant "contiguous object streams". Each contiguous object stream is a copy of a
// database object stored as a contiguous byte stream. The XMP that we want is one of these.
//
// The first 2 pages of the database are alternating master pages. A generation number is used to
// select the active master page. The master page contains an offset to the start of the contiguous
// object streams. Each of the contiguous object streams contains a header and trailer, allowing
// fast motion from one stream to the next.
//
// There is no unique "what am I" tagging to the contiguous object streams, so we simply pick the
// first one that looks right. At present this is a 4 byte little endian packet size followed by the
// packet.

// ! Note that insertion of XMP is not allowed for InDesign, the XMP must be a contiguous copy of an
// ! internal database object. So we don't set the packet offset to an insertion point if not found.

void InDesign_MetaHandler::CacheFileData()
{
	XMP_IO* fileRef = this->parent->ioRef;
	XMP_PacketInfo & packetInfo = this->packetInfo;

	XMP_Assert ( kINDD_PageSize == sizeof(InDesignMasterPage) );
	static const size_t kBufferSize = (2 * kINDD_PageSize);
	XMP_Uns8 buffer [kBufferSize];

	size_t	 dbPages;
	XMP_Uns8 cobjEndian;

	XMP_AbortProc abortProc  = this->parent->abortProc;
	void *        abortArg   = this->parent->abortArg;
	const bool    checkAbort = (abortProc != 0);

	this->containsXMP = false;

	// ---------------------------------------------------------------------------------
	// Figure out which master page is active and seek to the contiguous object portion.

	{
		fileRef->Rewind();
		fileRef->ReadAll ( buffer, (2 * kINDD_PageSize) );

		InDesignMasterPage * masters = (InDesignMasterPage *) &buffer[0];
		XMP_Uns64 seq0 = GetUns64LE ( (XMP_Uns8 *) &masters[0].fSequenceNumber );
		XMP_Uns64 seq1 = GetUns64LE ( (XMP_Uns8 *) &masters[1].fSequenceNumber );

		dbPages = GetUns32LE ( (XMP_Uns8 *) &masters[0].fFilePages );
		cobjEndian = masters[0].fObjectStreamEndian;
		if ( seq1 > seq0 ) {
			dbPages = GetUns32LE ( (XMP_Uns8 *)  &masters[1].fFilePages );
			cobjEndian = masters[1].fObjectStreamEndian;
		}
	}

	XMP_Assert ( ! this->streamBigEndian );
	if ( cobjEndian == kINDD_BigEndian ) this->streamBigEndian = true;

	// ---------------------------------------------------------------------------------------------
	// Look for the XMP contiguous object. Each contiguous object has a header and trailer, both of
	// the InDesignContigObjMarker structure. The stream size in the header/trailer is the number of
	// data bytes between the header and trailer. The XMP stream begins with a 4 byte size of the
	// XMP packet. Yes, this is the contiguous object data size minus 4 - silly but true. The XMP
	// must have a packet wrapper, the leading xpacket PI is used as the marker of XMP.

	XMP_Int64 cobjPos = (XMP_Int64)dbPages * kINDD_PageSize;	// ! Use a 64 bit multiply!
	cobjPos -= (2 * sizeof(InDesignContigObjMarker));			// ! For the first pass in the loop.
	XMP_Uns32 streamLength = 0;									// ! For the first pass in the loop.

	while ( true ) {

		if ( checkAbort && abortProc(abortArg) ) {
			XMP_Throw ( "InDesign_MetaHandler::LocateXMP - User abort", kXMPErr_UserAbort );
		}

		// Fetch the start of the next stream and check the contiguous object header.
		// ! The writeable bit of fObjectClassID is ignored, we use the packet trailer flag.

		cobjPos += streamLength + (2 * sizeof(InDesignContigObjMarker));
		fileRef->Seek ( cobjPos, kXMP_SeekFromStart );
		fileRef->ReadAll ( buffer, sizeof(InDesignContigObjMarker) );

		const InDesignContigObjMarker * cobjHeader = (const InDesignContigObjMarker *) &buffer[0];
		if ( ! CheckBytes ( Uns8Ptr(&cobjHeader->fGUID), kINDDContigObjHeaderGUID, kInDesignGUIDSize ) ) break;	// Not a contiguous object header.
		this->xmpObjID = cobjHeader->fObjectUID;	// Save these now while the buffer is good.
		this->xmpClassID = cobjHeader->fObjectClassID;
		streamLength = GetUns32LE ( (XMP_Uns8 *) &cobjHeader->fStreamLength );

		// See if this is the XMP stream.

		if ( streamLength < (4 + kUTF8_PacketHeaderLen + kUTF8_PacketTrailerLen) ) continue;	// Too small, can't possibly be XMP.

		fileRef->ReadAll ( buffer, (4 + kUTF8_PacketHeaderLen) );
		XMP_Uns32 innerLength = GetUns32LE ( &buffer[0] );
		if ( this->streamBigEndian ) innerLength = GetUns32BE ( &buffer[0] );
		if ( innerLength != (streamLength - 4) ) {
			// Be tolerant of a mistake with the endian flag.
			innerLength = Flip4 ( innerLength );
			if ( innerLength != (streamLength - 4) ) continue;	// Not legit XMP.
		}

		XMP_Uns8 * chPtr = &buffer[4];
		size_t startLen = strlen((char*)kUTF8_PacketStart);
		size_t idLen = strlen((char*)kUTF8_PacketID);
		
		if ( ! CheckBytes ( chPtr, kUTF8_PacketStart, startLen ) ) continue;
		chPtr += startLen;

		XMP_Uns8 quote = *chPtr;
		if ( (quote != '\'') && (quote != '"') ) continue;
		chPtr += 1;
		if ( *chPtr != quote ) {
			if ( ! CheckBytes ( chPtr, Uns8Ptr("\xEF\xBB\xBF"), 3 ) ) continue;
			chPtr += 3;
		}
		if ( *chPtr != quote ) continue;
		chPtr += 1;

		if ( ! CheckBytes ( chPtr, Uns8Ptr(" id="), 4 ) ) continue;
		chPtr += 4;
		quote = *chPtr;
		if ( (quote != '\'') && (quote != '"') ) continue;
		chPtr += 1;
		if ( ! CheckBytes ( chPtr, kUTF8_PacketID, idLen ) ) continue;
		chPtr += idLen;
		if ( *chPtr != quote ) continue;
		chPtr += 1;

		// We've seen enough, it is the XMP. To fit the Basic_Handler model we need to compute the
		// total size of remaining contiguous objects, the trailingContentSize. We don't use the
		// size to EOF, that would wrongly include the final zero padding for 4KB alignment.

		this->xmpPrefixSize = sizeof(InDesignContigObjMarker) + 4;
		this->xmpSuffixSize = sizeof(InDesignContigObjMarker);
		packetInfo.offset = cobjPos + this->xmpPrefixSize;
		packetInfo.length = innerLength;

		XMP_Int64 tcStart = cobjPos + streamLength + (2 * sizeof(InDesignContigObjMarker));
		while ( true ) {
			if ( checkAbort && abortProc(abortArg) ) {
				XMP_Throw ( "InDesign_MetaHandler::LocateXMP - User abort", kXMPErr_UserAbort );
			}
			cobjPos += streamLength + (2 * sizeof(InDesignContigObjMarker));
			XMP_Uns32 len = fileRef->Read ( buffer, sizeof(InDesignContigObjMarker) );
			if ( len < sizeof(InDesignContigObjMarker) ) break;	// Too small, must be end of file.
			cobjHeader = (const InDesignContigObjMarker *) &buffer[0];
			if ( ! CheckBytes ( Uns8Ptr(&cobjHeader->fGUID), kINDDContigObjHeaderGUID, kInDesignGUIDSize ) ) break;	// Not a contiguous object header.
			streamLength = GetUns32LE ( (XMP_Uns8 *) &cobjHeader->fStreamLength );
		}
		this->trailingContentSize = cobjPos - tcStart;

		#if TraceInDesignHandler
			XMP_Uns32 pktOffset = (XMP_Uns32)this->packetInfo.offset;
			printf ( "Found XMP in InDesign file, offsets:\n" );
			printf ( "  CObj head %X, XMP %X, CObj tail %X, file tail %X, padding %X\n",
					 (pktOffset - this->xmpPrefixSize), pktOffset, (pktOffset + this->packetInfo.length),
					 (pktOffset + this->packetInfo.length + this->xmpSuffixSize),
					 (pktOffset + this->packetInfo.length + this->xmpSuffixSize + (XMP_Uns32)this->trailingContentSize) );
		#endif

		this->containsXMP = true;
		break;

	}

	if ( this->containsXMP ) {
		this->xmpFileOffset = packetInfo.offset;
		this->xmpFileSize = packetInfo.length;
		ReadXMPPacket ( this );
	}

}	// InDesign_MetaHandler::CacheFileData

// =================================================================================================
// InDesign_MetaHandler::WriteXMPPrefix
// ====================================

void InDesign_MetaHandler::WriteXMPPrefix ( XMP_IO* fileRef )
{
	// Write the contiguous object header and the 4 byte length of the XMP packet.

	XMP_Uns32 packetSize = (XMP_Uns32)this->xmpPacket.size();

	InDesignContigObjMarker header;
	memcpy ( header.fGUID, kINDDContigObjHeaderGUID, sizeof(header.fGUID) );	// AUDIT: Use of dest sizeof for length is safe.
	header.fObjectUID = this->xmpObjID;
	header.fObjectClassID = this->xmpClassID;
	header.fStreamLength = MakeUns32LE ( 4 + packetSize );
	header.fChecksum = (XMP_Uns32)(-1);
	fileRef->Write ( &header, sizeof(header) );

	XMP_Uns32 pktLength = MakeUns32LE ( packetSize );
	if ( this->streamBigEndian ) pktLength = MakeUns32BE ( packetSize );
	fileRef->Write ( &pktLength, sizeof(pktLength) );

}	// InDesign_MetaHandler::WriteXMPPrefix

// =================================================================================================
// InDesign_MetaHandler::WriteXMPSuffix
// ====================================

void InDesign_MetaHandler::WriteXMPSuffix ( XMP_IO* fileRef )
{
	// Write the contiguous object trailer.

	XMP_Uns32 packetSize = (XMP_Uns32)this->xmpPacket.size();

	InDesignContigObjMarker trailer;

	memcpy ( trailer.fGUID, kINDDContigObjTrailerGUID, sizeof(trailer.fGUID) );	// AUDIT: Use of dest sizeof for length is safe.
	trailer.fObjectUID = this->xmpObjID;
	trailer.fObjectClassID = this->xmpClassID;
	trailer.fStreamLength = MakeUns32LE ( 4 + packetSize );
	trailer.fChecksum = (XMP_Uns32)(-1);

	fileRef->Write ( &trailer, sizeof(trailer) );

}	// InDesign_MetaHandler::WriteXMPSuffix

// =================================================================================================
// InDesign_MetaHandler::NoteXMPRemoval
// ====================================

void InDesign_MetaHandler::NoteXMPRemoval ( XMP_IO* fileRef )
{
	// Nothing to do.

}	// InDesign_MetaHandler::NoteXMPRemoval

// =================================================================================================
// InDesign_MetaHandler::NoteXMPInsertion
// ======================================

void InDesign_MetaHandler::NoteXMPInsertion ( XMP_IO* fileRef )
{
	// Nothing to do.

}	// InDesign_MetaHandler::NoteXMPInsertion

// =================================================================================================
// InDesign_MetaHandler::CaptureFileEnding
// =======================================

void InDesign_MetaHandler::CaptureFileEnding ( XMP_IO* fileRef )
{
	// Nothing to do. The back of an InDesign file is the final zero padding.

}	// InDesign_MetaHandler::CaptureFileEnding

// =================================================================================================
// InDesign_MetaHandler::RestoreFileEnding
// =======================================

void InDesign_MetaHandler::RestoreFileEnding ( XMP_IO* fileRef )
{
	// Pad the file with zeros to a page boundary.

	XMP_Int64 dataLength = fileRef->Length();
	XMP_Int32 padLength  = (kINDD_PageSize - ((XMP_Int32)dataLength & kINDD_PageMask)) & kINDD_PageMask;

	XMP_Uns8 buffer [kINDD_PageSize];
	memset ( buffer, 0, kINDD_PageSize );
	fileRef->Write ( buffer, padLength );

}	// InDesign_MetaHandler::RestoreFileEnding

// =================================================================================================
