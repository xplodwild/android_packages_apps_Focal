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

#include "XMPFiles/source/FormatSupport/ReconcileLegacy.hpp"
#include "XMPFiles/source/FormatSupport/Reconcile_Impl.hpp"
#include "source/XIO.hpp"

// =================================================================================================
/// \file ReconcileLegacy.cpp
/// \brief Top level parts of utilities to reconcile between XMP and legacy metadata forms such as
/// TIFF/Exif and IPTC.
///
// =================================================================================================

// =================================================================================================
// ImportPhotoData
// ===============
//
// Import legacy metadata for JPEG, TIFF, and Photoshop files into the XMP. The caller must have
// already done the file specific processing to select the appropriate sources of the TIFF stream,
// the Photoshop image resources, and the IPTC.

#define SaveExifTag(ns,prop)	\
	if ( xmp->DoesPropertyExist ( ns, prop ) ) SXMPUtils::DuplicateSubtree ( *xmp, &savedExif, ns, prop )
#define RestoreExifTag(ns,prop)	\
	if ( savedExif.DoesPropertyExist ( ns, prop ) ) SXMPUtils::DuplicateSubtree ( savedExif, xmp, ns, prop )

void ImportPhotoData ( const TIFF_Manager & exif,
					   const IPTC_Manager & iptc,
					   const PSIR_Manager & psir,
					   int                  iptcDigestState,
					   SXMPMeta *		    xmp,
					   XMP_OptionBits	    options /* = 0 */ )
{
	bool haveXMP  = XMP_OptionIsSet ( options, k2XMP_FileHadXMP );
	bool haveExif = XMP_OptionIsSet ( options, k2XMP_FileHadExif );
	bool haveIPTC = XMP_OptionIsSet ( options, k2XMP_FileHadIPTC );
	
	// Save some new Exif writebacks that can be XMP-only from older versions, delete all of the
	// XMP's tiff: and exif: namespaces (they should only reflect native Exif), then put back the
	// saved writebacks (which might get replaced by the native Exif values in the Import calls).
	// The value of exif:ISOSpeedRatings is saved for special case handling of ISO over 65535.

	bool haveOldExif = true;	// Default to old Exif if no version tag.
	TIFF_Manager::TagInfo tagInfo;
	bool found = exif.GetTag ( kTIFF_ExifIFD, kTIFF_ExifVersion, &tagInfo );
	if ( found && (tagInfo.type == kTIFF_UndefinedType) && (tagInfo.count == 4) ) {
		haveOldExif = (strncmp ( (char*)tagInfo.dataPtr, "0230", 4 ) < 0);
	}
	
	SXMPMeta savedExif;
	
	SaveExifTag ( kXMP_NS_EXIF, "DateTimeOriginal" );
	SaveExifTag ( kXMP_NS_EXIF, "GPSLatitude" );
	SaveExifTag ( kXMP_NS_EXIF, "GPSLongitude" );
	SaveExifTag ( kXMP_NS_EXIF, "GPSTimeStamp" );
	SaveExifTag ( kXMP_NS_EXIF, "GPSAltitude" );
	SaveExifTag ( kXMP_NS_EXIF, "GPSAltitudeRef" );
	SaveExifTag ( kXMP_NS_EXIF, "ISOSpeedRatings" );
	
	SXMPUtils::RemoveProperties ( xmp, kXMP_NS_TIFF, 0, kXMPUtil_DoAllProperties );
	SXMPUtils::RemoveProperties ( xmp, kXMP_NS_EXIF, 0, kXMPUtil_DoAllProperties );
	if ( ! haveOldExif ) SXMPUtils::RemoveProperties ( xmp, kXMP_NS_ExifEX, 0, kXMPUtil_DoAllProperties );

	RestoreExifTag ( kXMP_NS_EXIF, "DateTimeOriginal" );
	RestoreExifTag ( kXMP_NS_EXIF, "GPSLatitude" );
	RestoreExifTag ( kXMP_NS_EXIF, "GPSLongitude" );
	RestoreExifTag ( kXMP_NS_EXIF, "GPSTimeStamp" );
	RestoreExifTag ( kXMP_NS_EXIF, "GPSAltitude" );
	RestoreExifTag ( kXMP_NS_EXIF, "GPSAltitudeRef" );
	RestoreExifTag ( kXMP_NS_EXIF, "ISOSpeedRatings" );

	// Not obvious here, but the logic in PhotoDataUtils follows the MWG reader guidelines.
	
	PhotoDataUtils::ImportPSIR ( psir, xmp, iptcDigestState );

	if ( haveIPTC ) PhotoDataUtils::Import2WayIPTC ( iptc, xmp, iptcDigestState );
	if ( haveExif ) PhotoDataUtils::Import2WayExif ( exif, xmp, iptcDigestState );

	if ( haveExif | haveIPTC ) PhotoDataUtils::Import3WayItems ( exif, iptc, xmp, iptcDigestState );

	// If photoshop:DateCreated does not exist try to create it from exif:DateTimeOriginal.
	
	if ( ! xmp->DoesPropertyExist ( kXMP_NS_Photoshop, "DateCreated" ) ) {
		std::string exifValue;
		bool haveExifDTO = xmp->GetProperty ( kXMP_NS_EXIF, "DateTimeOriginal", &exifValue, 0 );
		if ( haveExifDTO ) xmp->SetProperty ( kXMP_NS_Photoshop, "DateCreated", exifValue.c_str() );
	}

}	// ImportPhotoData

