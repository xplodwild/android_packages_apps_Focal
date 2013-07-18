// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2006 Adobe Systems Incorporated
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

#include "XMPFiles/source/FileHandlers/TIFF_Handler.hpp"

#include "XMPFiles/source/FormatSupport/TIFF_Support.hpp"
#include "XMPFiles/source/FormatSupport/PSIR_Support.hpp"
#include "XMPFiles/source/FormatSupport/IPTC_Support.hpp"
#include "XMPFiles/source/FormatSupport/ReconcileLegacy.hpp"
#include "XMPFiles/source/FormatSupport/Reconcile_Impl.hpp"

#include "third-party/zuid/interfaces/MD5.h"

using namespace std;

// =================================================================================================
/// \file TIFF_Handler.cpp
/// \brief File format handler for TIFF.
///
/// This handler ...
///
// =================================================================================================

// =================================================================================================
// TIFF_CheckFormat
// ================

// For TIFF we just check for the II/42 or MM/42 in the first 4 bytes and that there are at least
// 26 bytes of data (4+4+2+12+4).
//
// ! The CheckXyzFormat routines don't track the filePos, that is left to ScanXyzFile.

bool TIFF_CheckFormat ( XMP_FileFormat format,
	                    XMP_StringPtr  filePath,
                        XMP_IO*    fileRef,
                        XMPFiles *     parent )
{
	IgnoreParam(format); IgnoreParam(filePath); IgnoreParam(parent);
	XMP_Assert ( format == kXMP_TIFFFile );

	enum { kMinimalTIFFSize = 4+4+2+12+4 };	// Header plus IFD with 1 entry.

	fileRef->Rewind ( );
	if ( ! XIO::CheckFileSpace ( fileRef, kMinimalTIFFSize ) ) return false;

	XMP_Uns8 buffer [4];
	fileRef->Read ( buffer, 4 );
	
	bool leTIFF = CheckBytes ( buffer, "\x49\x49\x2A\x00", 4 );
	bool beTIFF = CheckBytes ( buffer, "\x4D\x4D\x00\x2A", 4 );

	return (leTIFF | beTIFF);

}	// TIFF_CheckFormat

// =================================================================================================
// TIFF_MetaHandlerCTor
// ====================

XMPFileHandler * TIFF_MetaHandlerCTor ( XMPFiles * parent )
{
	return new TIFF_MetaHandler ( parent );

}	// TIFF_MetaHandlerCTor

// =================================================================================================
// TIFF_MetaHandler::TIFF_MetaHandler
// ==================================

TIFF_MetaHandler::TIFF_MetaHandler ( XMPFiles * _parent ) : psirMgr(0), iptcMgr(0)
{
	this->parent = _parent;
	this->handlerFlags = kTIFF_HandlerFlags;
	this->stdCharForm  = kXMP_Char8Bit;

}	// TIFF_MetaHandler::TIFF_MetaHandler

// =================================================================================================
// TIFF_MetaHandler::~TIFF_MetaHandler
// ===================================

TIFF_MetaHandler::~TIFF_MetaHandler()
{

	if ( this->psirMgr != 0 ) delete ( this->psirMgr );
	if ( this->iptcMgr != 0 ) delete ( this->iptcMgr );

}	// TIFF_MetaHandler::~TIFF_MetaHandler

// =================================================================================================
// TIFF_MetaHandler::CacheFileData
// ===============================
//
// The data caching for TIFF is easy to explain and implement, but does more processing than one
// might at first expect. This seems unavoidable given the need to close the disk file after calling
// CacheFileData. We parse the TIFF stream and cache the values for all tags of interest, and note
// whether XMP is present. We do not parse the XMP, Photoshop image resources, or IPTC datasets.

// *** This implementation simply returns when invalid TIFF is encountered. Should we throw instead?

