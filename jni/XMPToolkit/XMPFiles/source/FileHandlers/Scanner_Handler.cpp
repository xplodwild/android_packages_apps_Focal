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
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "source/XMPFiles_IO.hpp"
#include "source/XIO.hpp"

#include "XMPFiles/source/FormatSupport/XMPScanner.hpp"
#include "XMPFiles/source/FileHandlers/Scanner_Handler.hpp"

#include <vector>

using namespace std;

#if EnablePacketScanning

// =================================================================================================
/// \file Scanner_Handler.cpp
/// \brief File format handler for packet scanning.
///
/// This header ...
///
// =================================================================================================

struct CandidateInfo {
	XMP_PacketInfo packetInfo;
	std::string    xmpPacket;
	SXMPMeta *     xmpObj;
};

// =================================================================================================
// Scanner_MetaHandlerCTor
// =======================

XMPFileHandler * Scanner_MetaHandlerCTor ( XMPFiles * parent )
{
	return new Scanner_MetaHandler ( parent );

}	// Scanner_MetaHandlerCTor

// =================================================================================================
// Scanner_MetaHandler::Scanner_MetaHandler
// ========================================

Scanner_MetaHandler::Scanner_MetaHandler ( XMPFiles * _parent )
{
	this->parent = _parent;
	this->handlerFlags = kScanner_HandlerFlags;

}	// Scanner_MetaHandler::Scanner_MetaHandler

// =================================================================================================
// Scanner_MetaHandler::~Scanner_MetaHandler
// =========================================

Scanner_MetaHandler::~Scanner_MetaHandler()
{
	// ! Inherit the base cleanup.

}	// Scanner_MetaHandler::~Scanner_MetaHandler

// =================================================================================================
// PickMainPacket
// ==============
//
// Pick the main packet from the vector of candidates. The rules:
//	1. Use the manifest find containment. Prune contained packets.
//	2. Use the metadata date to pick the most recent.
//	3. if lenient, pick the last writeable packet, or the last if all are read only.

static int
PickMainPacket ( std::vector<CandidateInfo>& candidates, bool beLenient )
{
	int pkt;		// ! Must be signed.
	int	main = -1;	// Assume the worst.
	XMP_OptionBits options;

	int metaCount = (int)candidates.size();
	if ( metaCount == 0 ) return -1;
	if ( metaCount == 1 ) return 0;

	// ---------------------------------------------------------------------------------------------
	// 1. Look at each packet to see if it has a manifest. If it does, prune all of the others that
	// this one says it contains. Hopefully we'll end up with just one packet. Note that we have to
	// mark all the children first, then prune. Pruning on the fly means that we won't do a proper
	// tree discovery if we prune a parent before a child. This would happen if we happened to visit
	// a grandparent first.

	int child;

	std::vector<bool> pruned ( metaCount, false );

	for ( pkt = 0; pkt < (int)candidates.size(); ++pkt ) {

		// First see if this candidate has a manifest.

		try {
			std::string voidValue;
			bool found = candidates[pkt].xmpObj->GetProperty ( kXMP_NS_XMP_MM, "Manifest", &voidValue, &options );
			if ( (! found) || (! XMP_PropIsArray ( options )) ) continue;	// No manifest, or not an array.
		} catch ( ... ) {
			continue;	// No manifest.
		};

		// Mark all other candidates that are referred to in this manifest.

		for ( child = 0; child < (int)candidates.size(); ++child ) {
			if ( pruned[child] || (child == pkt) ) continue; // Skip already pruned ones and self.
		}

	}

	// Go ahead and actually remove the marked packets.

	for ( pkt = 0; pkt < (int)candidates.size(); ++pkt ) {
		if ( pruned[pkt] ) {
			delete candidates[pkt].xmpObj;
			candidates[pkt].xmpObj = 0;
			metaCount -= 1;
		}
	}

	// We're done if the containment pruning left us with 0 or 1 candidate.

	if ( metaCount == 0 ) {
		XMP_Throw ( "GetMainPacket/PickMainPacket: Recursive containment", kXMPErr_BadXMP );
	} else if ( metaCount == 1 ) {
		for ( pkt = 0; pkt < (int)candidates.size(); ++pkt ) {
			if ( candidates[pkt].xmpObj != 0 ) {
				main = pkt;
				break;
			}
		}
	}

	if ( main != -1 ) return main;	// We found the main.

	// -------------------------------------------------------------------------------------------
	// 2. Pick the packet with the most recent metadata date. If we are being lenient then missing
	// dates are older than any real date, and equal dates pick the last packet. If we are being
	// strict then any missing or equal dates mean we can't pick.

	XMP_DateTime latestTime, currTime;

	for ( pkt = 0; pkt < (int)candidates.size(); ++pkt ) {

		if ( candidates[pkt].xmpObj == 0 ) continue;	// This was pruned in the manifest stage.

		bool haveDate = candidates[pkt].xmpObj->GetProperty_Date ( kXMP_NS_XMP, "MetadataDate", &currTime, &options );

		if ( ! haveDate ) {

			if ( ! beLenient ) return -1;
			if ( main == -1 ) {
				main = pkt;
				memset ( &latestTime, 0, sizeof(latestTime) );
			}

		} else if ( main == -1 ) {

			main = pkt;
			latestTime = currTime;

		} else {

			int timeOp = SXMPUtils::CompareDateTime ( currTime, latestTime );

			if ( timeOp > 0 ) {
				main = pkt;
				latestTime = currTime;
			} else if ( timeOp == 0 ) {
				if ( ! beLenient ) return -1;
				main = pkt;
				latestTime = currTime;
			}

		}

	}

	if ( main != -1 ) return main;	// We found the main.

	// --------------------------------------------------------------------------------------------
	// 3. If we're being lenient, pick the last writeable packet, or the last if all are read only.

	if ( beLenient ) {

		for ( pkt = (int)candidates.size()-1; pkt >= 0; --pkt ) {
			if ( candidates[pkt].xmpObj == 0 ) continue;	// This was pruned in the manifest stage.
			if ( candidates[pkt].packetInfo.writeable ) {
				main = pkt;
				break;
			}
		}

		if ( main == -1 ) {
			for ( pkt = (int)candidates.size()-1; pkt >= 0; --pkt ) {
				if ( candidates[pkt].xmpObj != 0 ) {
					main = pkt;
					break;
				}
			}
		}

	}

	return main;

}	// PickMainPacket

