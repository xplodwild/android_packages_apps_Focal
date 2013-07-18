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

#include "XMPFiles/source/FileHandlers/PSD_Handler.hpp"

#include "XMPFiles/source/FormatSupport/TIFF_Support.hpp"
#include "XMPFiles/source/FormatSupport/PSIR_Support.hpp"
#include "XMPFiles/source/FormatSupport/IPTC_Support.hpp"
#include "XMPFiles/source/FormatSupport/ReconcileLegacy.hpp"
#include "XMPFiles/source/FormatSupport/Reconcile_Impl.hpp"

#include "third-party/zuid/interfaces/MD5.h"

using namespace std;

// =================================================================================================
/// \file PSD_Handler.cpp
/// \brief File format handler for PSD (Photoshop).
///
/// This handler ...
///
// =================================================================================================

// =================================================================================================
// PSD_CheckFormat
// ===============

// For PSD we just check the "8BPS" signature, the following version, and that the file is at least
// 34 bytes long. This covers the 26 byte header, the 4 byte color mode section length (which might
// be 0), and the 4 byte image resource section length (which might be 0). The parsing logic in
// CacheFileData will do further checks that the image resources actually exist. Those checks are
// not needed to decide if this is a PSD file though, instead they decide if this is valid PSD.

// ! The CheckXyzFormat routines don't track the filePos, that is left to ScanXyzFile.

bool PSD_CheckFormat ( XMP_FileFormat format,
	                   XMP_StringPtr  filePath,
                       XMP_IO*    	  fileRef,
                       XMPFiles *     parent )
{
	IgnoreParam(format); IgnoreParam(filePath); IgnoreParam(parent);
	XMP_Assert ( format == kXMP_PhotoshopFile );

	fileRef->Rewind ( );
	if ( fileRef->Length() < 34 ) return false;	// 34 = header plus 2 lengths

	XMP_Uns8 buffer [4];
	fileRef->ReadAll ( buffer, 4 );
	if ( ! CheckBytes ( buffer, "8BPS", 4 ) ) return false;
	XMP_Uns16 version = XIO::ReadUns16_BE ( fileRef );
	if ( (version != 1) && (version != 2) ) return false;

	return true;

}	// PSD_CheckFormat

// =================================================================================================
// PSD_MetaHandlerCTor
// ===================

XMPFileHandler * PSD_MetaHandlerCTor ( XMPFiles * parent )
{
	return new PSD_MetaHandler ( parent );

}	// PSD_MetaHandlerCTor

// =================================================================================================
// PSD_MetaHandler::PSD_MetaHandler
// ================================

PSD_MetaHandler::PSD_MetaHandler ( XMPFiles * _parent ) : iptcMgr(0), exifMgr(0), skipReconcile(false)
{
	this->parent = _parent;
	this->handlerFlags = kPSD_HandlerFlags;
	this->stdCharForm  = kXMP_Char8Bit;

}	// PSD_MetaHandler::PSD_MetaHandler

// =================================================================================================
// PSD_MetaHandler::~PSD_MetaHandler
// =================================

PSD_MetaHandler::~PSD_MetaHandler()
{

	if ( this->iptcMgr != 0 ) delete ( this->iptcMgr );
	if ( this->exifMgr != 0 ) delete ( this->exifMgr );

}	// PSD_MetaHandler::~PSD_MetaHandler

// =================================================================================================
// PSD_MetaHandler::CacheFileData
// ==============================
//
// Find and parse the image resource section, everything we want is in there. Don't simply capture
// the whole section, there could be lots of stuff we don't care about.

// *** This implementation simply returns when an invalid file is encountered. Should we throw instead?