void TIFF_MetaHandler::CacheFileData()
{
	XMP_IO*      fileRef    = this->parent->ioRef;
	XMP_PacketInfo & packetInfo = this->packetInfo;

	XMP_AbortProc abortProc  = this->parent->abortProc;
	void *        abortArg   = this->parent->abortArg;
	const bool    checkAbort = (abortProc != 0);

	XMP_Assert ( ! this->containsXMP );
	// Set containsXMP to true here only if the XMP tag is found.

	if ( checkAbort && abortProc(abortArg) ) {
		XMP_Throw ( "TIFF_MetaHandler::CacheFileData - User abort", kXMPErr_UserAbort );
	}

	this->tiffMgr.ParseFileStream ( fileRef );

	TIFF_Manager::TagInfo dngInfo;
	if ( this->tiffMgr.GetTag ( kTIFF_PrimaryIFD, kTIFF_DNGVersion, &dngInfo ) ) {

		// Reject DNG files that are version 2.0 or beyond, this is being written at the time of
		// DNG version 1.2. The DNG team says it is OK to use 2.0, not strictly 1.2. Use the
		// DNGBackwardVersion if it is present, else the DNGVersion. Note that the version value is
		// supposed to be type BYTE, so the file order is always essentially big endian.

		XMP_Uns8 majorVersion = *((XMP_Uns8*)dngInfo.dataPtr);	// Start with DNGVersion.
		if ( this->tiffMgr.GetTag ( kTIFF_PrimaryIFD, kTIFF_DNGBackwardVersion, &dngInfo ) ) {
			majorVersion = *((XMP_Uns8*)dngInfo.dataPtr);	// Use DNGBackwardVersion if possible.
		}
		if ( majorVersion > 1 ) XMP_Throw ( "DNG version beyond 1.x", kXMPErr_BadTIFF );

	}

	TIFF_Manager::TagInfo xmpInfo;
	bool found = this->tiffMgr.GetTag ( kTIFF_PrimaryIFD, kTIFF_XMP, &xmpInfo );

	if ( found ) {

		this->packetInfo.offset    = this->tiffMgr.GetValueOffset ( kTIFF_PrimaryIFD, kTIFF_XMP );
		this->packetInfo.length    = xmpInfo.dataLen;
		this->packetInfo.padSize   = 0;				// Assume for now, set these properly in ProcessXMP.
		this->packetInfo.charForm  = kXMP_CharUnknown;
		this->packetInfo.writeable = true;

		this->xmpPacket.assign ( (XMP_StringPtr)xmpInfo.dataPtr, xmpInfo.dataLen );

		this->containsXMP = true;

	}

}	// TIFF_MetaHandler::CacheFileData

// =================================================================================================
// TIFF_MetaHandler::ProcessXMP
// ============================
//
// Process the raw XMP and legacy metadata that was previously cached. The legacy metadata in TIFF
// is messy because there are 2 copies of the IPTC and because of a Photoshop 6 bug/quirk in the way
// Exif metadata is saved.

