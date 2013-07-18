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

#include "XMPFiles/source/FormatSupport/Reconcile_Impl.hpp"
#include "source/XIO.hpp"

#include <stdio.h>

#if XMP_WinBuild
	#pragma warning ( disable : 4800 )	// forcing value to bool 'true' or 'false' (performance warning)
	#pragma warning ( disable : 4996 )	// '...' was declared deprecated
#endif

// =================================================================================================
/// \file ReconcileIPTC.cpp
/// \brief Utilities to reconcile between XMP and legacy IPTC and PSIR metadata.
///
// =================================================================================================

// =================================================================================================
// NormalizeToCR
// =============

static inline void NormalizeToCR ( std::string * value )
{
	char * strPtr = (char*) value->data();
	char * strEnd = strPtr + value->size();

	for ( ; strPtr < strEnd; ++strPtr ) {
		if ( *strPtr == kLF ) *strPtr = kCR;
	}

}	// NormalizeToCR

// =================================================================================================
// NormalizeToLF
// =============

static inline void NormalizeToLF ( std::string * value )
{
	char * strPtr = (char*) value->data();
	char * strEnd = strPtr + value->size();

	for ( ; strPtr < strEnd; ++strPtr ) {
		if ( *strPtr == kCR ) *strPtr = kLF;
	}

}	// NormalizeToLF

// =================================================================================================
// ComputeIPTCDigest
// =================
//
// Compute a 128 bit (16 byte) MD5 digest of the full IPTC block.

static inline void ComputeIPTCDigest ( const void * iptcPtr, const XMP_Uns32 iptcLen, MD5_Digest * digest )
{
	MD5_CTX context;

	MD5Init ( &context );
	MD5Update ( &context, (XMP_Uns8*)iptcPtr, iptcLen );
	MD5Final ( *digest, &context );

}	// ComputeIPTCDigest;

// =================================================================================================
// PhotoDataUtils::CheckIPTCDigest
// ===============================

int PhotoDataUtils::CheckIPTCDigest ( const void * newPtr, const XMP_Uns32 newLen, const void * oldDigest )
{
	MD5_Digest newDigest;
	ComputeIPTCDigest ( newPtr, newLen, &newDigest );
	if ( memcmp ( &newDigest, oldDigest, 16 ) == 0 ) return kDigestMatches;
	return kDigestDiffers;

}	// PhotoDataUtils::CheckIPTCDigest

// =================================================================================================
// PhotoDataUtils::SetIPTCDigest
// =============================

void PhotoDataUtils::SetIPTCDigest ( void * iptcPtr, XMP_Uns32 iptcLen, PSIR_Manager * psir )
{
	MD5_Digest newDigest;

	ComputeIPTCDigest ( iptcPtr, iptcLen, &newDigest );
	psir->SetImgRsrc ( kPSIR_IPTCDigest, &newDigest, sizeof(newDigest) );

}	// PhotoDataUtils::SetIPTCDigest

// =================================================================================================
// =================================================================================================

// =================================================================================================
// PhotoDataUtils::ImportIPTC_Simple
// =================================

void PhotoDataUtils::ImportIPTC_Simple ( const IPTC_Manager & iptc, SXMPMeta * xmp,
										 XMP_Uns8 id, const char * xmpNS, const char * xmpProp )
{
	std::string utf8Str;
	size_t count = iptc.GetDataSet_UTF8 ( id, &utf8Str );

	if ( count != 0 ) {
		NormalizeToLF ( &utf8Str );
		xmp->SetProperty ( xmpNS, xmpProp, utf8Str.c_str() );
	}

}	// PhotoDataUtils::ImportIPTC_Simple

// =================================================================================================
// PhotoDataUtils::ImportIPTC_LangAlt
// ==================================

void PhotoDataUtils::ImportIPTC_LangAlt ( const IPTC_Manager & iptc, SXMPMeta * xmp,
										  XMP_Uns8 id, const char * xmpNS, const char * xmpProp )
{
	std::string utf8Str;
	size_t count = iptc.GetDataSet_UTF8 ( id, &utf8Str );

	if ( count != 0 ) {
		NormalizeToLF ( &utf8Str );
		xmp->SetLocalizedText ( xmpNS, xmpProp, "", "x-default", utf8Str.c_str() );
	}

}	// PhotoDataUtils::ImportIPTC_LangAlt

// =================================================================================================
// PhotoDataUtils::ImportIPTC_Array
// ================================