void PSD_MetaHandler::CacheFileData()
{
	XMP_IO*      fileRef    = this->parent->ioRef;
	XMP_PacketInfo & packetInfo = this->packetInfo;

	XMP_AbortProc abortProc  = this->parent->abortProc;
	void *        abortArg   = this->parent->abortArg;
	const bool    checkAbort = (abortProc != 0);

	XMP_Assert ( ! this->containsXMP );
	// Set containsXMP to true here only if the XMP image resource is found.

	if ( checkAbort && abortProc(abortArg) ) {
		XMP_Throw ( "PSD_MetaHandler::CacheFileData - User abort", kXMPErr_UserAbort );
	}

	XMP_Uns8  psdHeader[30];
	XMP_Uns32 ioLen, cmLen;

	XMP_Int64 filePos = 0;
	fileRef->Rewind ( );

	ioLen = fileRef->Read ( psdHeader, 30 );
	if ( ioLen != 30 ) return;	// Throw?

	this->imageHeight = GetUns32BE ( &psdHeader[14] );
	this->imageWidth  = GetUns32BE ( &psdHeader[18] );

	cmLen = GetUns32BE ( &psdHeader[26] );

	XMP_Int64 psirOrigin = 26 + 4 + cmLen;

	filePos = fileRef->Seek ( psirOrigin, kXMP_SeekFromStart  );
	if ( filePos !=  psirOrigin ) return;	// Throw?

	if ( ! XIO::CheckFileSpace ( fileRef, 4 ) ) return;	// Throw?
	XMP_Uns32 psirLen = XIO::ReadUns32_BE ( fileRef );

	this->psirMgr.ParseFileResources ( fileRef, psirLen );

	PSIR_Manager::ImgRsrcInfo xmpInfo;
	bool found = this->psirMgr.GetImgRsrc ( kPSIR_XMP, &xmpInfo );

	if ( found ) {

		// printf ( "PSD_MetaHandler::CacheFileData - XMP packet offset %d (0x%X), size %d\n",
		//		  xmpInfo.origOffset, xmpInfo.origOffset, xmpInfo.dataLen );
		this->packetInfo.offset = xmpInfo.origOffset;
		this->packetInfo.length = xmpInfo.dataLen;
		this->packetInfo.padSize   = 0;				// Assume for now, set these properly in ProcessXMP.
		this->packetInfo.charForm  = kXMP_CharUnknown;
		this->packetInfo.writeable = true;

		this->xmpPacket.assign ( (XMP_StringPtr)xmpInfo.dataPtr, xmpInfo.dataLen );

		this->containsXMP = true;

	}

}	// PSD_MetaHandler::CacheFileData

// =================================================================================================
// PSD_MetaHandler::ProcessXMP
// ===========================
//
// Process the raw XMP and legacy metadata that was previously cached.

void PSD_MetaHandler::ProcessXMP()
{

	this->processedXMP = true;	// Make sure we only come through here once.

	// Set up everything for the legacy import, but don't do it yet. This lets us do a forced legacy
	// import if the XMP packet gets parsing errors.

	bool readOnly = ((this->parent->openFlags & kXMPFiles_OpenForUpdate) == 0);

	if ( readOnly ) {
		this->iptcMgr = new IPTC_Reader();
		this->exifMgr = new TIFF_MemoryReader();
	} else {
		this->iptcMgr = new IPTC_Writer();	// ! Parse it later.
		this->exifMgr = new TIFF_FileWriter();
	}
	if ( this->parent )
		exifMgr->SetErrorCallback( &this->parent->errorCallback );

	PSIR_Manager & psir = this->psirMgr;	// Give the compiler help in recognizing non-aliases.
	IPTC_Manager & iptc = *this->iptcMgr;
	TIFF_Manager & exif = *this->exifMgr;

	PSIR_Manager::ImgRsrcInfo iptcInfo, exifInfo;
	bool haveIPTC = psir.GetImgRsrc ( kPSIR_IPTC, &iptcInfo );
	bool haveExif = psir.GetImgRsrc ( kPSIR_Exif, &exifInfo );
	int iptcDigestState = kDigestMatches;

	if ( haveExif ) exif.ParseMemoryStream ( exifInfo.dataPtr, exifInfo.dataLen );

	if ( haveIPTC ) {

		bool haveDigest = false;
		PSIR_Manager::ImgRsrcInfo digestInfo;
		haveDigest = psir.GetImgRsrc ( kPSIR_IPTCDigest, &digestInfo );
		if ( digestInfo.dataLen != 16 ) haveDigest = false;

		if ( ! haveDigest ) {
			iptcDigestState = kDigestMissing;
		} else {
			iptcDigestState = PhotoDataUtils::CheckIPTCDigest ( iptcInfo.dataPtr, iptcInfo.dataLen, digestInfo.dataPtr );
		}

	}

	XMP_OptionBits options = 0;
	if ( this->containsXMP ) options |= k2XMP_FileHadXMP;
	if ( haveIPTC ) options |= k2XMP_FileHadIPTC;
	if ( haveExif ) options |= k2XMP_FileHadExif;

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
	ImportPhotoData ( exif, iptc, psir, iptcDigestState, &this->xmpObj, options );
	this->containsXMP = true;	// Assume we now have something in the XMP.

}	// PSD_MetaHandler::ProcessXMP