void TIFF_MetaHandler::ProcessXMP()
{

	this->processedXMP = true;	// Make sure we only come through here once.

	// Set up everything for the legacy import, but don't do it yet. This lets us do a forced legacy
	// import if the XMP packet gets parsing errors.

	// ! Photoshop 6 wrote annoyingly wacky TIFF files. It buried a lot of the Exif metadata inside
	// ! image resource 1058, itself inside of tag 34377 in the 0th IFD. Take care of this before
	// ! doing any of the legacy metadata presence or priority analysis. Delete image resource 1058
	// ! to get rid of the buried Exif, but don't mark the XMPFiles object as changed. This change
	// ! should not trigger an update, but should be included as part of a normal update.

	bool found;
	bool readOnly = ((this->parent->openFlags & kXMPFiles_OpenForUpdate) == 0);

	if ( readOnly ) {
		this->psirMgr = new PSIR_MemoryReader();
		this->iptcMgr = new IPTC_Reader();
	} else {
		this->psirMgr = new PSIR_FileWriter();
		this->iptcMgr = new IPTC_Writer();	// ! Parse it later.
	}

	TIFF_Manager & tiff = this->tiffMgr;	// Give the compiler help in recognizing non-aliases.
	PSIR_Manager & psir = *this->psirMgr;
	IPTC_Manager & iptc = *this->iptcMgr;

	TIFF_Manager::TagInfo psirInfo;
	bool havePSIR = tiff.GetTag ( kTIFF_PrimaryIFD, kTIFF_PSIR, &psirInfo );

	if ( havePSIR ) {	// ! Do the Photoshop 6 integration before other legacy analysis.
		psir.ParseMemoryResources ( psirInfo.dataPtr, psirInfo.dataLen );
		PSIR_Manager::ImgRsrcInfo buriedExif;
		found = psir.GetImgRsrc ( kPSIR_Exif, &buriedExif );
		if ( found ) {
			tiff.IntegrateFromPShop6 ( buriedExif.dataPtr, buriedExif.dataLen );
			if ( ! readOnly ) psir.DeleteImgRsrc ( kPSIR_Exif );
		}
	}

	TIFF_Manager::TagInfo iptcInfo;
	bool haveIPTC = tiff.GetTag ( kTIFF_PrimaryIFD, kTIFF_IPTC, &iptcInfo );	// The TIFF IPTC tag.
	int iptcDigestState = kDigestMatches;

	if ( haveIPTC ) {

		bool haveDigest = false;
		PSIR_Manager::ImgRsrcInfo digestInfo;
		if ( havePSIR ) haveDigest = psir.GetImgRsrc ( kPSIR_IPTCDigest, &digestInfo );
		if ( digestInfo.dataLen != 16 ) haveDigest = false;

		if ( ! haveDigest ) {

			iptcDigestState = kDigestMissing;

		} else {

			// Older versions of Photoshop wrote tag 33723 with type LONG, but ignored the trailing
			// zero padding for the IPTC digest. If the full digest differs, recheck without the padding.

			iptcDigestState = PhotoDataUtils::CheckIPTCDigest ( iptcInfo.dataPtr, iptcInfo.dataLen, digestInfo.dataPtr );

			if ( (iptcDigestState == kDigestDiffers) && (kTIFF_TypeSizes[iptcInfo.type] > 1) ) {
				XMP_Uns8 * endPtr = (XMP_Uns8*)iptcInfo.dataPtr + iptcInfo.dataLen - 1;
				XMP_Uns8 * minPtr = endPtr - kTIFF_TypeSizes[iptcInfo.type] + 1;
				while ( (endPtr >= minPtr) && (*endPtr == 0) ) --endPtr;
				XMP_Uns32 unpaddedLen = (XMP_Uns32) (endPtr - (XMP_Uns8*)iptcInfo.dataPtr + 1);
				iptcDigestState = PhotoDataUtils::CheckIPTCDigest ( iptcInfo.dataPtr, unpaddedLen, digestInfo.dataPtr );
			}

		}

	}

	XMP_OptionBits options = k2XMP_FileHadExif;	// TIFF files are presumed to have Exif legacy.
	if ( haveIPTC ) options |= k2XMP_FileHadIPTC;
	if ( this->containsXMP ) options |= k2XMP_FileHadXMP;

	// Process the XMP packet. If it fails to parse, do a forced legacy import but still throw an
	// exception. This tells the caller that an error happened, but gives them recovered legacy
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

	// Process the legacy metadata.

	if ( haveIPTC && (! haveXMP) && (iptcDigestState == kDigestMatches) ) iptcDigestState = kDigestMissing;
	bool parseIPTC = (iptcDigestState != kDigestMatches) || (! readOnly);
	if ( parseIPTC ) iptc.ParseMemoryDataSets ( iptcInfo.dataPtr, iptcInfo.dataLen );
	ImportPhotoData ( tiff, iptc, psir, iptcDigestState, &this->xmpObj, options );

	this->containsXMP = true;	// Assume we now have something in the XMP.

}	// TIFF_MetaHandler::ProcessXMP

// =================================================================================================
// TIFF_MetaHandler::UpdateFile
// ============================
//
// There is very little to do directly in UpdateFile. ExportXMPtoJTP takes care of setting all of
// the necessary TIFF tags, including things like the 2nd copy of the IPTC in the Photoshop image
// resources in tag 34377. TIFF_FileWriter::UpdateFileStream does all of the update-by-append I/O.

// *** Need to pass the abort proc and arg to TIFF_FileWriter::UpdateFileStream.