// =================================================================================================
// ExportPhotoData
// ===============

void ExportPhotoData ( XMP_FileFormat destFormat,
					   SXMPMeta *     xmp,
					   TIFF_Manager * exif, // Pass 0 if not wanted.
					   IPTC_Manager * iptc, // Pass 0 if not wanted.
					   PSIR_Manager * psir, // Pass 0 if not wanted.
					   XMP_OptionBits options /* = 0 */ )
{
	XMP_Assert ( (destFormat == kXMP_JPEGFile) || (destFormat == kXMP_TIFFFile) || (destFormat == kXMP_PhotoshopFile) );

	// Do not write IPTC-IIM or PSIR in DNG files (which are a variant of TIFF).

	if ( (destFormat == kXMP_TIFFFile) && (exif != 0) &&
		 exif->GetTag ( kTIFF_PrimaryIFD, kTIFF_DNGVersion, 0 ) ) {

		iptc = 0;	// These prevent calls to ExportIPTC and ExportPSIR.
		psir = 0;

		exif->DeleteTag ( kTIFF_PrimaryIFD, kTIFF_IPTC );	// These remove any existing IPTC and PSIR.
		exif->DeleteTag ( kTIFF_PrimaryIFD, kTIFF_PSIR );

	}

	// Export the individual metadata items to the non-XMP forms. Set the IPTC digest whether or not
	// it changed, it might not have been present or correct before.

	bool iptcChanged = false;	// Save explicitly, internal flag is reset by UpdateMemoryDataSets.

	void *    iptcPtr = 0;
	XMP_Uns32 iptcLen = 0;
	
	if ( iptc != 0 ) {
		PhotoDataUtils::ExportIPTC ( *xmp, iptc );
		iptcChanged = iptc->IsChanged();
		if ( iptcChanged ) iptc->UpdateMemoryDataSets();
		iptcLen = iptc->GetBlockInfo ( &iptcPtr );
		if ( psir != 0 ) PhotoDataUtils::SetIPTCDigest ( iptcPtr, iptcLen, psir );
	}

	if ( exif != 0 ) PhotoDataUtils::ExportExif ( xmp, exif );
	if ( psir != 0 ) PhotoDataUtils::ExportPSIR ( *xmp, psir );

	// Now update the non-XMP collections of metadata according to the file format. Do not update
	// the XMP here, that is done in the file handlers after deciding if an XMP-only in-place
	// update should be done.
	// - JPEG has the IPTC in PSIR 1028, the Exif and PSIR are marker segments.
	// - TIFF has the IPTC and PSIR in primary IFD tags.
	// - PSD has everything in PSIRs.

	if ( destFormat == kXMP_JPEGFile ) {

		if ( iptcChanged && (psir != 0) ) psir->SetImgRsrc ( kPSIR_IPTC, iptcPtr, iptcLen );

	} else if ( destFormat == kXMP_TIFFFile ) {

		XMP_Assert ( exif != 0 );

		if ( iptcChanged ) exif->SetTag ( kTIFF_PrimaryIFD, kTIFF_IPTC, kTIFF_UndefinedType, iptcLen, iptcPtr );

		if ( (psir != 0) && psir->IsChanged() ) {
			void* psirPtr;
			XMP_Uns32 psirLen = psir->UpdateMemoryResources ( &psirPtr );
			exif->SetTag ( kTIFF_PrimaryIFD, kTIFF_PSIR, kTIFF_UndefinedType, psirLen, psirPtr );
		}

	} else if ( destFormat == kXMP_PhotoshopFile ) {

		XMP_Assert ( psir != 0 );

		if ( iptcChanged ) psir->SetImgRsrc ( kPSIR_IPTC, iptcPtr, iptcLen );

		if ( (exif != 0) && exif->IsChanged() ) {
			void* exifPtr;
			XMP_Uns32 exifLen = exif->UpdateMemoryStream ( &exifPtr );
			psir->SetImgRsrc ( kPSIR_Exif, exifPtr, exifLen );
		}

	}
	
	// Strip the tiff: and exif: namespaces from the XMP, we're done with them. Save the Exif
	// ISOSpeedRatings if any of the values are over 0xFFFF, the native tag is SHORT. Lower level
	// code already kept or stripped the XMP form.

	bool haveOldExif = true;	// Default to old Exif if no version tag.
	if ( exif != 0 ) {
		TIFF_Manager::TagInfo tagInfo;
		bool found = exif->GetTag ( kTIFF_ExifIFD, kTIFF_ExifVersion, &tagInfo );
		if ( found && (tagInfo.type == kTIFF_UndefinedType) && (tagInfo.count == 4) ) {
			haveOldExif = (strncmp ( (char*)tagInfo.dataPtr, "0230", 4 ) < 0);
		}
	}
	
	SXMPMeta savedExif;
	SaveExifTag ( kXMP_NS_EXIF, "ISOSpeedRatings" );
	
	SXMPUtils::RemoveProperties ( xmp, kXMP_NS_TIFF, 0, kXMPUtil_DoAllProperties );
	SXMPUtils::RemoveProperties ( xmp, kXMP_NS_EXIF, 0, kXMPUtil_DoAllProperties );
	if ( ! haveOldExif ) SXMPUtils::RemoveProperties ( xmp, kXMP_NS_ExifEX, 0, kXMPUtil_DoAllProperties );

	RestoreExifTag ( kXMP_NS_EXIF, "ISOSpeedRatings" );

}	// ExportPhotoData