void PhotoDataUtils::ImportIPTC_Array ( const IPTC_Manager & iptc, SXMPMeta * xmp,
										XMP_Uns8 id, const char * xmpNS, const char * xmpProp )
{
	std::string utf8Str;
	size_t count = iptc.GetDataSet ( id, 0 );

	xmp->DeleteProperty ( xmpNS, xmpProp );
	
	XMP_OptionBits arrayForm = kXMP_PropArrayIsUnordered;
	if ( XMP_LitMatch ( xmpNS, kXMP_NS_DC ) && XMP_LitMatch ( xmpProp, "creator" ) ) arrayForm = kXMP_PropArrayIsOrdered;

	for ( size_t ds = 0; ds < count; ++ds ) {
		(void) iptc.GetDataSet_UTF8 ( id, &utf8Str, ds );
		NormalizeToLF ( &utf8Str );
		xmp->AppendArrayItem ( xmpNS, xmpProp, arrayForm, utf8Str.c_str() );
	}

}	// PhotoDataUtils::ImportIPTC_Array

// =================================================================================================
// PhotoDataUtils::ImportIPTC_Date
// ===============================
//
// An IPTC (IIM) date is 8 characters, YYYYMMDD. Include the time portion if it is present. The IPTC
// time is HHMMSSxHHMM, where 'x' is '+' or '-'. Be tolerant of some ill-formed dates and times.
// Apparently some non-Adobe apps put strings like "YYYY-MM-DD" or "HH:MM:SSxHH:MM" in the IPTC.
// Allow a missing time zone portion.

// *** The date/time handling differs from the MWG 1.0.1 policy, following a proposed tweak to MWG:
// ***   Exif DateTimeOriginal <-> XMP exif:DateTimeOriginal
// ***   IPTC DateCreated <-> XMP photoshop:DateCreated
// ***   Exif DateTimeDigitized <-> IPTC DigitalCreateDate <-> XMP xmp:CreateDate

void PhotoDataUtils::ImportIPTC_Date ( XMP_Uns8 dateID, const IPTC_Manager & iptc, SXMPMeta * xmp )
{
	XMP_Uns8 timeID;
	XMP_StringPtr xmpNS, xmpProp;
	
	if ( dateID == kIPTC_DateCreated ) {
		timeID  = kIPTC_TimeCreated;
		xmpNS   = kXMP_NS_Photoshop;
		xmpProp = "DateCreated";
	} else if ( dateID == kIPTC_DigitalCreateDate ) {
		timeID  = kIPTC_DigitalCreateTime;
		xmpNS   = kXMP_NS_XMP;
		xmpProp = "CreateDate";
	} else {
		XMP_Throw ( "Unrecognized dateID", kXMPErr_BadParam );
	}
	
	// First gather the date portion.

	IPTC_Manager::DataSetInfo dsInfo;
	size_t count = iptc.GetDataSet ( dateID, &dsInfo );
	if ( count == 0 ) return;

	size_t chPos, digits;
	XMP_DateTime xmpDate;
	memset ( &xmpDate, 0, sizeof(xmpDate) );

	chPos = 0;
	for ( digits = 0; digits < 4; ++digits, ++chPos ) {
		if ( (chPos >= dsInfo.dataLen) || (dsInfo.dataPtr[chPos] < '0') || (dsInfo.dataPtr[chPos] > '9') ) break;
		xmpDate.year = (xmpDate.year * 10) + (dsInfo.dataPtr[chPos] - '0');
	}

	if ( dsInfo.dataPtr[chPos] == '-' ) ++chPos;
	for ( digits = 0; digits < 2; ++digits, ++chPos ) {
		if ( (chPos >= dsInfo.dataLen) || (dsInfo.dataPtr[chPos] < '0') || (dsInfo.dataPtr[chPos] > '9') ) break;
		xmpDate.month = (xmpDate.month * 10) + (dsInfo.dataPtr[chPos] - '0');
	}
	if ( xmpDate.month < 1 ) xmpDate.month = 1;
	if ( xmpDate.month > 12 ) xmpDate.month = 12;

	if ( dsInfo.dataPtr[chPos] == '-' ) ++chPos;
	for ( digits = 0; digits < 2; ++digits, ++chPos ) {
		if ( (chPos >= dsInfo.dataLen) || (dsInfo.dataPtr[chPos] < '0') || (dsInfo.dataPtr[chPos] > '9') ) break;
		xmpDate.day = (xmpDate.day * 10) + (dsInfo.dataPtr[chPos] - '0');
	}
	if ( xmpDate.day < 1 ) xmpDate.day = 1;
	if ( xmpDate.day > 31 ) xmpDate.day = 28;	// Close enough.

	if ( chPos != dsInfo.dataLen ) return;	// The DataSet is ill-formed.
	xmpDate.hasDate = true;

	// Now add the time portion if present.

	count = iptc.GetDataSet ( timeID, &dsInfo );
	if ( count != 0 ) {

		chPos = 0;
		for ( digits = 0; digits < 2; ++digits, ++chPos ) {
			if ( (chPos >= dsInfo.dataLen) || (dsInfo.dataPtr[chPos] < '0') || (dsInfo.dataPtr[chPos] > '9') ) break;
			xmpDate.hour = (xmpDate.hour * 10) + (dsInfo.dataPtr[chPos] - '0');
		}
		if ( xmpDate.hour < 0 ) xmpDate.hour = 0;
		if ( xmpDate.hour > 23 ) xmpDate.hour = 23;

		if ( dsInfo.dataPtr[chPos] == ':' ) ++chPos;
		for ( digits = 0; digits < 2; ++digits, ++chPos ) {
			if ( (chPos >= dsInfo.dataLen) || (dsInfo.dataPtr[chPos] < '0') || (dsInfo.dataPtr[chPos] > '9') ) break;
			xmpDate.minute = (xmpDate.minute * 10) + (dsInfo.dataPtr[chPos] - '0');
		}
		if ( xmpDate.minute < 0 ) xmpDate.minute = 0;
		if ( xmpDate.minute > 59 ) xmpDate.minute = 59;

		if ( dsInfo.dataPtr[chPos] == ':' ) ++chPos;
		for ( digits = 0; digits < 2; ++digits, ++chPos ) {
			if ( (chPos >= dsInfo.dataLen) || (dsInfo.dataPtr[chPos] < '0') || (dsInfo.dataPtr[chPos] > '9') ) break;
			xmpDate.second = (xmpDate.second * 10) + (dsInfo.dataPtr[chPos] - '0');
		}
		if ( xmpDate.second < 0 ) xmpDate.second = 0;
		if ( xmpDate.second > 59 ) xmpDate.second = 59;

		xmpDate.hasTime = true;

		if ( (dsInfo.dataPtr[chPos] != ' ') && (dsInfo.dataPtr[chPos] != 0) ) {	// Tolerate a missing TZ.
		
			if ( dsInfo.dataPtr[chPos] == '+' ) {
				xmpDate.tzSign = kXMP_TimeEastOfUTC;
			} else if ( dsInfo.dataPtr[chPos] == '-' ) {
				xmpDate.tzSign = kXMP_TimeWestOfUTC;
			} else if ( chPos != dsInfo.dataLen ) {
				return;	// The DataSet is ill-formed.
			}
	
			++chPos;	// Move past the time zone sign.
			for ( digits = 0; digits < 2; ++digits, ++chPos ) {
				if ( (chPos >= dsInfo.dataLen) || (dsInfo.dataPtr[chPos] < '0') || (dsInfo.dataPtr[chPos] > '9') ) break;
				xmpDate.tzHour = (xmpDate.tzHour * 10) + (dsInfo.dataPtr[chPos] - '0');
			}
			if ( xmpDate.tzHour < 0 ) xmpDate.tzHour = 0;
			if ( xmpDate.tzHour > 23 ) xmpDate.tzHour = 23;
	
			if ( dsInfo.dataPtr[chPos] == ':' ) ++chPos;
			for ( digits = 0; digits < 2; ++digits, ++chPos ) {
				if ( (chPos >= dsInfo.dataLen) || (dsInfo.dataPtr[chPos] < '0') || (dsInfo.dataPtr[chPos] > '9') ) break;
				xmpDate.tzMinute = (xmpDate.tzMinute * 10) + (dsInfo.dataPtr[chPos] - '0');
			}
			if ( xmpDate.tzMinute < 0 ) xmpDate.tzMinute = 0;
			if ( xmpDate.tzMinute > 59 ) xmpDate.tzMinute = 59;
	
			if ( chPos != dsInfo.dataLen ) return;	// The DataSet is ill-formed.
			xmpDate.hasTimeZone = true;
			
		}

	}

	// Finally, set the XMP property.

	xmp->SetProperty_Date ( xmpNS, xmpProp, xmpDate );

}	// PhotoDataUtils::ImportIPTC_Date