// =================================================================================================
// PSD_MetaHandler::UpdateFile
// ===========================

void PSD_MetaHandler::UpdateFile ( bool doSafeUpdate )
{
	XMP_Assert ( ! doSafeUpdate );	// This should only be called for "unsafe" updates.

	XMP_Int64 oldPacketOffset = this->packetInfo.offset;
	XMP_Int32 oldPacketLength = this->packetInfo.length;

	if ( oldPacketOffset == kXMPFiles_UnknownOffset ) oldPacketOffset = 0;	// ! Simplify checks.
	if ( oldPacketLength == kXMPFiles_UnknownLength ) oldPacketLength = 0;

	bool fileHadXMP = ((oldPacketOffset != 0) && (oldPacketLength != 0));

	// Update the IPTC-IIM and native TIFF/Exif metadata. ExportPhotoData also trips the tiff: and
	// exif: copies from the XMP, so reserialize the now final XMP packet.

	ExportPhotoData ( kXMP_PhotoshopFile, &this->xmpObj, this->exifMgr, this->iptcMgr, &this->psirMgr );

	try {
		XMP_OptionBits options = kXMP_UseCompactFormat;
		if ( fileHadXMP ) options |= kXMP_ExactPacketLength;
		this->xmpObj.SerializeToBuffer ( &this->xmpPacket, options, oldPacketLength );
	} catch ( ... ) {
		this->xmpObj.SerializeToBuffer ( &this->xmpPacket, kXMP_UseCompactFormat );
	}

	// Decide whether to do an in-place update. This can only happen if all of the following are true:
	//	- There is an XMP packet in the file.
	//	- The are no changes to the legacy image resources. (The IPTC and EXIF are in the PSIR.)
	//	- The new XMP can fit in the old space.

	bool doInPlace = (fileHadXMP && (this->xmpPacket.size() <= (size_t)oldPacketLength));
	if ( this->psirMgr.IsLegacyChanged() ) doInPlace = false;
	XMP_ProgressTracker* progressTracker = this->parent->progressTracker;

	if ( doInPlace ) {

		#if GatherPerformanceData
			sAPIPerf->back().extraInfo += ", PSD in-place update";
		#endif

		if ( this->xmpPacket.size() < (size_t)this->packetInfo.length ) {
			// They ought to match, cheap to be sure.
			size_t extraSpace = (size_t)this->packetInfo.length - this->xmpPacket.size();
			this->xmpPacket.append ( extraSpace, ' ' );
		}

		XMP_IO* liveFile = this->parent->ioRef;

		XMP_Assert ( this->xmpPacket.size() == (size_t)oldPacketLength );	// ! Done by common PutXMP logic.

		if ( progressTracker != 0 ) progressTracker->BeginWork ( this->xmpPacket.size() );
		liveFile->Seek ( oldPacketOffset, kXMP_SeekFromStart  );
		liveFile->Write ( this->xmpPacket.c_str(), (XMP_StringLen)this->xmpPacket.size() );
		if ( progressTracker != 0 ) progressTracker->WorkComplete();

	} else {

		#if GatherPerformanceData
			sAPIPerf->back().extraInfo += ", PSD copy update";
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

}	// PSD_MetaHandler::UpdateFile

// =================================================================================================
// PSD_MetaHandler::WriteTempFile
// ==============================

// The metadata parts of a Photoshop file are all in the image resources. The PSIR_Manager's
// UpdateFileResources method will take care of the image resource portion of the file, updating
// those resources that have changed and preserving those that have not.

void PSD_MetaHandler::WriteTempFile ( XMP_IO* tempRef )
{
	XMP_IO* origRef = this->parent->ioRef;

	XMP_AbortProc abortProc  = this->parent->abortProc;
	void *        abortArg   = this->parent->abortArg;
	const bool    checkAbort = (abortProc != 0);
	XMP_ProgressTracker* progressTracker = this->parent->progressTracker;

	XMP_Uns64 sourceLen = origRef->Length();
	if ( sourceLen == 0 ) return;	// Tolerate empty files.

	// Reconcile the legacy metadata, unless this is called from UpdateFile. Reserialize the XMP to
	// get standard padding, PutXMP has probably done an in-place serialize. Set the XMP image resource.

	if ( ! skipReconcile ) {
		// Update the IPTC-IIM and native TIFF/Exif metadata, and reserialize the now final XMP packet.
		ExportPhotoData ( kXMP_JPEGFile, &this->xmpObj, this->exifMgr, this->iptcMgr, &this->psirMgr );
		this->xmpObj.SerializeToBuffer ( &this->xmpPacket, kXMP_UseCompactFormat );
	}

	this->xmpObj.SerializeToBuffer ( &this->xmpPacket, kXMP_UseCompactFormat );
	this->packetInfo.offset = kXMPFiles_UnknownOffset;
	this->packetInfo.length = (XMP_StringLen)this->xmpPacket.size();
	FillPacketInfo ( this->xmpPacket, &this->packetInfo );

	this->psirMgr.SetImgRsrc ( kPSIR_XMP, this->xmpPacket.c_str(), (XMP_StringLen)this->xmpPacket.size() );
	
	// Calculate the total writes I/O to be done by this method. This includes header section, color
	// mode section and tail length after the image resources section. The write I/O for image
	// resources section is added to total work in PSIR_FileWriter::UpdateFileResources.

	origRef->Seek ( 26, kXMP_SeekFromStart );	//move to the point after Header 26 is the header length

	XMP_Uns32 cmLen,cmLen1;
	origRef->Read ( &cmLen, 4 );	// get the length of color mode section
	cmLen1 = GetUns32BE ( &cmLen );
	origRef->Seek ( cmLen1, kXMP_SeekFromCurrent );	//move to the end of color mode section
	
	XMP_Uns32 irLen;
	origRef->Read ( &irLen, 4 );	// Get the source image resource section length.
	irLen = GetUns32BE ( &irLen );
		
	XMP_Uns64 tailOffset = 26 + 4 + cmLen1 + 4 + irLen;
	XMP_Uns64 tailLength = sourceLen - tailOffset;
	
	// Add work for 26 bytes header, 4 bytes color mode section length, color mode section length
	// and tail length after the image resources section length.

	if ( progressTracker != 0 ) progressTracker->BeginWork ( (float)(26.0f + 4.0f + cmLen1 + tailLength) );

	// Copy the file header and color mode section, then write the updated image resource section,
	// and copy the tail of the source file (layer and mask section to EOF).

	origRef->Rewind ( );
	tempRef->Truncate ( 0  );
	XIO::Copy ( origRef, tempRef, 26 );	// Copy the file header.

	origRef->Seek ( 4, kXMP_SeekFromCurrent );
	tempRef->Write ( &cmLen, 4 );	// Copy the color mode section length.

	XIO::Copy ( origRef, tempRef, cmLen1 );	// Copy the color mode section contents.

	this->psirMgr.UpdateFileResources ( origRef, tempRef, abortProc, abortArg ,progressTracker );

	origRef->Seek ( tailOffset, kXMP_SeekFromStart  );
	tempRef->Seek ( 0, kXMP_SeekFromEnd  );
	XIO::Copy ( origRef, tempRef, tailLength );	// Copy the tail of the file.

	this->needsUpdate = false;
	if ( progressTracker != 0 ) progressTracker->WorkComplete();

}	// PSD_MetaHandler::WriteTempFile

// =================================================================================================