void TIFF_MetaHandler::UpdateFile ( bool doSafeUpdate )
{
	XMP_Assert ( ! doSafeUpdate );	// This should only be called for "unsafe" updates.

	XMP_IO*   destRef    = this->parent->ioRef;
	XMP_AbortProc abortProc  = this->parent->abortProc;
	void *        abortArg   = this->parent->abortArg;

	XMP_Int64 oldPacketOffset = this->packetInfo.offset;
	XMP_Int32 oldPacketLength = this->packetInfo.length;

	if ( oldPacketOffset == kXMPFiles_UnknownOffset ) oldPacketOffset = 0;	// ! Simplify checks.
	if ( oldPacketLength == kXMPFiles_UnknownLength ) oldPacketLength = 0;

	bool fileHadXMP = ((oldPacketOffset != 0) && (oldPacketLength != 0));

	// Update the IPTC-IIM and native TIFF/Exif metadata. ExportPhotoData also trips the tiff: and
	// exif: copies from the XMP, so reserialize the now final XMP packet.

	ExportPhotoData ( kXMP_TIFFFile, &this->xmpObj, &this->tiffMgr, this->iptcMgr, this->psirMgr );

	try {
		XMP_OptionBits options = kXMP_UseCompactFormat;
		if ( fileHadXMP ) options |= kXMP_ExactPacketLength;
		this->xmpObj.SerializeToBuffer ( &this->xmpPacket, options, oldPacketLength );
	} catch ( ... ) {
		this->xmpObj.SerializeToBuffer ( &this->xmpPacket, kXMP_UseCompactFormat );
	}

	// Decide whether to do an in-place update. This can only happen if all of the following are true:
	//	- There is an XMP packet in the file.
	//	- The are no changes to the legacy tags. (The IPTC and PSIR are in the TIFF tags.)
	//	- The new XMP can fit in the old space.

	bool doInPlace = (fileHadXMP && (this->xmpPacket.size() <= (size_t)oldPacketLength));
	if ( this->tiffMgr.IsLegacyChanged() ) doInPlace = false;
	
	bool localProgressTracking = false;
	XMP_ProgressTracker* progressTracker = this->parent->progressTracker;

	if ( ! doInPlace ) {

		#if GatherPerformanceData
			sAPIPerf->back().extraInfo += ", TIFF append update";
		#endif

		if ( (progressTracker != 0) && (! progressTracker->WorkInProgress()) ) {
			localProgressTracking = true;
			progressTracker->BeginWork();
		}

		this->tiffMgr.SetTag ( kTIFF_PrimaryIFD, kTIFF_XMP, kTIFF_UndefinedType, (XMP_Uns32)this->xmpPacket.size(), this->xmpPacket.c_str() );
		this->tiffMgr.UpdateFileStream ( destRef, progressTracker );

	} else {

		#if GatherPerformanceData
			sAPIPerf->back().extraInfo += ", TIFF in-place update";
		#endif

		if ( this->xmpPacket.size() < (size_t)this->packetInfo.length ) {
			// They ought to match, cheap to be sure.
			size_t extraSpace = (size_t)this->packetInfo.length - this->xmpPacket.size();
			this->xmpPacket.append ( extraSpace, ' ' );
		}

		XMP_IO* liveFile = this->parent->ioRef;

		XMP_Assert ( this->xmpPacket.size() == (size_t)oldPacketLength );	// ! Done by common PutXMP logic.

		if ( progressTracker != 0 ) {
			if ( progressTracker->WorkInProgress() ) {
				progressTracker->AddTotalWork ( this->xmpPacket.size() );
			} else {
				localProgressTracking = true;
				progressTracker->BeginWork ( this->xmpPacket.size() );
			}
		}

		liveFile->Seek ( oldPacketOffset, kXMP_SeekFromStart  );
		liveFile->Write ( this->xmpPacket.c_str(), (XMP_Int32)this->xmpPacket.size() );

	}
	
	if ( localProgressTracking ) progressTracker->WorkComplete();
	this->needsUpdate = false;

}	// TIFF_MetaHandler::UpdateFile

// =================================================================================================
// TIFF_MetaHandler::WriteTempFile
// ===============================
//
// The structure of TIFF makes it hard to do a sequential source-to-dest copy with interleaved
// updates. So, copy the existing source to the destination and call UpdateFile.

void TIFF_MetaHandler::WriteTempFile ( XMP_IO* tempRef )
{
	XMP_IO* origRef = this->parent->ioRef;

	XMP_AbortProc abortProc  = this->parent->abortProc;
	void *        abortArg   = this->parent->abortArg;

	XMP_Int64 fileLen = origRef->Length();
	if ( fileLen > 0xFFFFFFFFLL ) {	// Check before making a copy of the file.
		XMP_Throw ( "TIFF fles can't exceed 4GB", kXMPErr_BadTIFF );
	}
	
	XMP_ProgressTracker* progressTracker = this->parent->progressTracker;
	if ( progressTracker != 0 ) progressTracker->BeginWork ( (float)fileLen );

	origRef->Rewind ( );
	tempRef->Truncate ( 0 );
	XIO::Copy ( origRef, tempRef, fileLen, abortProc, abortArg );

	try {
		this->parent->ioRef = tempRef;	// ! Make UpdateFile update the temp.
		this->UpdateFile ( false );
		this->parent->ioRef = origRef;
	} catch ( ... ) {
		this->parent->ioRef = origRef;
		throw;
	}
	
	if ( progressTracker != 0 ) progressTracker->WorkComplete();

}	// TIFF_MetaHandler::WriteTempFile

// =================================================================================================