// =================================================================================================
// ImportIPTC_IntellectualGenre
// ============================
//
// Import DataSet 2:04. In the IIM this is a 3 digit number, a colon, and an optional text name.
// Even though the number is the more formal part, the IPTC4XMP rule is that the name is imported to
// XMP and the number is dropped. Also, even though IIMv4.1 says that 2:04 is repeatable, the XMP
// property to which it is mapped is simple.

static void ImportIPTC_IntellectualGenre ( const IPTC_Manager & iptc, SXMPMeta * xmp )
{
	std::string utf8Str;
	size_t count = iptc.GetDataSet_UTF8 ( kIPTC_IntellectualGenre, &utf8Str );

	if ( count == 0 ) return;
	NormalizeToLF ( &utf8Str );

	XMP_StringPtr namePtr = utf8Str.c_str() + 4;

	if ( utf8Str.size() <= 4 ) {
		// No name in the IIM. Look up the number in our list of known genres.
		int i;
		XMP_StringPtr numPtr = utf8Str.c_str();
		for ( i = 0; kIntellectualGenreMappings[i].refNum != 0; ++i ) {
			if ( strncmp ( numPtr, kIntellectualGenreMappings[i].refNum, 3 ) == 0 ) break;
		}
		if ( kIntellectualGenreMappings[i].refNum == 0 ) return;
		namePtr = kIntellectualGenreMappings[i].name;
	}

	xmp->SetProperty ( kXMP_NS_IPTCCore, "IntellectualGenre", namePtr );

}	// ImportIPTC_IntellectualGenre