// =================================================================================================
// Scanner_MetaHandler::CacheFileData
// ==================================

void Scanner_MetaHandler::CacheFileData()
{
	XMP_IO* fileRef   = this->parent->ioRef;
	bool        beLenient = XMP_OptionIsClear ( this->parent->openFlags, kXMPFiles_OpenStrictly );

	int			pkt;
	XMP_Int64	bufPos;
	size_t		bufLen;
	SXMPMeta *	newMeta;

	XMP_AbortProc abortProc  = this->parent->abortProc;
	void *            abortArg   = this->parent->abortArg;
	const bool        checkAbort = (abortProc != 0);

	std::vector<CandidateInfo> candidates;	// ! These have SXMPMeta* fields, don't leak on exceptions.

	this->containsXMP = false;

	try {

		// ------------------------------------------------------
		// Scan the entire file to find all of the valid packets.

		XMP_Int64  fileLen = fileRef->Length();
		XMPScanner scanner ( fileLen );

		enum { kBufferSize = 64*1024 };
		XMP_Uns8	buffer [kBufferSize];

		fileRef->Rewind();

		for ( bufPos = 0; bufPos < fileLen; bufPos += bufLen ) {
			if ( checkAbort && abortProc(abortArg) ) {
				XMP_Throw ( "Scanner_MetaHandler::LocateXMP - User abort", kXMPErr_UserAbort );
			}
			bufLen = fileRef->Read ( buffer, kBufferSize );
			if ( bufLen == 0 ) XMP_Throw ( "Scanner_MetaHandler::LocateXMP: Read failure", kXMPErr_ExternalFailure );
			scanner.Scan ( buffer, bufPos, bufLen );
		}

		// --------------------------------------------------------------
		// Parse the valid packet snips, building a vector of candidates.

		long snipCount = scanner.GetSnipCount();

		XMPScanner::SnipInfoVector snips ( snipCount );
		scanner.Report ( snips );

		for ( pkt = 0; pkt < snipCount; ++pkt ) {

			if ( checkAbort && abortProc(abortArg) ) {
				XMP_Throw ( "Scanner_MetaHandler::LocateXMP - User abort", kXMPErr_UserAbort );
			}

			// Seek to the packet then try to parse it.

			if ( snips[pkt].fState != XMPScanner::eValidPacketSnip ) continue;
			fileRef->Seek ( snips[pkt].fOffset, kXMP_SeekFromStart );
			newMeta = new SXMPMeta();
			std::string xmpPacket;
			xmpPacket.reserve ( (size_t)snips[pkt].fLength );

			try {
				for ( bufPos = 0; bufPos < snips[pkt].fLength; bufPos += bufLen ) {
					bufLen = kBufferSize;
					if ( (bufPos + bufLen) > (size_t)snips[pkt].fLength ) bufLen = size_t ( snips[pkt].fLength - bufPos );
					(void) fileRef->ReadAll ( buffer, (XMP_Int32)bufLen );
					xmpPacket.append ( (const char *)buffer, bufLen );
					newMeta->ParseFromBuffer ( (char *)buffer, (XMP_StringLen)bufLen, kXMP_ParseMoreBuffers );
				}
				newMeta->ParseFromBuffer ( 0, 0, kXMP_NoOptions );
			} catch ( ... ) {
				delete newMeta;
				if ( beLenient ) continue;	// Skip if we're being lenient, else rethrow.
				throw;
			}

			// It parsed OK, add it to the array of candidates.

			candidates.push_back ( CandidateInfo() );
			CandidateInfo & newInfo = candidates.back();
			newInfo.xmpObj = newMeta;
			newInfo.xmpPacket.swap ( xmpPacket );
			newInfo.packetInfo.offset = snips[pkt].fOffset;
			newInfo.packetInfo.length = (XMP_Int32)snips[pkt].fLength;
			newInfo.packetInfo.charForm  = snips[pkt].fCharForm;
			newInfo.packetInfo.writeable = (snips[pkt].fAccess == 'w');

		}

		// ----------------------------------------
		// Figure out which packet is the main one.

		int main = PickMainPacket ( candidates, beLenient );

		if ( main != -1 ) {
			this->packetInfo = candidates[main].packetInfo;
			this->xmpPacket.swap ( candidates[main].xmpPacket );
			this->xmpObj = *candidates[main].xmpObj;
			this->containsXMP = true;
			this->processedXMP = true;
		}

		for ( pkt = 0; pkt < (int)candidates.size(); ++pkt ) {
			if ( candidates[pkt].xmpObj != 0 ) delete candidates[pkt].xmpObj;
		}

	} catch ( ... ) {

		// Clean up the SXMPMeta* fields from the vector of candidates.
		for ( pkt = 0; pkt < (int)candidates.size(); ++pkt ) {
			if ( candidates[pkt].xmpObj != 0 ) delete candidates[pkt].xmpObj;
		}
		throw;

	}

}	// Scanner_MetaHandler::CacheFileData

// =================================================================================================

#endif	// IncludePacketScanning