// =================================================================================================
// ImportIPTC_SubjectCode
// ======================
//
// Import all 2:12 DataSets into an unordered array. In the IIM each DataSet is composed of 5 colon
// separated sections: a provider name, an 8 digit reference number, and 3 optional names for the
// levels of the reference number hierarchy. The IPTC4XMP mapping rule is that only the reference
// number is imported to XMP.

static void ImportIPTC_SubjectCode ( const IPTC_Manager & iptc, SXMPMeta * xmp )
{
	std::string utf8Str;
	size_t count = iptc.GetDataSet_UTF8 ( kIPTC_SubjectCode, 0 );

	for ( size_t ds = 0; ds < count; ++ds ) {

		(void) iptc.GetDataSet_UTF8 ( kIPTC_SubjectCode, &utf8Str, ds );

		char * refNumPtr = (char*) utf8Str.c_str();
		for ( ; (*refNumPtr != ':') && (*refNumPtr != 0); ++refNumPtr ) {}
		if ( *refNumPtr == 0 ) continue;	// This DataSet is ill-formed.

		char * refNumEnd = refNumPtr + 1;
		for ( ; (*refNumEnd != ':') && (*refNumEnd != 0); ++refNumEnd ) {}
		if ( (refNumEnd - refNumPtr) != 8 ) continue;	// This DataSet is ill-formed.
		*refNumEnd = 0;	// Ensure a terminating nul for the reference number portion.

		xmp->AppendArrayItem ( kXMP_NS_IPTCCore, "SubjectCode", kXMP_PropArrayIsUnordered, refNumPtr );

	}

}	// ImportIPTC_SubjectCode

// =================================================================================================
// PhotoDataUtils::Import2WayIPTC
// ==============================

void PhotoDataUtils::Import2WayIPTC ( const IPTC_Manager & iptc, SXMPMeta * xmp, int iptcDigestState )
{
	if ( iptcDigestState == kDigestMatches ) return;	// Ignore the IPTC if the digest matches.

	std::string oldStr, newStr;
	IPTC_Writer oldIPTC;

	if ( iptcDigestState == kDigestDiffers ) {
		PhotoDataUtils::ExportIPTC ( *xmp, &oldIPTC );	// Predict old IPTC DataSets based on the existing XMP.
	}
	
	size_t newCount;
	IPTC_Manager::DataSetInfo newInfo, oldInfo;
	
	for ( size_t i = 0; kKnownDataSets[i].dsNum != 255; ++i ) {

		const DataSetCharacteristics & thisDS = kKnownDataSets[i];
		if ( thisDS.mapForm >= kIPTC_Map3Way ) continue;	// The mapping is handled elsewhere, or not at all.
		
		bool haveXMP = xmp->DoesPropertyExist ( thisDS.xmpNS, thisDS.xmpProp );
		newCount = PhotoDataUtils::GetNativeInfo ( iptc, thisDS.dsNum, iptcDigestState, haveXMP, &newInfo );
		if ( newCount == 0 ) continue;	// GetNativeInfo returns 0 for ignored local text.
		
		if ( iptcDigestState == kDigestMissing ) {
			if ( haveXMP ) continue;	// Keep the existing XMP.
		} else if ( ! PhotoDataUtils::IsValueDifferent ( iptc, oldIPTC, thisDS.dsNum ) ) {
			continue;	// Don't import values that match the previous export.
		}
		
		// The IPTC wins. Delete any existing XMP and import the DataSet.
		
		xmp->DeleteProperty ( thisDS.xmpNS, thisDS.xmpProp );

		try {	// Don't let errors with one stop the others.

			switch ( thisDS.mapForm ) {

				case kIPTC_MapSimple :
					ImportIPTC_Simple ( iptc, xmp, thisDS.dsNum, thisDS.xmpNS, thisDS.xmpProp );
					break;

				case kIPTC_MapLangAlt :
					ImportIPTC_LangAlt ( iptc, xmp, thisDS.dsNum, thisDS.xmpNS, thisDS.xmpProp );
					break;

				case kIPTC_MapArray :
					ImportIPTC_Array ( iptc, xmp, thisDS.dsNum, thisDS.xmpNS, thisDS.xmpProp );
					break;

				case kIPTC_MapSpecial :
					if ( thisDS.dsNum == kIPTC_DateCreated ) {
						PhotoDataUtils::ImportIPTC_Date ( thisDS.dsNum, iptc, xmp );
					} else if ( thisDS.dsNum == kIPTC_IntellectualGenre ) {
						ImportIPTC_IntellectualGenre ( iptc, xmp );
					} else if ( thisDS.dsNum == kIPTC_SubjectCode ) {
						ImportIPTC_SubjectCode ( iptc, xmp );
					} else {
						XMP_Assert ( false );	// Catch mapping errors.
					}
					break;

			}

		} catch ( ... ) {

			// Do nothing, let other imports proceed.
			// ? Notify client?

		}

	}

}	// PhotoDataUtils::Import2WayIPTC

// =================================================================================================
// PhotoDataUtils::ImportPSIR
// ==========================
//
// There are only 2 standalone Photoshop image resources for XMP properties:
//    1034 - Copyright Flag - 0/1 Boolean mapped to xmpRights:Marked.
//    1035 - Copyright URL - Local OS text mapped to xmpRights:WebStatement.

// ! Photoshop does not use a true/false/missing model for PSIR 1034. Instead it essentially uses a
// ! yes/don't-know model when importing. A missing or 0 value for PSIR 1034 cause xmpRights:Marked
// ! to be deleted.

void PhotoDataUtils::ImportPSIR ( const PSIR_Manager & psir, SXMPMeta * xmp, int iptcDigestState )
{
	PSIR_Manager::ImgRsrcInfo rsrcInfo;
	bool import;

	if ( iptcDigestState == kDigestMatches ) return;

	try {	// Don't let errors with one stop the others.
		import = psir.GetImgRsrc ( kPSIR_CopyrightFlag, &rsrcInfo );
		if ( import ) import = (! xmp->DoesPropertyExist ( kXMP_NS_XMP_Rights, "Marked" ));
		if ( import && (rsrcInfo.dataLen == 1) && (*((XMP_Uns8*)rsrcInfo.dataPtr) != 0) ) {
			xmp->SetProperty_Bool ( kXMP_NS_XMP_Rights, "Marked", true );
		}
	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

	try {	// Don't let errors with one stop the others.
		import = psir.GetImgRsrc ( kPSIR_CopyrightURL, &rsrcInfo );
		if ( import ) import = (! xmp->DoesPropertyExist ( kXMP_NS_XMP_Rights, "WebStatement" ));
		if ( import ) {
			std::string utf8;
			if ( ReconcileUtils::IsUTF8 ( rsrcInfo.dataPtr, rsrcInfo.dataLen ) ) {
				utf8.assign ( (char*)rsrcInfo.dataPtr, rsrcInfo.dataLen );
			} else if ( ! ignoreLocalText ) {
				ReconcileUtils::LocalToUTF8 ( rsrcInfo.dataPtr, rsrcInfo.dataLen, &utf8 );
			} else {
				import = false;	// Inhibit the SetProperty call.
			}
			if ( import ) xmp->SetProperty ( kXMP_NS_XMP_Rights, "WebStatement", utf8.c_str() );
		}
	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// PhotoDataUtils::ImportPSIR;

// =================================================================================================
// =================================================================================================

// =================================================================================================
// ExportIPTC_Simple
// =================

static void ExportIPTC_Simple ( const SXMPMeta & xmp, IPTC_Manager * iptc,
								const char * xmpNS, const char * xmpProp, XMP_Uns8 id )
{
	std::string    value;
	XMP_OptionBits xmpFlags;

	bool found = xmp.GetProperty ( xmpNS, xmpProp, &value, &xmpFlags );
	if ( ! found ) {
		iptc->DeleteDataSet ( id );
		return;
	}

	if ( ! XMP_PropIsSimple ( xmpFlags ) ) return;	// ? Complain? Delete the DataSet?

	NormalizeToCR ( &value );

	size_t iptcCount = iptc->GetDataSet ( id, 0 );
	if ( iptcCount > 1 ) iptc->DeleteDataSet ( id );

	iptc->SetDataSet_UTF8 ( id, value.c_str(), (XMP_Uns32)value.size(), 0 );	// ! Don't append a 2nd DataSet!

}	// ExportIPTC_Simple

// =================================================================================================
// ExportIPTC_LangAlt
// ==================

static void ExportIPTC_LangAlt ( const SXMPMeta & xmp, IPTC_Manager * iptc,
								 const char * xmpNS, const char * xmpProp, XMP_Uns8 id )
{
	std::string    value;
	XMP_OptionBits xmpFlags;

	bool found = xmp.GetProperty ( xmpNS, xmpProp, 0, &xmpFlags );
	if ( ! found ) {
		iptc->DeleteDataSet ( id );
		return;
	}

	if ( ! XMP_ArrayIsAltText ( xmpFlags ) ) return;	// ? Complain? Delete the DataSet?

	found = xmp.GetLocalizedText ( xmpNS, xmpProp, "", "x-default", 0, &value, 0 );
	if ( ! found ) {
		iptc->DeleteDataSet ( id );
		return;
	}

	NormalizeToCR ( &value );

	size_t iptcCount = iptc->GetDataSet ( id, 0 );
	if ( iptcCount > 1 ) iptc->DeleteDataSet ( id );

	iptc->SetDataSet_UTF8 ( id, value.c_str(), (XMP_Uns32)value.size(), 0 );	// ! Don't append a 2nd DataSet!

}	// ExportIPTC_LangAlt

// =================================================================================================
// ExportIPTC_Array
// ================
//
// Array exporting needs a bit of care to preserve the detection of XMP-only updates. If the current
// XMP and IPTC array sizes differ, delete the entire IPTC and append all new values. If they match,
// set the individual values in order - which lets SetDataSet apply its no-change optimization.

static void ExportIPTC_Array ( const SXMPMeta & xmp, IPTC_Manager * iptc,
							   const char * xmpNS, const char * xmpProp, XMP_Uns8 id )
{
	std::string    value;
	XMP_OptionBits xmpFlags;

	bool found = xmp.GetProperty ( xmpNS, xmpProp, 0, &xmpFlags );
	if ( ! found ) {
		iptc->DeleteDataSet ( id );
		return;
	}

	if ( ! XMP_PropIsArray ( xmpFlags ) ) return;	// ? Complain? Delete the DataSet?

	XMP_Index xmpCount  = xmp.CountArrayItems ( xmpNS, xmpProp );
	XMP_Index iptcCount = (XMP_Index) iptc->GetDataSet ( id, 0 );

	if ( xmpCount != iptcCount ) iptc->DeleteDataSet ( id );

	for ( XMP_Index ds = 0; ds < xmpCount; ++ds ) {	// ! XMP arrays are indexed from 1, IPTC from 0.

		(void) xmp.GetArrayItem ( xmpNS, xmpProp, ds+1, &value, &xmpFlags );
		if ( ! XMP_PropIsSimple ( xmpFlags ) ) continue;	// ? Complain?

		NormalizeToCR ( &value );

		iptc->SetDataSet_UTF8 ( id, value.c_str(), (XMP_Uns32)value.size(), ds );	// ! Appends if necessary.

	}

}	// ExportIPTC_Array

// =================================================================================================
// ExportIPTC_IntellectualGenre
// ============================
//
// Export DataSet 2:04. In the IIM this is a 3 digit number, a colon, and a text name. Even though
// the number is the more formal part, the IPTC4XMP rule is that the name is imported to XMP and the
// number is dropped. Also, even though IIMv4.1 says that 2:04 is repeatable, the XMP property to
// which it is mapped is simple. Look up the XMP value in a list of known genres to get the number.

static void ExportIPTC_IntellectualGenre ( const SXMPMeta & xmp, IPTC_Manager * iptc )
{
	std::string    xmpValue;
	XMP_OptionBits xmpFlags;

	bool found = xmp.GetProperty ( kXMP_NS_IPTCCore, "IntellectualGenre", &xmpValue, &xmpFlags );
	if ( ! found ) {
		iptc->DeleteDataSet ( kIPTC_IntellectualGenre );
		return;
	}

	if ( ! XMP_PropIsSimple ( xmpFlags ) ) return;	// ? Complain? Delete the DataSet?

	NormalizeToCR ( &xmpValue );

	int i;
	XMP_StringPtr namePtr = xmpValue.c_str();
	for ( i = 0; kIntellectualGenreMappings[i].name != 0; ++i ) {
		if ( strcmp ( namePtr, kIntellectualGenreMappings[i].name ) == 0 ) break;
	}
	if ( kIntellectualGenreMappings[i].name == 0 ) return;	// Not a known genre, don't export it.

	std::string iimValue = kIntellectualGenreMappings[i].refNum;
	iimValue += ':';
	iimValue += xmpValue;

	size_t iptcCount = iptc->GetDataSet ( kIPTC_IntellectualGenre, 0 );
	if ( iptcCount > 1 ) iptc->DeleteDataSet ( kIPTC_IntellectualGenre );

	iptc->SetDataSet_UTF8 ( kIPTC_IntellectualGenre, iimValue.c_str(), (XMP_Uns32)iimValue.size(), 0 );	// ! Don't append a 2nd DataSet!

}	// ExportIPTC_IntellectualGenre

// =================================================================================================
// ExportIPTC_SubjectCode
// ======================
//
// Export 2:12 DataSets from an unordered array. In the IIM each DataSet is composed of 5 colon
// separated sections: a provider name, an 8 digit reference number, and 3 optional names for the
// levels of the reference number hierarchy. The IPTC4XMP mapping rule is that only the reference
// number is imported to XMP. We export with a fixed provider of "IPTC" and no optional names.

static void ExportIPTC_SubjectCode ( const SXMPMeta & xmp, IPTC_Manager * iptc )
{
	std::string    xmpValue, iimValue;
	XMP_OptionBits xmpFlags;

	bool found = xmp.GetProperty ( kXMP_NS_IPTCCore, "SubjectCode", 0, &xmpFlags );
	if ( ! found ) {
		iptc->DeleteDataSet ( kIPTC_SubjectCode );
		return;
	}

	if ( ! XMP_PropIsArray ( xmpFlags ) ) return;	// ? Complain? Delete the DataSet?

	XMP_Index xmpCount  = xmp.CountArrayItems ( kXMP_NS_IPTCCore, "SubjectCode" );
	XMP_Index iptcCount = (XMP_Index) iptc->GetDataSet ( kIPTC_SubjectCode, 0 );

	if ( xmpCount != iptcCount ) iptc->DeleteDataSet ( kIPTC_SubjectCode );

	for ( XMP_Index ds = 0; ds < xmpCount; ++ds ) {	// ! XMP arrays are indexed from 1, IPTC from 0.

		(void) xmp.GetArrayItem ( kXMP_NS_IPTCCore, "SubjectCode", ds+1, &xmpValue, &xmpFlags );
		if ( ! XMP_PropIsSimple ( xmpFlags ) ) continue;	// ? Complain?
		if ( xmpValue.size() != 8 ) continue;	// ? Complain?

		iimValue = "IPTC:";
		iimValue += xmpValue;
		iimValue += ":::";	// Add the separating colons for the empty name portions.

		iptc->SetDataSet_UTF8 ( kIPTC_SubjectCode, iimValue.c_str(), (XMP_Uns32)iimValue.size(), ds );	// ! Appends if necessary.

	}

}	// ExportIPTC_SubjectCode

// =================================================================================================
// ExportIPTC_Date
// ===============
//
// The IPTC date and time are "YYYYMMDD" and "HHMMSSxHHMM" where 'x' is '+' or '-'. Export the IPTC
// time only if already present, or if the XMP has a time portion.

// *** The date/time handling differs from the MWG 1.0 policy, following a proposed tweak to MWG:
// ***   Exif DateTimeOriginal <-> IPTC DateCreated <-> XMP photoshop:DateCreated
// ***   Exif DateTimeDigitized <-> IPTC DigitalCreateDate <-> XMP xmp:CreateDate

static void ExportIPTC_Date ( XMP_Uns8 dateID, const SXMPMeta & xmp, IPTC_Manager * iptc )
{
	XMP_Uns8 timeID;
	XMP_StringPtr xmpNS, xmpProp;
	
	if ( dateID == kIPTC_DateCreated ) {
		timeID  = kIPTC_TimeCreated;
		xmpNS   = kXMP_NS_Photoshop;
		xmpProp = "DateCreated";
	} else if ( dateID == kIPTC_DigitalCreateDate ) {
		timeID  = kIPTC_DigitalCreateTime;
		xmpNS   = kXMP_NS_XMP;
		xmpProp = "CreateDate";
	} else {
		XMP_Throw ( "Unrecognized dateID", kXMPErr_BadParam );
	}

	iptc->DeleteDataSet ( dateID );	// ! Either the XMP does not exist and we want to 
	iptc->DeleteDataSet ( timeID );	// ! delete the IPTC, or we're replacing the IPTC.

	XMP_DateTime xmpValue;
	bool found = xmp.GetProperty_Date ( xmpNS, xmpProp, &xmpValue, 0 );
	if ( ! found ) return;

	char iimValue[16];	// AUDIT: Big enough for "YYYYMMDD" (8) and "HHMMSS+HHMM" (11).

	// Set the IIM date portion as YYYYMMDD with zeroes for unknown parts.
	
	snprintf ( iimValue, sizeof(iimValue), "%04d%02d%02d",	// AUDIT: Use of sizeof(iimValue) is safe.
			   xmpValue.year, xmpValue.month, xmpValue.day );

	iptc->SetDataSet_UTF8 ( dateID, iimValue, 8 );

	// Set the IIM time portion as HHMMSS+HHMM (or -HHMM). Allow a missing time zone.

	if ( xmpValue.hasTimeZone )  {
		snprintf ( iimValue, sizeof(iimValue), "%02d%02d%02d%c%02d%02d",	// AUDIT: Use of sizeof(iimValue) is safe.
				   xmpValue.hour, xmpValue.minute, xmpValue.second,
				   ((xmpValue.tzSign == kXMP_TimeWestOfUTC) ? '-' : '+'), xmpValue.tzHour, xmpValue.tzMinute );
		iptc->SetDataSet_UTF8 ( timeID, iimValue, 11 );
	} else if ( xmpValue.hasTime ) {
		snprintf ( iimValue, sizeof(iimValue), "%02d%02d%02d",	// AUDIT: Use of sizeof(iimValue) is safe.
				   xmpValue.hour, xmpValue.minute, xmpValue.second );
		iptc->SetDataSet_UTF8 ( timeID, iimValue, 6 );
	} else {
		iptc->DeleteDataSet ( timeID );
	}

}	// ExportIPTC_Date

// =================================================================================================
// PhotoDataUtils::ExportIPTC
// ==========================

void PhotoDataUtils::ExportIPTC ( const SXMPMeta & xmp, IPTC_Manager * iptc )
{

	for ( size_t i = 0; kKnownDataSets[i].dsNum != 255; ++i ) {

		try {	// Don't let errors with one stop the others.

			const DataSetCharacteristics & thisDS = kKnownDataSets[i];
			if ( thisDS.mapForm >= kIPTC_UnmappedText ) continue;

			switch ( thisDS.mapForm ) {

				case kIPTC_MapSimple :
					ExportIPTC_Simple ( xmp, iptc, thisDS.xmpNS, thisDS.xmpProp, thisDS.dsNum );
					break;

				case kIPTC_MapLangAlt :
					ExportIPTC_LangAlt ( xmp, iptc, thisDS.xmpNS, thisDS.xmpProp, thisDS.dsNum );
					break;

				case kIPTC_MapArray :
					ExportIPTC_Array ( xmp, iptc, thisDS.xmpNS, thisDS.xmpProp, thisDS.dsNum );
					break;

				case kIPTC_MapSpecial :
					if ( thisDS.dsNum == kIPTC_DateCreated ) {
						ExportIPTC_Date ( thisDS.dsNum, xmp, iptc );
					} else if ( thisDS.dsNum == kIPTC_IntellectualGenre ) {
						ExportIPTC_IntellectualGenre ( xmp, iptc );
					} else if ( thisDS.dsNum == kIPTC_SubjectCode ) {
						ExportIPTC_SubjectCode ( xmp, iptc );
					} else {
						XMP_Assert ( false );	// Catch mapping errors.
					}
					break;
				
				case kIPTC_Map3Way :	// The 3 way case is special for import, not for export.
					if ( thisDS.dsNum == kIPTC_DigitalCreateDate ) {
						// ! Special case: Don't create IIM DigitalCreateDate. This can avoid PSD
						// ! full rewrite due to new mapping from xmp:CreateDate.
						if ( iptc->GetDataSet ( thisDS.dsNum, 0 ) > 0 ) ExportIPTC_Date ( thisDS.dsNum, xmp, iptc );
					} else if ( thisDS.dsNum == kIPTC_Creator ) {
						ExportIPTC_Array ( xmp, iptc, kXMP_NS_DC, "creator", kIPTC_Creator );
					} else if ( thisDS.dsNum == kIPTC_CopyrightNotice ) {
						ExportIPTC_LangAlt ( xmp, iptc, kXMP_NS_DC, "rights", kIPTC_CopyrightNotice );
					} else if ( thisDS.dsNum == kIPTC_Description ) {
						ExportIPTC_LangAlt ( xmp, iptc, kXMP_NS_DC, "description", kIPTC_Description );
					} else {
						XMP_Assert ( false );	// Catch mapping errors.
					}

			}

		} catch ( ... ) {

			// Do nothing, let other exports proceed.
			// ? Notify client?

		}

	}

}	// PhotoDataUtils::ExportIPTC;

// =================================================================================================
// PhotoDataUtils::ExportPSIR
// ==========================
//
// There are only 2 standalone Photoshop image resources for XMP properties:
//    1034 - Copyright Flag - 0/1 Boolean mapped to xmpRights:Marked.
//    1035 - Copyright URL - Local OS text mapped to xmpRights:WebStatement.

// ! We don't bother with the CR<->LF normalization for xmpRights:WebStatement. Very little chance
// ! of having a raw CR character in a URI.

void PhotoDataUtils::ExportPSIR ( const SXMPMeta & xmp, PSIR_Manager * psir )
{
	bool found;
	std::string utf8Value;

	try {	// Don't let errors with one stop the others.
		found = xmp.GetProperty ( kXMP_NS_XMP_Rights, "Marked", &utf8Value, 0 );
		if ( ! found ) {
			psir->DeleteImgRsrc ( kPSIR_CopyrightFlag );
		} else {
			bool copyrighted = SXMPUtils::ConvertToBool ( utf8Value );
			psir->SetImgRsrc ( kPSIR_CopyrightFlag, &copyrighted, 1 );
		}
	} catch ( ... ) {
		// Do nothing, let other exports proceed.
		// ? Notify client?
	}

	try {	// Don't let errors with one stop the others.
		found = xmp.GetProperty ( kXMP_NS_XMP_Rights, "WebStatement", &utf8Value, 0 );
		if ( ! found ) {
			psir->DeleteImgRsrc ( kPSIR_CopyrightURL );
		} else if ( ! ignoreLocalText ) {
			std::string localValue;
			ReconcileUtils::UTF8ToLocal ( utf8Value.c_str(), utf8Value.size(), &localValue );
			psir->SetImgRsrc ( kPSIR_CopyrightURL, localValue.c_str(), (XMP_Uns32)localValue.size() );
		} else if ( ReconcileUtils::IsASCII ( utf8Value.c_str(), utf8Value.size() ) ) {
			psir->SetImgRsrc ( kPSIR_CopyrightURL, utf8Value.c_str(), (XMP_Uns32)utf8Value.size() );
		} else {
			psir->DeleteImgRsrc ( kPSIR_CopyrightURL );
		}
	} catch ( ... ) {
		// Do nothing, let other exports proceed.
		// ? Notify client?
	}

}	// PhotoDataUtils::ExportPSIR;
