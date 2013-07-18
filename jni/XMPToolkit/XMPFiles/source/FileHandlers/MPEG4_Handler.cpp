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
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "source/XMPFiles_IO.hpp"
#include "source/XIO.hpp"

#include "XMPFiles/source/FileHandlers/MPEG4_Handler.hpp"

#include "XMPFiles/source/FormatSupport/ISOBaseMedia_Support.hpp"
#include "XMPFiles/source/FormatSupport/MOOV_Support.hpp"

#include "source/XMP_ProgressTracker.hpp"
#include "source/UnicodeConversions.hpp"
#include "third-party/zuid/interfaces/MD5.h"

#if XMP_WinBuild
	#pragma warning ( disable : 4996 )	// '...' was declared deprecated
#endif

using namespace std;

// =================================================================================================
/// \file MPEG4_Handler.cpp
/// \brief File format handler for MPEG-4, a flavor of the ISO Base Media File Format.
///
/// This handler ...
///
// =================================================================================================

// The basic content of a timecode sample description table entry. Does not include trailing boxes.

#if SUNOS_SPARC || SUNOS_X86
#pragma pack ( 1 )
#else
#pragma pack ( push, 1 )
#endif //#if SUNOS_SPARC || SUNOS_X86

struct stsdBasicEntry {
	XMP_Uns32 entrySize;
	XMP_Uns32 format;
	XMP_Uns8  reserved_1 [6];
	XMP_Uns16 dataRefIndex;
	XMP_Uns32 reserved_2;
	XMP_Uns32 flags;
	XMP_Uns32 timeScale;
	XMP_Uns32 frameDuration;
	XMP_Uns8  frameCount;
	XMP_Uns8  reserved_3;
};

#if SUNOS_SPARC || SUNOS_X86
#pragma pack (  )
#else
#pragma pack ( pop )
#endif //#if SUNOS_SPARC || SUNOS_X86

// =================================================================================================

// ! The buffer and constants are both already big endian.
#define Get4CharCode(buffPtr) (*((XMP_Uns32*)(buffPtr)))

// =================================================================================================

static inline bool IsClassicQuickTimeBox ( XMP_Uns32 boxType )
{
	if ( (boxType == ISOMedia::k_moov) || (boxType == ISOMedia::k_mdat) || (boxType == ISOMedia::k_pnot) ||
		 (boxType == ISOMedia::k_free) || (boxType == ISOMedia::k_skip) || (boxType == ISOMedia::k_wide) ) return true;
	return false;
}	// IsClassicQuickTimeBox

// =================================================================================================

// Pairs of 3 letter ISO 639-2 codes mapped to 2 letter ISO 639-1 codes from:
//   http://www.loc.gov/standards/iso639-2/php/code_list.php
// Would have to write an "==" operator to use std::map, must compare values not pointers.
// ! Not fully sorted, do not use a binary search.

static XMP_StringPtr kKnownLangs[] =
	{ "aar", "aa", "abk", "ab", "afr", "af", "aka", "ak", "alb", "sq", "sqi", "sq", "amh", "am",
	  "ara", "ar", "arg", "an", "arm", "hy", "hye", "hy", "asm", "as", "ava", "av", "ave", "ae",
	  "aym", "ay", "aze", "az", "bak", "ba", "bam", "bm", "baq", "eu", "eus", "eu", "bel", "be",
	  "ben", "bn", "bih", "bh", "bis", "bi", "bod", "bo", "tib", "bo", "bos", "bs", "bre", "br",
	  "bul", "bg", "bur", "my", "mya", "my", "cat", "ca", "ces", "cs", "cze", "cs", "cha", "ch",
	  "che", "ce", "chi", "zh", "zho", "zh", "chu", "cu", "chv", "cv", "cor", "kw", "cos", "co",
	  "cre", "cr", "cym", "cy", "wel", "cy", "cze", "cs", "ces", "cs", "dan", "da", "deu", "de",
	  "ger", "de", "div", "dv", "dut", "nl", "nld", "nl", "dzo", "dz", "ell", "el", "gre", "el",
	  "eng", "en", "epo", "eo", "est", "et", "eus", "eu", "baq", "eu", "ewe", "ee", "fao", "fo",
	  "fas", "fa", "per", "fa", "fij", "fj", "fin", "fi", "fra", "fr", "fre", "fr", "fre", "fr",
	  "fra", "fr", "fry", "fy", "ful", "ff", "geo", "ka", "kat", "ka", "ger", "de", "deu", "de",
	  "gla", "gd", "gle", "ga", "glg", "gl", "glv", "gv", "gre", "el", "ell", "el", "grn", "gn",
	  "guj", "gu", "hat", "ht", "hau", "ha", "heb", "he", "her", "hz", "hin", "hi", "hmo", "ho",
	  "hrv", "hr", "scr", "hr", "hun", "hu", "hye", "hy", "arm", "hy", "ibo", "ig", "ice", "is",
	  "isl", "is", "ido", "io", "iii", "ii", "iku", "iu", "ile", "ie", "ina", "ia", "ind", "id",
	  "ipk", "ik", "isl", "is", "ice", "is", "ita", "it", "jav", "jv", "jpn", "ja", "kal", "kl",
	  "kan", "kn", "kas", "ks", "kat", "ka", "geo", "ka", "kau", "kr", "kaz", "kk", "khm", "km",
	  "kik", "ki", "kin", "rw", "kir", "ky", "kom", "kv", "kon", "kg", "kor", "ko", "kua", "kj",
	  "kur", "ku", "lao", "lo", "lat", "la", "lav", "lv", "lim", "li", "lin", "ln", "lit", "lt",
	  "ltz", "lb", "lub", "lu", "lug", "lg", "mac", "mk", "mkd", "mk", "mah", "mh", "mal", "ml",
	  "mao", "mi", "mri", "mi", "mar", "mr", "may", "ms", "msa", "ms", "mkd", "mk", "mac", "mk",
	  "mlg", "mg", "mlt", "mt", "mol", "mo", "mon", "mn", "mri", "mi", "mao", "mi", "msa", "ms",
	  "may", "ms", "mya", "my", "bur", "my", "nau", "na", "nav", "nv", "nbl", "nr", "nde", "nd",
	  "ndo", "ng", "nep", "ne", "nld", "nl", "dut", "nl", "nno", "nn", "nob", "nb", "nor", "no",
	  "nya", "ny", "oci", "oc", "oji", "oj", "ori", "or", "orm", "om", "oss", "os", "pan", "pa",
	  "per", "fa", "fas", "fa", "pli", "pi", "pol", "pl", "por", "pt", "pus", "ps", "que", "qu",
	  "roh", "rm", "ron", "ro", "rum", "ro", "rum", "ro", "ron", "ro", "run", "rn", "rus", "ru",
	  "sag", "sg", "san", "sa", "scc", "sr", "srp", "sr", "scr", "hr", "hrv", "hr", "sin", "si",
	  "slk", "sk", "slo", "sk", "slo", "sk", "slk", "sk", "slv", "sl", "sme", "se", "smo", "sm",
	  "sna", "sn", "snd", "sd", "som", "so", "sot", "st", "spa", "es", "sqi", "sq", "alb", "sq",
	  "srd", "sc", "srp", "sr", "scc", "sr", "ssw", "ss", "sun", "su", "swa", "sw", "swe", "sv",
	  "tah", "ty", "tam", "ta", "tat", "tt", "tel", "te", "tgk", "tg", "tgl", "tl", "tha", "th",
	  "tib", "bo", "bod", "bo", "tir", "ti", "ton", "to", "tsn", "tn", "tso", "ts", "tuk", "tk",
	  "tur", "tr", "twi", "tw", "uig", "ug", "ukr", "uk", "urd", "ur", "uzb", "uz", "ven", "ve",
	  "vie", "vi", "vol", "vo", "wel", "cy", "cym", "cy", "wln", "wa", "wol", "wo", "xho", "xh",
	  "yid", "yi", "yor", "yo", "zha", "za", "zho", "zh", "chi", "zh", "zul", "zu",
	  0, 0 };

static inline XMP_StringPtr Lookup2LetterLang ( XMP_StringPtr lang3 )
{
	for ( size_t i = 0; kKnownLangs[i] != 0; i += 2 ) {
		if ( XMP_LitMatch ( lang3, kKnownLangs[i] ) ) return kKnownLangs[i+1];
	}
	return "";
}

static inline XMP_StringPtr Lookup3LetterLang ( XMP_StringPtr lang2 )
{
	for ( size_t i = 0; kKnownLangs[i] != 0; i += 2 ) {
		if ( XMP_LitMatch ( lang2, kKnownLangs[i+1] ) ) return kKnownLangs[i];
	}
	return "";
}

// =================================================================================================
// MPEG4_CheckFormat
// =================
//
// There are 3 variations of recognized file:
//	- Normal MPEG-4 - has an 'ftyp' box containing a known compatible brand but not 'qt  '.
//	- Modern QuickTime - has an 'ftyp' box containing 'qt  ' as a compatible brand.
//	- Classic QuickTime - has no 'ftyp' box, should have recognized top level boxes.
//
// An MPEG-4 or modern QuickTime file is an instance of an ISO Base Media file, ISO 14496-12 and -14.
// A classic QuickTime file has the same physical box structure, but somewhat different box types.
// The ISO files must begin with an 'ftyp' box containing 'mp41', 'mp42', 'f4v ', or 'qt  ' in the
// compatible brands.
//
// The general box structure is:
//
//   0  4  uns32  box size, 0 means "to EoF", 1 means 64-bit size follows
//   4  4  uns32  box type
//   8  8  uns64  box size, present only if uns32 size is 1
//   -  *  box content
//
// The 'ftyp' box content is:
//
//   -  4  uns32  major brand
//   -  4  uns32  minor version
//   -  *  uns32  sequence of compatible brands, to the end of the box

// ! With the addition of QuickTime support there is some change in behavior in OpenFile when the
// ! kXMPFiles_OpenStrictly option is used wth a specific file format. The kXMP_MPEG4File and
// ! kXMP_MOVFile formats are treated uniformly, you can't force "real MOV" or "real MPEG-4". You
// ! can check afterwards using GetFileInfo to see what the file happens to be.

bool MPEG4_CheckFormat ( XMP_FileFormat format,
						 XMP_StringPtr  filePath,
						 XMP_IO*    fileRef,
						 XMPFiles*      parent )
{
	XMP_Uns8  buffer [4*1024];
	XMP_Uns32 ioCount, brandCount, brandOffset;
	XMP_Uns64 fileSize, nextOffset;
	ISOMedia::BoxInfo currBox;

	#define IsTolerableBoxChar(ch)	( ((0x20 <= (ch)) && ((ch) <= 0x7E)) || ((ch) == 0xA9) )

	XMP_AbortProc abortProc  = parent->abortProc;
	void *        abortArg   = parent->abortArg;
	const bool    checkAbort = (abortProc != 0);

	bool openStrictly = XMP_OptionIsSet ( parent->openFlags, kXMPFiles_OpenStrictly);

	// Get the first box's info, see if it is 'ftyp' or not.

	XMP_Assert ( (parent->tempPtr == 0) && (parent->tempUI32 == 0) );

	fileSize = fileRef->Length();
	if ( fileSize < 8 ) return false;

	nextOffset = ISOMedia::GetBoxInfo ( fileRef, 0, fileSize, &currBox );
	if ( currBox.headerSize < 8 ) return false;	// Can't be an ISO or QuickTime file.

	if ( currBox.boxType == ISOMedia::k_ftyp ) {

		// Have an 'ftyp' box, look through the compatible brands. If 'qt  ' is present then this is
		// a modern QuickTime file, regardless of what else is found. Otherwise this is plain ISO if
		// any of the other recognized brands are found.

		if ( currBox.contentSize < 12 ) return false;	// No compatible brands at all.
		if ( currBox.contentSize > 1024*1024 ) return false;	// Sanity check and make sure count fits in 32 bits.
		brandCount = ((XMP_Uns32)currBox.contentSize - 8) >> 2;

		fileRef->Seek ( 8, kXMP_SeekFromCurrent );	// Skip the major and minor brands.
		ioCount = brandOffset = 0;

		bool haveCompatibleBrand = false;

		for ( ; brandCount > 0; --brandCount, brandOffset += 4 ) {

			if ( brandOffset >= ioCount ) {
				if ( checkAbort && abortProc(abortArg) ) {
					XMP_Throw ( "MPEG4_CheckFormat - User abort", kXMPErr_UserAbort );
				}
				ioCount = 4 * brandCount;
				if ( ioCount > sizeof(buffer) ) ioCount = sizeof(buffer);
				ioCount = fileRef->ReadAll ( buffer, ioCount );
				brandOffset = 0;
			}

			XMP_Uns32 brand = GetUns32BE ( &buffer[brandOffset] );
			if ( brand == ISOMedia::k_qt ) {	// Don't need to look further.
				if ( openStrictly && (format != kXMP_MOVFile) ) return false;
				parent->format = kXMP_MOVFile;
				parent->tempUI32 = MOOV_Manager::kFileIsModernQT;
				return true;
			} else if ( (brand == ISOMedia::k_mp41) || (brand == ISOMedia::k_mp42) || 
				(brand == ISOMedia::k_f4v) || ( brand == ISOMedia::k_avc1 ) ) {
				haveCompatibleBrand = true;	// Need to keep looking in case 'qt  ' follows.
			}

		}

		if ( ! haveCompatibleBrand ) return false;
		if ( openStrictly && (format != kXMP_MPEG4File) ) return false;
		parent->format = kXMP_MPEG4File;
		parent->tempUI32 = MOOV_Manager::kFileIsNormalISO;
		return true;

	} else {
		// No 'ftyp', look for classic QuickTime: 'moov', 'mdat', 'pnot', 'free', 'skip', and 'wide'.
		// As an expedient, quit when a 'moov' box is found. Tolerate other boxes, they are in the
		// wild for ill-formed files, e.g. seen when 'moov'/'udta' children get left at top level.

		while ( currBox.boxType != ISOMedia::k_moov ) {

			if ( ! IsClassicQuickTimeBox ( currBox.boxType ) ) {
				// Make sure the box type is 4 ASCII characters or 0xA9 (MacRoman copyright).
				XMP_Uns8 b1 = (XMP_Uns8) (currBox.boxType >> 24);
				XMP_Uns8 b2 = (XMP_Uns8) ((currBox.boxType >> 16) & 0xFF);
				XMP_Uns8 b3 = (XMP_Uns8) ((currBox.boxType >> 8) & 0xFF);
				XMP_Uns8 b4 = (XMP_Uns8) (currBox.boxType & 0xFF);
				bool ok = IsTolerableBoxChar(b1) && IsTolerableBoxChar(b2) &&
						  IsTolerableBoxChar(b3) && IsTolerableBoxChar(b4);
				if ( ! ok ) return false;
			}
			if ( nextOffset >= fileSize ) return false;
			if ( checkAbort && abortProc(abortArg) ) {
				XMP_Throw ( "MPEG4_CheckFormat - User abort", kXMPErr_UserAbort );
			}
			nextOffset = ISOMedia::GetBoxInfo ( fileRef, nextOffset, fileSize, &currBox );

		}

		if ( openStrictly && (format != kXMP_MOVFile) ) return false;
		parent->format = kXMP_MOVFile;
		parent->tempUI32 = MOOV_Manager::kFileIsTraditionalQT;
		return true;

	}

	return false;

}	// MPEG4_CheckFormat

// =================================================================================================
// MPEG4_MetaHandlerCTor
// =====================

XMPFileHandler * MPEG4_MetaHandlerCTor ( XMPFiles * parent )
{

	return new MPEG4_MetaHandler ( parent );

}	// MPEG4_MetaHandlerCTor

// =================================================================================================
// MPEG4_MetaHandler::MPEG4_MetaHandler
// ====================================

MPEG4_MetaHandler::MPEG4_MetaHandler ( XMPFiles * _parent )
	: fileMode(0), havePreferredXMP(false), xmpBoxPos(0), moovBoxPos(0), xmpBoxSize(0), moovBoxSize(0)
{

	this->parent = _parent;	// Inherited, can't set in the prefix.
	this->handlerFlags = kMPEG4_HandlerFlags;
	this->stdCharForm  = kXMP_Char8Bit;
	this->fileMode = (XMP_Uns8)_parent->tempUI32;
	_parent->tempUI32 = 0;

}	// MPEG4_MetaHandler::MPEG4_MetaHandler

// =================================================================================================
// MPEG4_MetaHandler::~MPEG4_MetaHandler
// =====================================

MPEG4_MetaHandler::~MPEG4_MetaHandler()
{

	// Nothing to do.

}	// MPEG4_MetaHandler::~MPEG4_MetaHandler

// =================================================================================================
// SecondsToXMPDate
// ================

// *** ASF has similar code with different origin, should make a shared utility.

static void SecondsToXMPDate ( XMP_Uns64 isoSeconds, XMP_DateTime * xmpDate )
{
	memset ( xmpDate, 0, sizeof(XMP_DateTime) );	// AUDIT: Using sizeof(XMP_DateTime) is safe.

	XMP_Int32 days = (XMP_Int32) (isoSeconds / 86400);
	isoSeconds -= ((XMP_Uns64)days * 86400);

	XMP_Int32 hour = (XMP_Int32) (isoSeconds / 3600);
	isoSeconds -= ((XMP_Uns64)hour * 3600);

	XMP_Int32 minute = (XMP_Int32) (isoSeconds / 60);
	isoSeconds -= ((XMP_Uns64)minute * 60);

	XMP_Int32 second = (XMP_Int32)isoSeconds;

	xmpDate->year  = 1904;	// Start with the ISO origin.
	xmpDate->month = 1;
	xmpDate->day   = 1;

	xmpDate->day += days;	// Add in the delta.
	xmpDate->hour = hour;
	xmpDate->minute = minute;
	xmpDate->second = second;

	xmpDate->hasTimeZone = true;	// ! Needed for ConvertToUTCTime to do anything.
	SXMPUtils::ConvertToUTCTime ( xmpDate );	// Normalize the date/time.

}	// SecondsToXMPDate

// =================================================================================================
// XMPDateToSeconds
// ================

// *** ASF has similar code with different origin, should make a shared utility.

static bool IsLeapYear ( XMP_Int32 year )
{
	if ( year < 0 ) year = -year + 1;		// Fold the negative years, assuming there is a year 0.
	if ( (year % 4) != 0 ) return false;	// Not a multiple of 4.
	if ( (year % 100) != 0 ) return true;	// A multiple of 4 but not a multiple of 100.
	if ( (year % 400) == 0 ) return true;	// A multiple of 400.
	return false;							// A multiple of 100 but not a multiple of 400.
}

// -------------------------------------------------------------------------------------------------

static XMP_Int32 DaysInMonth ( XMP_Int32 year, XMP_Int32 month )
{
	static XMP_Int32 daysInMonth[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
										 // Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec
	XMP_Int32 days = daysInMonth[month];
	if ( (month == 2) && IsLeapYear(year) ) days += 1;
	return days;
}

// -------------------------------------------------------------------------------------------------

static void XMPDateToSeconds ( const XMP_DateTime & _xmpDate, XMP_Uns64 * isoSeconds )
{
	XMP_DateTime xmpDate = _xmpDate;
	SXMPUtils::ConvertToUTCTime ( &xmpDate );

	XMP_Uns64 tempSeconds = (XMP_Uns64)xmpDate.second;
	tempSeconds += (XMP_Uns64)xmpDate.minute * 60;
	tempSeconds += (XMP_Uns64)xmpDate.hour * 3600;

	XMP_Int32 days = (xmpDate.day - 1);

	--xmpDate.month;
	while ( xmpDate.month >= 1 ) {
		days += DaysInMonth ( xmpDate.year, xmpDate.month );
		--xmpDate.month;
	}

	--xmpDate.year;
	while ( xmpDate.year >= 1904 ) {
		days += (IsLeapYear ( xmpDate.year) ? 366 : 365 );
		--xmpDate.year;
	}

	tempSeconds += (XMP_Uns64)days * 86400;
	*isoSeconds = tempSeconds;

}	// XMPDateToSeconds

// =================================================================================================
// ImportMVHDItems
// ===============

static bool ImportMVHDItems ( MOOV_Manager::BoxInfo mvhdInfo, SXMPMeta * xmp )
{
	XMP_Assert ( mvhdInfo.boxType == ISOMedia::k_mvhd );
	if ( mvhdInfo.contentSize < 4 ) return false;	// Just enough to check the version/flags at first.

	XMP_Uns8 mvhdVersion = *mvhdInfo.content;
	if ( mvhdVersion > 1 ) return false;

	XMP_Uns64 creationTime, modificationTime, duration;
	XMP_Uns32 timescale;

	if ( mvhdVersion == 0 ) {

		if ( mvhdInfo.contentSize < sizeof ( MOOV_Manager::Content_mvhd_0 ) ) return false;
		MOOV_Manager::Content_mvhd_0 * mvhdRaw_0 = (MOOV_Manager::Content_mvhd_0*) mvhdInfo.content;

		creationTime = (XMP_Uns64) GetUns32BE ( &mvhdRaw_0->creationTime );
		modificationTime = (XMP_Uns64) GetUns32BE ( &mvhdRaw_0->modificationTime );
		timescale = GetUns32BE ( &mvhdRaw_0->timescale );
		duration = (XMP_Uns64) GetUns32BE ( &mvhdRaw_0->duration );

	} else {

		XMP_Assert ( mvhdVersion == 1 );
		if ( mvhdInfo.contentSize < sizeof ( MOOV_Manager::Content_mvhd_1 ) ) return false;
		MOOV_Manager::Content_mvhd_1 * mvhdRaw_1 = (MOOV_Manager::Content_mvhd_1*) mvhdInfo.content;

		creationTime = GetUns64BE ( &mvhdRaw_1->creationTime );
		modificationTime = GetUns64BE ( &mvhdRaw_1->modificationTime );
		timescale = GetUns32BE ( &mvhdRaw_1->timescale );
		duration = GetUns64BE ( &mvhdRaw_1->duration );

	}

	bool haveImports = false;
	XMP_DateTime xmpDate;

	if ( (creationTime >> 32) < 0xFF ) {		// Sanity check for bogus date info.
		SecondsToXMPDate ( creationTime, &xmpDate );
		xmp->SetProperty_Date ( kXMP_NS_XMP, "CreateDate", xmpDate );
		haveImports = true;
	}

	if ( (modificationTime >> 32) < 0xFF ) {	// Sanity check for bogus date info.
		SecondsToXMPDate ( modificationTime, &xmpDate );
		xmp->SetProperty_Date ( kXMP_NS_XMP, "ModifyDate", xmpDate );
		haveImports = true;
	}

	if ( timescale != 0 ) {	// Avoid 1/0 for the scale field.
		char buffer [32];	// A 64-bit number is at most 20 digits.
		xmp->DeleteProperty ( kXMP_NS_DM, "duration" );	// Delete the whole struct.
		snprintf ( buffer, sizeof(buffer), "%llu", duration );	// AUDIT: The buffer is big enough.
		xmp->SetStructField ( kXMP_NS_DM, "duration", kXMP_NS_DM, "value", &buffer[0] );
		snprintf ( buffer, sizeof(buffer), "1/%u", timescale );	// AUDIT: The buffer is big enough.
		xmp->SetStructField ( kXMP_NS_DM, "duration", kXMP_NS_DM, "scale", &buffer[0] );
		haveImports = true;
	}

	return haveImports;

}	// ImportMVHDItems

// =================================================================================================
// ExportMVHDItems
// ===============

static void ExportMVHDItems ( const SXMPMeta & xmp, MOOV_Manager * moovMgr )
{
	XMP_DateTime xmpDate;
	bool createFound, modifyFound;
	XMP_Uns64 createSeconds = 0, modifySeconds = 0;

	MOOV_Manager::BoxInfo mvhdInfo;
	MOOV_Manager::BoxRef  mvhdRef = moovMgr->GetBox ( "moov/mvhd", &mvhdInfo );
	if ( (mvhdRef == 0) || (mvhdInfo.contentSize < 4) ) return;

	XMP_Uns8 version = *mvhdInfo.content;
	if ( version > 1 ) return;

	createFound = xmp.GetProperty_Date ( kXMP_NS_XMP, "CreateDate", &xmpDate, 0 );
	if ( createFound ) XMPDateToSeconds ( xmpDate, &createSeconds );

	modifyFound = xmp.GetProperty_Date ( kXMP_NS_XMP, "ModifyDate", &xmpDate, 0 );
	if ( modifyFound ) XMPDateToSeconds ( xmpDate, &modifySeconds );

	if ( version == 1 ) {

		// Modify the v1 box in-place.

		if ( mvhdInfo.contentSize < sizeof ( MOOV_Manager::Content_mvhd_1 ) ) return;

		XMP_Uns64 oldCreate = GetUns64BE ( mvhdInfo.content + 4 );
		XMP_Uns64 oldModify = GetUns64BE ( mvhdInfo.content + 12 );

		if ( createFound ) {
			if ( createSeconds != oldCreate ) PutUns64BE ( createSeconds, ((XMP_Uns8*)mvhdInfo.content + 4) );
			moovMgr->NoteChange();
		}
		if ( modifyFound  ) {
			if ( modifySeconds != oldModify ) PutUns64BE ( modifySeconds, ((XMP_Uns8*)mvhdInfo.content + 12) );
			moovMgr->NoteChange();
		}

	} else if ( ((createSeconds >> 32) == 0) && ((modifySeconds >> 32) == 0) ) {

		// Modify the v0 box in-place.

		if ( mvhdInfo.contentSize < sizeof ( MOOV_Manager::Content_mvhd_0 ) ) return;

		XMP_Uns32 oldCreate = GetUns32BE ( mvhdInfo.content + 4 );
		XMP_Uns32 oldModify = GetUns32BE ( mvhdInfo.content + 8 );

		if ( createFound ) {
			if ( (XMP_Uns32)createSeconds != oldCreate ) PutUns32BE ( (XMP_Uns32)createSeconds, ((XMP_Uns8*)mvhdInfo.content + 4) );
			moovMgr->NoteChange();
		}
		if ( modifyFound ) {
			if ( (XMP_Uns32)modifySeconds != oldModify ) PutUns32BE ( (XMP_Uns32)modifySeconds, ((XMP_Uns8*)mvhdInfo.content + 8) );
			moovMgr->NoteChange();
		}

	} else {

		// Replace the v0 box with a v1 box.

		XMP_Assert ( createFound | modifyFound );	// One of them has high bits set.
		if ( mvhdInfo.contentSize != sizeof ( MOOV_Manager::Content_mvhd_0 ) ) return;

		MOOV_Manager::Content_mvhd_0 * mvhdV0 = (MOOV_Manager::Content_mvhd_0*) mvhdInfo.content;
		MOOV_Manager::Content_mvhd_1   mvhdV1;

		// Copy the unchanged fields directly.

		mvhdV1.timescale = mvhdV0->timescale;
		mvhdV1.rate = mvhdV0->rate;
		mvhdV1.volume = mvhdV0->volume;
		mvhdV1.pad_1 = mvhdV0->pad_1;
		mvhdV1.pad_2 = mvhdV0->pad_2;
		mvhdV1.pad_3 = mvhdV0->pad_3;
		for ( int i = 0; i < 9; ++i ) mvhdV1.matrix[i] = mvhdV0->matrix[i];
		for ( int i = 0; i < 6; ++i ) mvhdV1.preDef[i] = mvhdV0->preDef[i];
		mvhdV1.nextTrackID = mvhdV0->nextTrackID;

		// Set the fields that have changes.

		mvhdV1.vFlags = (1 << 24) | (mvhdV0->vFlags & 0xFFFFFF);
		mvhdV1.duration = MakeUns64BE ( (XMP_Uns64) GetUns32BE ( &mvhdV0->duration ) );

		XMP_Uns64 temp64;

		temp64 = (XMP_Uns64) GetUns32BE ( &mvhdV0->creationTime );
		if ( createFound ) temp64 = createSeconds;
		mvhdV1.creationTime = MakeUns64BE ( temp64 );

		temp64 = (XMP_Uns64) GetUns32BE ( &mvhdV0->modificationTime );
		if ( modifyFound ) temp64 = modifySeconds;
		mvhdV1.modificationTime = MakeUns64BE ( temp64 );

		moovMgr->SetBox ( mvhdRef, &mvhdV1, sizeof ( MOOV_Manager::Content_mvhd_1 ) );

	}

}	// ExportMVHDItems

// =================================================================================================
// ImportISOCopyrights
// ===================
//
// The cached 'moov'/'udta'/'cprt' boxes are full boxes. The "real" content is a UInt16 packed 3
// character language code and a UTF-8 or UTF-16 string.

static bool ImportISOCopyrights ( const std::vector<MOOV_Manager::BoxInfo> & cprtBoxes, SXMPMeta * xmp )
{
	bool haveImports = false;

	std::string tempStr;
	char lang3 [4];	// The unpacked ISO-639-2/T language code with final null.
	lang3[3] = 0;

	for ( size_t i = 0, limit = cprtBoxes.size(); i < limit; ++i ) {

		const MOOV_Manager::BoxInfo & currBox = cprtBoxes[i];
		if ( currBox.contentSize < 4+2+1 ) continue;	// Want enough for a non-empty value.
		if ( *currBox.content != 0 ) continue;	// Only proceed for version 0, ignore the flags.

		XMP_Uns16 packedLang = GetUns16BE ( currBox.content + 4 );
		lang3[0] = (packedLang >> 10) + 0x60;
		lang3[1] = ((packedLang >> 5) & 0x1F) + 0x60;
		lang3[2] = (packedLang & 0x1F) + 0x60;
		XMP_StringPtr xmpLang  = Lookup2LetterLang ( lang3 );
		if ( *xmpLang == 0 ) continue;

		XMP_StringPtr textPtr = (XMP_StringPtr) (currBox.content + 6);
		XMP_StringLen textLen = currBox.contentSize - 6;

		if ( (textLen >= 2) && (GetUns16BE(textPtr) == 0xFEFF) ) {
			FromUTF16 ( (UTF16Unit*)textPtr, textLen/2, &tempStr, true /* big endian */ );
			textPtr = tempStr.c_str();
		}

		xmp->SetLocalizedText ( kXMP_NS_DC, "rights", xmpLang, xmpLang, textPtr );
		haveImports = true;

	}

	return haveImports;

}	// ImportISOCopyrights

// =================================================================================================
// ExportISOCopyrights
// ===================

static void ExportISOCopyrights ( const SXMPMeta & xmp, MOOV_Manager * moovMgr )
{
	bool haveMappings = false;	// True if any ISO-XMP language mappings are found.

	// Go through the ISO 'cprt' items and look for a corresponding XMP item. Ignore the ISO item if
	// there is no language mapping to XMP. Update the ISO item if the mappable XMP exists, delete
	// the ISO item if the mappable XMP does not exist. Since the import side would have made sure
	// the mappable XMP items existed, if they don't now they must have been deleted.

	MOOV_Manager::BoxInfo udtaInfo;
	MOOV_Manager::BoxRef  udtaRef = moovMgr->GetBox ( "moov/udta", &udtaInfo );
	if ( udtaRef == 0 ) return;

	std::string xmpPath, xmpValue, xmpLang, tempStr;
	char lang3 [4];	// An unpacked ISO-639-2/T language code.
	lang3[3] = 0;

	for ( XMP_Uns32 ordinal = udtaInfo.childCount; ordinal > 0; --ordinal ) {	// ! Go backwards because of deletions.

		MOOV_Manager::BoxInfo cprtInfo;
		MOOV_Manager::BoxRef  cprtRef = moovMgr->GetNthChild ( udtaRef, ordinal-1, &cprtInfo );
		if ( cprtRef == 0 ) break;	// Sanity check, should not happen.
		if ( (cprtInfo.boxType != ISOMedia::k_cprt) || (cprtInfo.contentSize < 6) ) continue;
		if ( *cprtInfo.content != 0 ) continue;	// Only accept version 0, ignore the flags.

		XMP_Uns16 packedLang = GetUns16BE ( cprtInfo.content + 4 );
		lang3[0] = (packedLang >> 10) + 0x60;
		lang3[1] = ((packedLang >> 5) & 0x1F) + 0x60;
		lang3[2] = (packedLang & 0x1F) + 0x60;

		XMP_StringPtr lang2  = Lookup2LetterLang ( lang3 );
		if ( *lang2 == 0 ) continue;	// No language mapping to XMP.
		haveMappings = true;

		bool xmpFound = xmp.GetLocalizedText ( kXMP_NS_DC, "rights", lang2, lang2, &xmpLang, &xmpValue, 0 );
		if ( xmpFound ) {
			if ( (xmpLang.size() < 2) ||
				 ( (xmpLang.size() == 2) && (xmpLang != lang2) ) ||
				 ( (xmpLang.size() > 2) && ( (xmpLang[2] != '-') || (! XMP_LitNMatch ( xmpLang.c_str(), lang2, 2)) ) ) ) {
				xmpFound = false;	// The language does not match, the corresponding XMP does not exist.
			}
		}

		if ( ! xmpFound ) {

			// No XMP, delete the ISO item.
			moovMgr->DeleteNthChild ( udtaRef, ordinal-1 );

		} else {

			// Update the ISO item if necessary.
			XMP_StringPtr isoStr = (XMP_StringPtr)cprtInfo.content + 6;
			size_t rawLen = cprtInfo.contentSize - 6;
			if ( (rawLen >= 8) && (GetUns16BE(isoStr) == 0xFEFF) ) {
				FromUTF16 ( (UTF16Unit*)(isoStr+2), (rawLen-2)/2, &tempStr, true /* big endian */ );
				isoStr = tempStr.c_str();
			}
			if ( xmpValue != isoStr ) {
				std::string newContent = "vfffll";
				newContent += xmpValue;
				memcpy ( (char*)newContent.c_str(), cprtInfo.content, 6 ); // Keep old version, flags, and language.
				moovMgr->SetBox ( cprtRef, newContent.c_str(), (XMP_Uns32)(newContent.size() + 1) );
			}

		}

	}

	// Go through the XMP items and look for a corresponding ISO item. Skip if found (did it above),
	// otherwise add a new ISO item.

	bool haveXDefault = false;
	XMP_Index xmpCount = xmp.CountArrayItems ( kXMP_NS_DC, "rights" );

	for ( XMP_Index xmpIndex = 1; xmpIndex <= xmpCount; ++xmpIndex ) {	// ! The first XMP array index is 1.

		SXMPUtils::ComposeArrayItemPath ( kXMP_NS_DC, "rights", xmpIndex, &xmpPath );
		xmp.GetArrayItem ( kXMP_NS_DC, "rights", xmpIndex, &xmpValue, 0 );
		bool hasLang = xmp.GetQualifier ( kXMP_NS_DC, xmpPath.c_str(), kXMP_NS_XML, "lang", &xmpLang, 0 );
		if ( ! hasLang ) continue;	// Sanity check.
		if ( xmpLang == "x-default" ) {
			haveXDefault = true;	// See later special case.
			continue;
		}

		XMP_StringPtr isoLang = "";
		size_t rootLen = xmpLang.find ( '-' );
		if ( rootLen == std::string::npos ) rootLen = xmpLang.size();
		if ( rootLen == 2 ) {
			if( xmpLang.size() > 2 ) xmpLang[2] = 0;
			isoLang = Lookup3LetterLang ( xmpLang.c_str() );
			if ( *isoLang == 0 ) continue;
		} else if ( rootLen == 3 ) {
			if( xmpLang.size() > 3 ) xmpLang[3] = 0;
			isoLang = xmpLang.c_str();
		} else {
			continue;
		}
		haveMappings = true;

		bool isoFound = false;
		XMP_Uns16 packedLang = ((isoLang[0] - 0x60) << 10) | ((isoLang[1] - 0x60) << 5) | (isoLang[2] - 0x60);

		for ( XMP_Uns32 isoIndex = 0; (isoIndex < udtaInfo.childCount) && (! isoFound); ++isoIndex ) {

			MOOV_Manager::BoxInfo cprtInfo;
			MOOV_Manager::BoxRef  cprtRef = moovMgr->GetNthChild ( udtaRef, isoIndex, &cprtInfo );
			if ( cprtRef == 0 ) break;	// Sanity check, should not happen.
			if ( (cprtInfo.boxType != ISOMedia::k_cprt) || (cprtInfo.contentSize < 6) ) continue;
			if ( *cprtInfo.content != 0 ) continue;	// Only accept version 0, ignore the flags.
			if ( packedLang != GetUns16BE ( cprtInfo.content + 4 ) ) continue;	// Look for matching language.

			isoFound = true;	// Found the language entry, whether or not we update it.

		}

		if ( ! isoFound ) {

			std::string newContent = "vfffll";
			newContent += xmpValue;
			*((XMP_Uns32*)newContent.c_str()) = 0;	// Set the version and flags to zero.
			PutUns16BE ( packedLang, (char*)newContent.c_str() + 4 );
			moovMgr->AddChildBox ( udtaRef, ISOMedia::k_cprt, newContent.c_str(), (XMP_Uns32)(newContent.size() + 1) );

		}

	}

	// If there were no mappings in the loops, export the XMP "x-default" value to the first ISO item.

	if ( ! haveMappings ) {

		MOOV_Manager::BoxInfo cprtInfo;
		MOOV_Manager::BoxRef  cprtRef = moovMgr->GetTypeChild ( udtaRef, ISOMedia::k_cprt, &cprtInfo );

		if ( (cprtRef != 0) && (cprtInfo.contentSize >= 6) && (*cprtInfo.content == 0) ) {

			bool xmpFound = xmp.GetLocalizedText ( kXMP_NS_DC, "rights", "", "x-default", &xmpLang, &xmpValue, 0 );

			if ( xmpFound && (xmpLang == "x-default") ) {

				// Update the ISO item if necessary.
				XMP_StringPtr isoStr = (XMP_StringPtr)cprtInfo.content + 6;
				size_t rawLen = cprtInfo.contentSize - 6;
				if ( (rawLen >= 8) && (GetUns16BE(isoStr) == 0xFEFF) ) {
					FromUTF16 ( (UTF16Unit*)(isoStr+2), (rawLen-2)/2, &tempStr, true /* big endian */ );
					isoStr = tempStr.c_str();
				}
				if ( xmpValue != isoStr ) {
					std::string newContent = "vfffll";
					newContent += xmpValue;
					memcpy ( (char*)newContent.c_str(), cprtInfo.content, 6 ); // Keep old version, flags, and language.
					moovMgr->SetBox ( cprtRef, newContent.c_str(), (XMP_Uns32)(newContent.size() + 1) );
				}

			}

		}

	}

}	// ExportISOCopyrights

// =================================================================================================
// ExportQuickTimeItems
// ====================

static void ExportQuickTimeItems ( const SXMPMeta & xmp, TradQT_Manager * qtMgr, MOOV_Manager * moovMgr )
{

	// The QuickTime 'udta' timecode items are done here for simplicity.

	#define createWithZeroLang true

	qtMgr->ExportSimpleXMP ( kQTilst_Reel, xmp, kXMP_NS_DM, "tapeName" );
	qtMgr->ExportSimpleXMP ( kQTilst_Timecode, xmp, kXMP_NS_DM, "startTimecode/xmpDM:timeValue", createWithZeroLang );
	qtMgr->ExportSimpleXMP ( kQTilst_TimeScale, xmp, kXMP_NS_DM, "startTimeScale", createWithZeroLang );
	qtMgr->ExportSimpleXMP ( kQTilst_TimeSize, xmp, kXMP_NS_DM, "startTimeSampleSize", createWithZeroLang );

	qtMgr->UpdateChangedBoxes ( moovMgr );

}	// ExportQuickTimeItems

// =================================================================================================
// SelectTimeFormat
// ================

static const char * SelectTimeFormat ( const MPEG4_MetaHandler::TimecodeTrackInfo & tmcdInfo )
{
	const char * timeFormat = 0;

	float fltFPS = (float)tmcdInfo.timeScale / (float)tmcdInfo.frameDuration;
	int   intFPS = (int) (fltFPS + 0.5);

	switch ( intFPS ) {

		case 30:
			if ( fltFPS >= 29.985 ) {
				timeFormat = "30Timecode";
			} else if ( tmcdInfo.isDropFrame ) {
				timeFormat = "2997DropTimecode";
			} else {
				timeFormat = "2997NonDropTimecode";
			}
			break;

		case 24:
			if ( fltFPS >= 23.988 ) {
				timeFormat = "24Timecode";
			} else {
				timeFormat = "23976Timecode";
			}
			break;

		case 25:
			timeFormat = "25Timecode";
			break;

		case 50:
			timeFormat = "50Timecode";
			break;

		case 60:
			if ( fltFPS >= 59.97 ) {
				timeFormat = "60Timecode";
			} else if ( tmcdInfo.isDropFrame ) {
				timeFormat = "5994DropTimecode";
			} else {
				timeFormat = "5994NonDropTimecode";
			}
			break;

	}

	return timeFormat;

}	// SelectTimeFormat

// =================================================================================================
// SelectTimeFormat
// ================

static const char * SelectTimeFormat ( const SXMPMeta & xmp )
{
	bool ok;
	MPEG4_MetaHandler::TimecodeTrackInfo tmcdInfo;

	XMP_Int64 timeScale;
	ok = xmp.GetProperty_Int64 ( kXMP_NS_DM, "startTimeScale", &timeScale, 0 );
	if ( ! ok ) return 0;
	tmcdInfo.timeScale = (XMP_Uns32)timeScale;

	XMP_Int64 frameDuration;
	ok = xmp.GetProperty_Int64 ( kXMP_NS_DM, "startTimeSampleSize", &frameDuration, 0 );
	if ( ! ok ) return 0;
	tmcdInfo.frameDuration = (XMP_Uns32)frameDuration;

	std::string timecode;
	ok = xmp.GetProperty ( kXMP_NS_DM, "startTimecode/xmpDM:timeValue", &timecode, 0 );
	if ( ! ok ) return 0;
	if ( (timecode.size() == 11) && (timecode[8] == ';') ) tmcdInfo.isDropFrame = true;

	return SelectTimeFormat ( tmcdInfo );

}	// SelectTimeFormat

// =================================================================================================
// ComposeTimecode
// ===============

static const char * kDecDigits = "0123456789";
#define TwoDigits(val,str)	(str)[0] = kDecDigits[(val)/10]; (str)[1] = kDecDigits[(val)%10]

static bool ComposeTimecode ( const MPEG4_MetaHandler::TimecodeTrackInfo & tmcdInfo, std::string * strValue )
{
	float fltFPS = (float)tmcdInfo.timeScale / (float)tmcdInfo.frameDuration;
	int   intFPS = (int) (fltFPS + 0.5);
	if ( (intFPS != 30) && (intFPS != 24) && (intFPS != 25) && (intFPS != 50) && (intFPS != 60) ) return false;

	XMP_Uns32 framesPerDay = intFPS * 24*60*60;
	XMP_Uns32 dropLimit = 2;	// Used in the drop-frame correction.

	if ( tmcdInfo.isDropFrame ) {
		if ( intFPS == 30 ) {
			framesPerDay = 2589408;	// = 29.97 * 24*60*60
		} else if ( intFPS == 60 ) {
			framesPerDay = 5178816;	// = 59.94 * 24*60*60
			dropLimit = 4;
		} else {
			strValue->erase();
			return false;	// Dropframe can only apply to 29.97 and 59.94.
		}
	}

	XMP_Uns32 framesPerHour = framesPerDay / 24;
	XMP_Uns32 framesPerTenMinutes = framesPerHour / 6;
	XMP_Uns32 framesPerMinute = framesPerTenMinutes / 10;

	XMP_Uns32 frameCount = tmcdInfo.timecodeSample;
	while (frameCount >= framesPerDay ) frameCount -= framesPerDay;	// Normalize to be within 24 hours.

	XMP_Uns32 hours, minHigh, minLow, seconds;

	hours = frameCount / framesPerHour;
	frameCount -= (hours * framesPerHour);

	minHigh = frameCount / framesPerTenMinutes;
	frameCount -= (minHigh * framesPerTenMinutes);

	minLow = frameCount / framesPerMinute;
	frameCount -= (minLow * framesPerMinute);

	// Do some drop-frame corrections at this point: If this is drop-frame and the units of minutes
	// is non-zero, and the seconds are zero, and the frames are zero or one, the time is illegal.
	// Perform correction by subtracting 1 from the units of minutes and adding 1798 to the frames. 
	// For example, 1:00:00 becomes 59:28, and 1:00:01 becomes 59:29. A special case can occur for
	// when the frameCount just before the minHigh calculation is less than framesPerTenMinutes but
	// more than 10*framesPerMinute. This happens because of roundoff, and will result in a minHigh
	// of 0 and a minLow of 10. The drop frame correction must also be performed for this case.

	if ( tmcdInfo.isDropFrame ) {
		if ( (minLow == 10) || ((minLow != 0) && (frameCount < dropLimit)) ) {
			minLow -= 1;
			frameCount += framesPerMinute;
		}
	}

	seconds = frameCount / intFPS;
	frameCount -= (seconds * intFPS);

	if ( tmcdInfo.isDropFrame ) {
		*strValue = "hh;mm;ss;ff";
	} else {
		*strValue = "hh:mm:ss:ff";
	}

	char * str = (char*)strValue->c_str();
	TwoDigits ( hours, str );
	str[3] = kDecDigits[minHigh]; str[4] = kDecDigits[minLow];
	TwoDigits ( seconds, str+6 );
	TwoDigits ( frameCount, str+9 );

	return true;

}	// ComposeTimecode

// =================================================================================================
// DecomposeTimecode
// =================

static bool DecomposeTimecode ( const char * strValue, MPEG4_MetaHandler::TimecodeTrackInfo * tmcdInfo )
{
	float fltFPS = (float)tmcdInfo->timeScale / (float)tmcdInfo->frameDuration;
	int   intFPS = (int) (fltFPS + 0.5);
	if ( (intFPS != 30) && (intFPS != 24) && (intFPS != 25) && (intFPS != 50) && (intFPS != 60) ) return false;

	XMP_Uns32 framesPerDay = intFPS * 24*60*60;

	int items, hours, minutes, seconds, frames;

	if ( ! tmcdInfo->isDropFrame ) {
		items = sscanf ( strValue, "%d:%d:%d:%d", &hours, &minutes, &seconds, &frames );
	} else {
		items = sscanf ( strValue, "%d;%d;%d;%d", &hours, &minutes, &seconds, &frames );
		if ( intFPS == 30 ) {
			framesPerDay = 2589408;	// = 29.97 * 24*60*60
		} else if ( intFPS == 60 ) {
			framesPerDay = 5178816;	// = 59.94 * 24*60*60
		} else {
			return false;	// Dropframe can only apply to 29.97 and 59.94.
		}
	}

	if ( items != 4 ) return false;
	int minHigh = minutes / 10;
	int minLow = minutes % 10;

	XMP_Uns32 framesPerHour = framesPerDay / 24;
	XMP_Uns32 framesPerTenMinutes = framesPerHour / 6;
	XMP_Uns32 framesPerMinute = framesPerTenMinutes / 10;

	tmcdInfo->timecodeSample = (hours * framesPerHour) + (minHigh * framesPerTenMinutes) +
							   (minLow * framesPerMinute) + (seconds * intFPS) + frames;

	return true;

}	// DecomposeTimecode

// =================================================================================================
// FindTimecode_trak
// =================
//
// Look for a well-formed timecode track, return the trak box ref.

static MOOV_Manager::BoxRef FindTimecode_trak ( const MOOV_Manager & moovMgr )
{

	// Find a 'trak' box with a handler type of 'tmcd'.

	MOOV_Manager::BoxInfo moovInfo;
	MOOV_Manager::BoxRef  moovRef = moovMgr.GetBox ( "moov", &moovInfo );
	XMP_Assert ( moovRef != 0 );

	MOOV_Manager::BoxInfo trakInfo;
	MOOV_Manager::BoxRef  trakRef;

	size_t i = 0;
	for ( ; i < moovInfo.childCount; ++i ) {

		trakRef = moovMgr.GetNthChild ( moovRef, i, &trakInfo );
		if ( trakRef == 0 ) return 0;	// Sanity check, should not happen.
		if ( trakInfo.boxType != ISOMedia::k_trak ) continue;

		MOOV_Manager::BoxRef  innerRef;
		MOOV_Manager::BoxInfo innerInfo;

		innerRef = moovMgr.GetTypeChild ( trakRef, ISOMedia::k_mdia, &innerInfo );
		if ( innerRef == 0 ) continue;

		innerRef = moovMgr.GetTypeChild ( innerRef, ISOMedia::k_hdlr, &innerInfo );
		if ( (innerRef == 0) || (innerInfo.contentSize < sizeof ( MOOV_Manager::Content_hdlr )) ) continue;

		const MOOV_Manager::Content_hdlr * hdlr = (MOOV_Manager::Content_hdlr*) innerInfo.content;
		if ( hdlr->versionFlags != 0 ) continue;
		if ( GetUns32BE ( &hdlr->handlerType ) == ISOMedia::k_tmcd ) break;

	}

	if ( i == moovInfo.childCount ) return 0;
	return trakRef;

}	// FindTimecode_trak

// =================================================================================================
// FindTimecode_dref
// =================
//
// Look for the mdia/minf/dinf/dref box within a well-formed timecode track, return the dref box ref.

static MOOV_Manager::BoxRef FindTimecode_dref ( const MOOV_Manager & moovMgr )
{

	MOOV_Manager::BoxRef trakRef = FindTimecode_trak ( moovMgr );
	if ( trakRef == 0 ) return 0;

	MOOV_Manager::BoxInfo tempInfo;
	MOOV_Manager::BoxRef  tempRef, drefRef;

	tempRef = moovMgr.GetTypeChild ( trakRef, ISOMedia::k_mdia, &tempInfo );
	if ( tempRef == 0 ) return 0;

	tempRef = moovMgr.GetTypeChild ( tempRef, ISOMedia::k_minf, &tempInfo );
	if ( tempRef == 0 ) return 0;

	tempRef = moovMgr.GetTypeChild ( tempRef, ISOMedia::k_dinf, &tempInfo );
	if ( tempRef == 0 ) return 0;

	drefRef = moovMgr.GetTypeChild ( tempRef, ISOMedia::k_dref, &tempInfo );

	return drefRef;

}	// FindTimecode_dref

// =================================================================================================
// FindTimecode_stbl
// =================
//
// Look for the mdia/minf/stbl box within a well-formed timecode track, return the stbl box ref.

static MOOV_Manager::BoxRef FindTimecode_stbl ( const MOOV_Manager & moovMgr )
{

	MOOV_Manager::BoxRef trakRef = FindTimecode_trak ( moovMgr );
	if ( trakRef == 0 ) return 0;

	MOOV_Manager::BoxInfo tempInfo;
	MOOV_Manager::BoxRef  tempRef, stblRef;

	tempRef = moovMgr.GetTypeChild ( trakRef, ISOMedia::k_mdia, &tempInfo );
	if ( tempRef == 0 ) return 0;

	tempRef = moovMgr.GetTypeChild ( tempRef, ISOMedia::k_minf, &tempInfo );
	if ( tempRef == 0 ) return 0;

	stblRef = moovMgr.GetTypeChild ( tempRef, ISOMedia::k_stbl, &tempInfo );
	return stblRef;

}	// FindTimecode_stbl

// =================================================================================================
// FindTimecode_elst
// =================
//
// Look for the edts/elst box within a well-formed timecode track, return the elst box ref.

static MOOV_Manager::BoxRef FindTimecode_elst ( const MOOV_Manager & moovMgr )
{

	MOOV_Manager::BoxRef trakRef = FindTimecode_trak ( moovMgr );
	if ( trakRef == 0 ) return 0;

	MOOV_Manager::BoxInfo tempInfo;
	MOOV_Manager::BoxRef  tempRef, elstRef;

	tempRef = moovMgr.GetTypeChild ( trakRef, ISOMedia::k_edts, &tempInfo );
	if ( tempRef == 0 ) return 0;

	elstRef = moovMgr.GetTypeChild ( tempRef, ISOMedia::k_elst, &tempInfo );
	return elstRef;

}	// FindTimecode_elst

// =================================================================================================
// ImportTimecodeItems
// ===================

static bool ImportTimecodeItems ( const MPEG4_MetaHandler::TimecodeTrackInfo & tmcdInfo,
								  const TradQT_Manager & qtInfo, SXMPMeta * xmp )
{
	std::string xmpValue;
	bool haveItem;
	bool haveImports = false;

	// The QT user data item '©REL' goes into xmpDM:tapeName, and the 'name' box at the end of the
	// timecode sample description goes into xmpDM:altTapeName.
	haveImports |= qtInfo.ImportSimpleXMP ( kQTilst_Reel, xmp, kXMP_NS_DM, "tapeName" );
	if ( ! tmcdInfo.macName.empty() ) {
		haveItem = ConvertFromMacLang ( tmcdInfo.macName, tmcdInfo.macLang, &xmpValue );
		if ( haveItem ) {
			xmp->SetProperty ( kXMP_NS_DM, "altTapeName", xmpValue.c_str() );
			haveImports = true;
		}
	}

	// The QT user data item '©TSC' goes into xmpDM:startTimeScale. If that isn't present, then
	// the timecode sample description's timeScale is used.
	haveItem = qtInfo.ImportSimpleXMP ( kQTilst_TimeScale, xmp, kXMP_NS_DM, "startTimeScale" );
	if ( tmcdInfo.stsdBoxFound & (! haveItem) ) {
		xmp->SetProperty_Int64 ( kXMP_NS_DM, "startTimeScale", tmcdInfo.timeScale );
		haveItem = true;
	}
	haveImports |= haveItem;

	// The QT user data item '©TSZ' goes into xmpDM:startTimeSampleSize. If that isn't present, then
	// the timecode sample description's frameDuration is used.
	haveItem = qtInfo.ImportSimpleXMP ( kQTilst_TimeSize, xmp, kXMP_NS_DM, "startTimeSampleSize" );
	if ( tmcdInfo.stsdBoxFound & (! haveItem) ) {
		xmp->SetProperty_Int64 ( kXMP_NS_DM, "startTimeSampleSize", tmcdInfo.frameDuration );
		haveItem = true;
	}
	haveImports |= haveItem;

	const char * timeFormat;

	// The Timecode struct type is used for xmpDM:startTimecode and xmpDM:altTimecode. For both, only
	// the xmpDM:timeValue and xmpDM:timeFormat fields are set.

	// The QT user data item '©TIM' goes into xmpDM:startTimecode/xmpDM:timeValue. This is an already
	// formatted timecode string. The XMP values of xmpDM:startTimeScale, xmpDM:startTimeSampleSize,
	// and xmpDM:startTimecode/xmpDM:timeValue are used to select the timeFormat value.
	haveImports |= qtInfo.ImportSimpleXMP ( kQTilst_Timecode, xmp, kXMP_NS_DM, "startTimecode/xmpDM:timeValue" );
	timeFormat = SelectTimeFormat ( *xmp );
	if ( timeFormat != 0 ) {
		xmp->SetProperty ( kXMP_NS_DM, "startTimecode/xmpDM:timeFormat", timeFormat );
		haveImports = true;
	}

	if ( tmcdInfo.stsdBoxFound ) {

		haveItem = ComposeTimecode ( tmcdInfo, &xmpValue );
		if ( haveItem ) {
			xmp->SetProperty ( kXMP_NS_DM, "altTimecode/xmpDM:timeValue", xmpValue.c_str() );
			haveImports = true;
		}

		timeFormat = SelectTimeFormat ( tmcdInfo );
		if ( timeFormat != 0 ) {
			xmp->SetProperty ( kXMP_NS_DM, "altTimecode/xmpDM:timeFormat", timeFormat );
			haveImports = true;
		}

	}

	return haveImports;

}	// ImportTimecodeItems

// =================================================================================================
// ExportTimecodeItems
// ===================

static void ExportTimecodeItems ( const SXMPMeta & xmp, MPEG4_MetaHandler::TimecodeTrackInfo * tmcdInfo,
								  TradQT_Manager * qtMgr, MOOV_Manager * moovMgr )
{
	// Export the items that go into the timecode track:
	//  - the timescale and frame duration in the first 'stsd' table entry
	//  - the 'name' box appended to the first 'stsd' table entry
	//  - the first timecode sample
	// ! The QuickTime 'udta' timecode items are handled in ExportQuickTimeItems.

	if ( ! tmcdInfo->stsdBoxFound ) return;	// Don't make changes unless there is a well-formed timecode track.

	MOOV_Manager::BoxRef stblRef = FindTimecode_stbl ( *moovMgr );
	if ( stblRef == 0 ) return;

	MOOV_Manager::BoxInfo stsdInfo;
	MOOV_Manager::BoxRef  stsdRef;

	stsdRef = moovMgr->GetTypeChild ( stblRef, ISOMedia::k_stsd, &stsdInfo );
	if ( stsdRef == 0 ) return;
	if ( stsdInfo.contentSize < (8 + sizeof ( MOOV_Manager::Content_stsd_entry )) ) return;
	if ( GetUns32BE ( stsdInfo.content + 4 ) == 0 ) return;	// Make sure the entry count is non-zero.

	MOOV_Manager::Content_stsd_entry * stsdRawEntry = (MOOV_Manager::Content_stsd_entry*) (stsdInfo.content + 8);

	XMP_Uns32 stsdEntrySize = GetUns32BE ( &stsdRawEntry->entrySize );
	if ( stsdEntrySize > (stsdInfo.contentSize - 4) ) stsdEntrySize = stsdInfo.contentSize - 4;
	if ( stsdEntrySize < sizeof ( MOOV_Manager::Content_stsd_entry ) ) return;

	bool ok, haveScale = false, haveDuration = false;
	std::string xmpValue;
	XMP_Int64 int64;	// Used to allow UInt32 values, GetProperty_Int is SInt32.

	// The tmcdInfo timeScale field is set from xmpDM:startTimeScale.
	 ok = xmp.GetProperty_Int64 ( kXMP_NS_DM, "startTimeScale", &int64, 0 );
	 if ( ok && (int64 <= 0xFFFFFFFF) ) {
	 	haveScale = true;
	 	if ( tmcdInfo->timeScale != 0 ) { // Entry must not be created if not existing before
			tmcdInfo->timeScale = (XMP_Uns32)int64;
			PutUns32BE ( tmcdInfo->timeScale, (void*)&stsdRawEntry->timeScale );
			moovMgr->NoteChange();
	 	}
	 }

	// The tmcdInfo frameDuration field is set from xmpDM:startTimeSampleSize.
	 ok = xmp.GetProperty_Int64 ( kXMP_NS_DM, "startTimeSampleSize", &int64, 0 );
	 if ( ok && (int64 <= 0xFFFFFFFF) ) {
	 	haveDuration = true;
	 	if ( tmcdInfo->frameDuration != 0 ) { // Entry must not be created if not existing before
			tmcdInfo->frameDuration = (XMP_Uns32)int64;
			PutUns32BE ( tmcdInfo->frameDuration, (void*)&stsdRawEntry->frameDuration );
			moovMgr->NoteChange();
	 	}
	 }
	
	 // The tmcdInfo frameCount field is a simple ratio of the timeScale and frameDuration.
	 if ( (haveScale & haveDuration) && (tmcdInfo->frameDuration != 0) ) {
	 	float floatScale = (float) tmcdInfo->timeScale;
	 	float floatDuration = (float) tmcdInfo->frameDuration;
		XMP_Uns8 newCount = (XMP_Uns8) ( (floatScale / floatDuration) + 0.5 );
		if ( newCount != stsdRawEntry->frameCount ) {
			stsdRawEntry->frameCount = newCount;
			moovMgr->NoteChange();
		}
	 }

	// The tmcdInfo isDropFrame flag is set from xmpDM:altTimecode/xmpDM:timeValue. The timeScale
	// and frameDuration must be updated first, they are used by DecomposeTimecode. Compute the new
	// UInt32 timecode sample, but it gets written to the file later by UpdateFile.

	ok = xmp.GetProperty ( kXMP_NS_DM, "altTimecode/xmpDM:timeValue", &xmpValue, 0 );
	if ( ok && (xmpValue.size() == 11) ) {

		bool oldDropFrame = tmcdInfo->isDropFrame;
		tmcdInfo->isDropFrame = false;
		if ( xmpValue[8] == ';' ) tmcdInfo->isDropFrame = true;
		if ( oldDropFrame != tmcdInfo->isDropFrame ) {
			XMP_Uns32 flags = GetUns32BE ( &stsdRawEntry->flags );
			flags = (flags & 0xFFFFFFFE) | (XMP_Uns32)tmcdInfo->isDropFrame;
			PutUns32BE ( flags, (void*)&stsdRawEntry->flags );
		 	moovMgr->NoteChange();
		}

		XMP_Uns32 oldSample = tmcdInfo->timecodeSample;
		ok = DecomposeTimecode ( xmpValue.c_str(), tmcdInfo );
	 	if ( ok && (oldSample != tmcdInfo->timecodeSample) ) moovMgr->NoteChange();

	}

	// The 'name' box attached to the first 'stsd' table entry is set from xmpDM:altTapeName.

	bool replaceNameBox = false;

	ok = xmp.GetProperty ( kXMP_NS_DM, "altTapeName", &xmpValue, 0 );
	if ( (! ok) || xmpValue.empty() ) {
		if ( tmcdInfo->nameOffset != 0 ) replaceNameBox = true;	// No XMP, get rid of existing name.
	} else {
		std::string macValue;
		ok = ConvertToMacLang ( xmpValue, tmcdInfo->macLang, &macValue );
		if ( ok && (macValue != tmcdInfo->macName) ) {
			tmcdInfo->macName = macValue;
			replaceNameBox = true;	// Write changed name.
		}
	}

	if ( replaceNameBox ) {

		// To replace the 'name' box we have to create an entire new 'stsd' box, and attach the
		// new name to the first 'stsd' table entry. The 'name' box content is a UInt16 text length,
		// UInt16 language code, and Mac encoded text with no nul termination.

		if ( tmcdInfo->macName.size() > 0xFFFF ) tmcdInfo->macName.erase ( 0xFFFF );

		ISOMedia::BoxInfo oldNameInfo;
		XMP_Assert ( (oldNameInfo.headerSize == 0) && (oldNameInfo.contentSize == 0) );
		if ( tmcdInfo->nameOffset != 0 ) {
			const XMP_Uns8 * oldNamePtr = stsdInfo.content + tmcdInfo->nameOffset;
			const XMP_Uns8 * oldNameLimit = stsdInfo.content + stsdInfo.contentSize;
			(void) ISOMedia::GetBoxInfo ( oldNamePtr, oldNameLimit, &oldNameInfo );
		}

		XMP_Uns32 oldNameBoxSize = (XMP_Uns32)oldNameInfo.headerSize + (XMP_Uns32)oldNameInfo.contentSize;
		XMP_Uns32 newNameBoxSize = 0;
		if ( ! tmcdInfo->macName.empty() ) newNameBoxSize = 4+4 + 2+2 + (XMP_Uns32)tmcdInfo->macName.size();

		XMP_Uns32 stsdNewContentSize = stsdInfo.contentSize - oldNameBoxSize + newNameBoxSize;
		RawDataBlock stsdNewContent;
		stsdNewContent.assign ( stsdNewContentSize, 0 );	// Get the space allocated, direct fill below.

		XMP_Uns32 stsdPrefixSize = tmcdInfo->nameOffset;
		if ( tmcdInfo->nameOffset == 0 ) stsdPrefixSize = 4+4 + sizeof ( MOOV_Manager::Content_stsd_entry );

		XMP_Uns32 oldSuffixOffset = stsdPrefixSize + oldNameBoxSize;
		XMP_Uns32 newSuffixOffset = stsdPrefixSize + newNameBoxSize;
		XMP_Uns32 stsdSuffixSize  = stsdInfo.contentSize - oldSuffixOffset;

		memcpy ( &stsdNewContent[0], stsdInfo.content, stsdPrefixSize );
		if ( stsdSuffixSize != 0 ) memcpy ( &stsdNewContent[newSuffixOffset], (stsdInfo.content + oldSuffixOffset), stsdSuffixSize );

		XMP_Uns32 newEntrySize = stsdEntrySize - oldNameBoxSize + newNameBoxSize;
		MOOV_Manager::Content_stsd_entry * stsdNewEntry = (MOOV_Manager::Content_stsd_entry*) (&stsdNewContent[0] + 8);
		PutUns32BE ( newEntrySize, &stsdNewEntry->entrySize );

		if ( newNameBoxSize != 0 ) {
			PutUns32BE ( newNameBoxSize, &stsdNewContent[stsdPrefixSize] );
			PutUns32BE ( ISOMedia::k_name, &stsdNewContent[stsdPrefixSize+4] );
			PutUns16BE ( (XMP_Uns16)tmcdInfo->macName.size(), &stsdNewContent[stsdPrefixSize+8] );
			PutUns16BE ( tmcdInfo->macLang, &stsdNewContent[stsdPrefixSize+10] );
			memcpy ( &stsdNewContent[stsdPrefixSize+12], tmcdInfo->macName.c_str(), tmcdInfo->macName.size() );
		}

		moovMgr->SetBox ( stsdRef, &stsdNewContent[0], stsdNewContentSize );

	}

}	// ExportTimecodeItems

// =================================================================================================
// ImportCr8rItems
// ===============

#if SUNOS_SPARC || SUNOS_X86
#pragma pack ( 1 )
#else
#pragma pack ( push, 1 )
#endif //#if SUNOS_SPARC || SUNOS_X86

struct PrmLBoxContent {
	XMP_Uns32 magic;
	XMP_Uns32 size;
	XMP_Uns16 verAPI;
	XMP_Uns16 verCode;
	XMP_Uns32 exportType;
	XMP_Uns16 MacVRefNum;
	XMP_Uns32 MacParID;
	char filePath[260];
};

enum { kExportTypeMovie = 0, kExportTypeStill = 1, kExportTypeAudio = 2, kExportTypeCustom = 3 };

struct Cr8rBoxContent {
	XMP_Uns32 magic;
	XMP_Uns32 size;
	XMP_Uns16 majorVer;
	XMP_Uns16 minorVer;
	XMP_Uns32 creatorCode;
	XMP_Uns32 appleEvent;
	char fileExt[16];
	char appOptions[16];
	char appName[32];
};

#if SUNOS_SPARC || SUNOS_X86
#pragma pack ( )
#else
#pragma pack ( pop )
#endif //#if SUNOS_SPARC || SUNOS_X86

// -------------------------------------------------------------------------------------------------

static bool ImportCr8rItems ( const MOOV_Manager & moovMgr, SXMPMeta * xmp )
{
	bool haveXMP = false;
	std::string fieldPath;

	MOOV_Manager::BoxInfo infoPrmL, infoCr8r;
	MOOV_Manager::BoxRef  refPrmL = moovMgr.GetBox ( "moov/udta/PrmL", &infoPrmL );
	MOOV_Manager::BoxRef  refCr8r = moovMgr.GetBox ( "moov/udta/Cr8r", &infoCr8r );

	bool havePrmL = ( (refPrmL != 0) && (infoPrmL.contentSize == sizeof ( PrmLBoxContent )) );
	bool haveCr8r = ( (refCr8r != 0) && (infoCr8r.contentSize == sizeof ( Cr8rBoxContent )) );

	if ( havePrmL ) {

		PrmLBoxContent rawPrmL;
		XMP_Assert ( sizeof ( rawPrmL ) == 282 );
		XMP_Assert ( sizeof ( rawPrmL.filePath ) == 260 );
		memcpy ( &rawPrmL, infoPrmL.content, sizeof ( rawPrmL ) );
		if ( rawPrmL.magic != 0xBEEFCAFE ) {
			Flip4 ( &rawPrmL.exportType );	// The only numeric field that we care about.
		}

		rawPrmL.filePath[259] = 0;	// Ensure a terminating nul.
		if ( rawPrmL.filePath[0] != 0 ) {
			if ( rawPrmL.filePath[0] == '/' ) {
				haveXMP = true;
				SXMPUtils::ComposeStructFieldPath ( kXMP_NS_CreatorAtom, "macAtom",
												    kXMP_NS_CreatorAtom, "posixProjectPath", &fieldPath );
				if ( ! xmp->DoesPropertyExist ( kXMP_NS_CreatorAtom, fieldPath.c_str() ) ) {
					xmp->SetProperty ( kXMP_NS_CreatorAtom, fieldPath.c_str(), rawPrmL.filePath );
				}
			} else if ( XMP_LitNMatch ( rawPrmL.filePath, "\\\\?\\", 4 ) ) {
				haveXMP = true;
				SXMPUtils::ComposeStructFieldPath ( kXMP_NS_CreatorAtom, "windowsAtom",
												    kXMP_NS_CreatorAtom, "uncProjectPath", &fieldPath );
				if ( ! xmp->DoesPropertyExist ( kXMP_NS_CreatorAtom, fieldPath.c_str() ) ) {
					xmp->SetProperty ( kXMP_NS_CreatorAtom, fieldPath.c_str(), rawPrmL.filePath );
				}
			}
		}

		const char * exportStr = 0;
		switch ( rawPrmL.exportType ) {
			case kExportTypeMovie  : exportStr = "movie";  break;
			case kExportTypeStill  : exportStr = "still";  break;
			case kExportTypeAudio  : exportStr = "audio";  break;
			case kExportTypeCustom : exportStr = "custom"; break;
		}
		if ( exportStr != 0 ) {
			haveXMP = true;
			SXMPUtils::ComposeStructFieldPath ( kXMP_NS_DM, "projectRef", kXMP_NS_DM, "type", &fieldPath );
			if ( ! xmp->DoesPropertyExist ( kXMP_NS_DM, fieldPath.c_str() ) ) {
				xmp->SetProperty ( kXMP_NS_DM, fieldPath.c_str(), exportStr );
			}
		}

	}

	if ( haveCr8r ) {

		Cr8rBoxContent rawCr8r;
		XMP_Assert ( sizeof ( rawCr8r ) == 84 );
		XMP_Assert ( sizeof ( rawCr8r.fileExt ) == 16 );
		XMP_Assert ( sizeof ( rawCr8r.appOptions ) == 16 );
		XMP_Assert ( sizeof ( rawCr8r.appName ) == 32 );
		memcpy ( &rawCr8r, infoCr8r.content, sizeof ( rawCr8r ) );
		if ( rawCr8r.magic != 0xBEEFCAFE ) {
			Flip4 ( &rawCr8r.creatorCode );	// The only numeric fields that we care about.
			Flip4 ( &rawCr8r.appleEvent );
		}

		std::string fieldPath;

		SXMPUtils::ComposeStructFieldPath ( kXMP_NS_CreatorAtom, "macAtom", kXMP_NS_CreatorAtom, "applicationCode", &fieldPath );
		if ( (rawCr8r.creatorCode != 0) && (! xmp->DoesPropertyExist ( kXMP_NS_CreatorAtom, fieldPath.c_str() )) ) {
			haveXMP = true;
			xmp->SetProperty_Int64 ( kXMP_NS_CreatorAtom, fieldPath.c_str(), (XMP_Int64)rawCr8r.creatorCode );	// ! Unsigned trickery.
		}

		SXMPUtils::ComposeStructFieldPath ( kXMP_NS_CreatorAtom, "macAtom", kXMP_NS_CreatorAtom, "invocationAppleEvent", &fieldPath );
		if ( (rawCr8r.appleEvent != 0) && (! xmp->DoesPropertyExist ( kXMP_NS_CreatorAtom, fieldPath.c_str() )) ) {
			haveXMP = true;
			xmp->SetProperty_Int64 ( kXMP_NS_CreatorAtom, fieldPath.c_str(), (XMP_Int64)rawCr8r.appleEvent );	// ! Unsigned trickery.
		}

		rawCr8r.fileExt[15] = 0;	// Ensure a terminating nul.
		SXMPUtils::ComposeStructFieldPath ( kXMP_NS_CreatorAtom, "windowsAtom", kXMP_NS_CreatorAtom, "extension", &fieldPath );
		if ( (rawCr8r.fileExt[0] != 0) && (! xmp->DoesPropertyExist ( kXMP_NS_CreatorAtom, fieldPath.c_str() )) ) {
			haveXMP = true;
			xmp->SetProperty ( kXMP_NS_CreatorAtom, fieldPath.c_str(), rawCr8r.fileExt );
		}

		rawCr8r.appOptions[15] = 0;	// Ensure a terminating nul.
		SXMPUtils::ComposeStructFieldPath ( kXMP_NS_CreatorAtom, "windowsAtom", kXMP_NS_CreatorAtom, "invocationFlags", &fieldPath );
		if ( (rawCr8r.appOptions[0] != 0) && (! xmp->DoesPropertyExist ( kXMP_NS_CreatorAtom, fieldPath.c_str() )) ) {
			haveXMP = true;
			xmp->SetProperty ( kXMP_NS_CreatorAtom, fieldPath.c_str(), rawCr8r.appOptions );
		}

		rawCr8r.appName[31] = 0;	// Ensure a terminating nul.
		if ( (rawCr8r.appName[0] != 0) && (! xmp->DoesPropertyExist ( kXMP_NS_XMP, "CreatorTool" )) ) {
			haveXMP = true;
			xmp->SetProperty ( kXMP_NS_XMP, "CreatorTool", rawCr8r.appName );
		}

	}

	return haveXMP;

}	// ImportCr8rItems

// =================================================================================================
// ExportCr8rItems
// ===============

static inline void SetBufferedString ( char * dest, const std::string source, size_t limit )
{
	memset ( dest, 0, limit );
	size_t count = source.size();
	if ( count >= limit ) count = limit - 1;	// Ensure a terminating nul.
	memcpy ( dest, source.c_str(), count );
}

// -------------------------------------------------------------------------------------------------

static void ExportCr8rItems ( const SXMPMeta & xmp, MOOV_Manager * moovMgr )
{
	bool haveNewCr8r = false;
	std::string creatorCode, appleEvent, fileExt, appOptions, appName;

	haveNewCr8r |= xmp.GetStructField ( kXMP_NS_CreatorAtom, "macAtom", kXMP_NS_CreatorAtom, "applicationCode", &creatorCode, 0 );
	haveNewCr8r |= xmp.GetStructField ( kXMP_NS_CreatorAtom, "macAtom", kXMP_NS_CreatorAtom, "invocationAppleEvent", &appleEvent, 0 );
	haveNewCr8r |= xmp.GetStructField ( kXMP_NS_CreatorAtom, "windowsAtom", kXMP_NS_CreatorAtom, "extension", &fileExt, 0 );
	haveNewCr8r |= xmp.GetStructField ( kXMP_NS_CreatorAtom, "windowsAtom", kXMP_NS_CreatorAtom, "invocationFlags", &appOptions, 0 );
	haveNewCr8r |= xmp.GetProperty ( kXMP_NS_XMP, "CreatorTool", &appName, 0 );

	MOOV_Manager::BoxInfo infoCr8r;
	MOOV_Manager::BoxRef  refCr8r = moovMgr->GetBox ( "moov/udta/Cr8r", &infoCr8r );
	bool haveOldCr8r = ( (refCr8r != 0) && (infoCr8r.contentSize == sizeof ( Cr8rBoxContent )) );

	if ( ! haveNewCr8r ) {
		if ( haveOldCr8r ) {
			MOOV_Manager::BoxRef udtaRef = moovMgr->GetBox ( "moov/udta", 0 );
			moovMgr->DeleteTypeChild ( udtaRef, 0x43723872 /* 'Cr8r' */ );
		}
		return;
	}

	Cr8rBoxContent newCr8r;
	const Cr8rBoxContent * oldCr8r = (Cr8rBoxContent*) infoCr8r.content;

	if ( ! haveOldCr8r ) {

		memset ( &newCr8r, 0, sizeof(newCr8r) );
		newCr8r.magic = MakeUns32BE ( 0xBEEFCAFE );
		newCr8r.size = MakeUns32BE ( sizeof ( newCr8r ) );
		newCr8r.majorVer = MakeUns16BE ( 1 );

	} else {

		memcpy ( &newCr8r, oldCr8r, sizeof(newCr8r) );
		if ( GetUns32BE ( &newCr8r.magic ) != 0xBEEFCAFE ) {	// Make sure we write BE numbers.
			Flip4 ( &newCr8r.magic );
			Flip4 ( &newCr8r.size );
			Flip2 ( &newCr8r.majorVer );
			Flip2 ( &newCr8r.minorVer );
			Flip4 ( &newCr8r.creatorCode );
			Flip4 ( &newCr8r.appleEvent );
		}

	}

	if ( ! creatorCode.empty() ) {
		newCr8r.creatorCode = MakeUns32BE ( (XMP_Uns32) strtoul ( creatorCode.c_str(), 0, 0 ) );
	}

	if ( ! appleEvent.empty() ) {
		newCr8r.appleEvent = MakeUns32BE ( (XMP_Uns32) strtoul ( appleEvent.c_str(), 0, 0 ) );
	}

	if ( ! fileExt.empty() ) SetBufferedString ( newCr8r.fileExt, fileExt, sizeof ( newCr8r.fileExt ) );
	if ( ! appOptions.empty() ) SetBufferedString ( newCr8r.appOptions, appOptions, sizeof ( newCr8r.appOptions ) );
	if ( ! appName.empty() ) SetBufferedString ( newCr8r.appName, appName, sizeof ( newCr8r.appName ) );

	moovMgr->SetBox ( "moov/udta/Cr8r", &newCr8r, sizeof(newCr8r) );

}	// ExportCr8rItems

// =================================================================================================
// GetAtomInfo
// ===========

struct AtomInfo {
	XMP_Int64 atomSize;
	XMP_Uns32 atomType;
	bool hasLargeSize;
};

enum {	// ! Do not rearrange, code depends on this order.
	kBadQT_NoError		= 0,	// No errors.
	kBadQT_SmallInner	= 1,	// An extra 1..7 bytes at the end of an inner span.
	kBadQT_LargeInner	= 2,	// More serious inner garbage, found as invalid atom length.
	kBadQT_SmallOuter	= 3,	// An extra 1..7 bytes at the end of the file.
	kBadQT_LargeOuter	= 4		// More serious EOF garbage, found as invalid atom length.
};
typedef XMP_Uns8 QTErrorMode;

static QTErrorMode GetAtomInfo ( XMP_IO* qtFile, XMP_Int64 spanSize, int nesting, AtomInfo * info )
{
	QTErrorMode status = kBadQT_NoError;
	XMP_Uns8 buffer [8];

	info->hasLargeSize = false;

	qtFile->ReadAll ( buffer, 8 );	// Will throw if 8 bytes aren't available.
	info->atomSize = GetUns32BE ( &buffer[0] );	// ! Yes, the initial size is big endian UInt32.
	info->atomType = GetUns32BE ( &buffer[4] );

	if ( info->atomSize == 0 ) {	// Does the atom extend to EOF?

		if ( nesting != 0 ) return kBadQT_LargeInner;
		info->atomSize = spanSize;	// This outer atom goes to EOF.

	} else if ( info->atomSize == 1 ) {	// Does the atom have a 64-bit size?

		if ( spanSize < 16 ) {	// Is there room in the span for the 16 byte header?
			status = kBadQT_LargeInner;
			if ( nesting == 0 ) status += 2;	// Convert to "outer".
			return status;
		}

		qtFile->ReadAll ( buffer, 8 );
		info->atomSize = (XMP_Int64) GetUns64BE ( &buffer[0] );
		info->hasLargeSize = true;

	}

	XMP_Assert ( status == kBadQT_NoError );
	return status;

}	// GetAtomInfo

// =================================================================================================
// CheckAtomList
// =============
//
// Check that a sequence of atoms fills a given span. The I/O position must be at the start of the
// span, it is left just past the span on success. Recursive checks are done for top level 'moov'
// atoms, and second level 'udta' atoms ('udta' inside 'moov').
//
// Checking continues for "small inner" errors. They will be reported if no other kinds of errors
// are found, otherwise the other error is reported. Checking is immediately aborted for any "large"
// error. The rationale is that QuickTime can apparently handle small inner errors. They might be
// arise from updates that shorten an atom by less than 8 bytes. Larger shrinkage should introduce a
// 'free' atom.

static QTErrorMode CheckAtomList ( XMP_IO* qtFile, XMP_Int64 spanSize, int nesting )
{
	QTErrorMode status = kBadQT_NoError;
	AtomInfo    info;

	const static XMP_Uns32 moovAtomType = 0x6D6F6F76;	// ! Don't use MakeUns32BE, already big endian.
	const static XMP_Uns32 udtaAtomType = 0x75647461;

	for ( ; spanSize >= 8; spanSize -= info.atomSize ) {

		QTErrorMode atomStatus = GetAtomInfo ( qtFile, spanSize, nesting, &info );
		if ( atomStatus != kBadQT_NoError ) return atomStatus;

		XMP_Int64 headerSize = 8;
		if ( info.hasLargeSize ) headerSize = 16;

		if ( (info.atomSize < headerSize) || (info.atomSize > spanSize) ) {
			status = kBadQT_LargeInner;
			if ( nesting == 0 ) status += 2;	// Convert to "outer".
			return status;
		}

		bool doChildren = false;
		if ( (nesting == 0) && (info.atomType == moovAtomType) ) doChildren = true;
		if ( (nesting == 1) && (info.atomType == udtaAtomType) ) doChildren = true;

		XMP_Int64 dataSize = info.atomSize - headerSize;

		if ( ! doChildren ) {
			qtFile->Seek ( dataSize, kXMP_SeekFromCurrent );
		} else {
			QTErrorMode innerStatus = CheckAtomList ( qtFile, dataSize, nesting+1 );
			if ( innerStatus > kBadQT_SmallInner ) return innerStatus;	// Quit for serious errors.
			if ( status == kBadQT_NoError ) status = innerStatus;	// Remember small inner errors.
		}

	}

	XMP_Assert ( status <= kBadQT_SmallInner );	// Else already returned.
	// ! Make sure inner kBadQT_SmallInner is propagated if this span is OK.

	if ( spanSize != 0 ) {
		qtFile->Seek ( spanSize, kXMP_SeekFromCurrent );	// ! Skip the trailing garbage of this span.
		status = kBadQT_SmallInner;
		if ( spanSize >= 8 ) status = kBadQT_LargeInner;
		if ( nesting == 0 ) status += 2;	// Convert to "outer".
	}

	return status;

}	// CheckAtomList

// =================================================================================================
// AttemptFileRepair
// =================

static void AttemptFileRepair ( XMP_IO* qtFile, XMP_Int64 fileSpace, QTErrorMode status )
{

	switch ( status ) {
		case kBadQT_NoError    : return;	// Sanity check.
		case kBadQT_SmallInner : return;	// Fixed in normal update code for the 'udta' box.
		case kBadQT_LargeInner : XMP_Throw ( "Can't repair QuickTime file", kXMPErr_BadFileFormat );
		case kBadQT_SmallOuter : break;		// Truncate file below.
		case kBadQT_LargeOuter : break;		// Truncate file below.
		default                : XMP_Throw ( "Invalid QuickTime error mode", kXMPErr_InternalFailure );
	}

	AtomInfo info;
	XMP_Int64 headerSize;

	// Process the top level atoms until an error is found.

	qtFile->Rewind();

	for ( ; fileSpace >= 8; fileSpace -= info.atomSize ) {

		QTErrorMode atomStatus = GetAtomInfo ( qtFile, fileSpace, 0, &info );

		headerSize = 8;	// ! Set this before checking atomStatus, used after the loop.
		if ( info.hasLargeSize ) headerSize = 16;

		if ( atomStatus != kBadQT_NoError ) break;
		if ( (info.atomSize < headerSize) || (info.atomSize > fileSpace) ) break;

		XMP_Int64 dataSize = info.atomSize - headerSize;
		qtFile->Seek ( dataSize, kXMP_SeekFromCurrent );

	}

	// Truncate the file. If fileSpace >= 8 then the loop exited early due to a bad atom, seek back
	// to the atom's start. Otherwise, the loop exited because no more atoms are possible, no seek.

	if ( fileSpace >= 8 ) qtFile->Seek ( -headerSize, kXMP_SeekFromCurrent );
	XMP_Int64 currPos = qtFile->Offset();
	qtFile->Truncate ( currPos );

}	// AttemptFileRepair

// =================================================================================================
// CheckQTFileStructure
// ====================

static void CheckQTFileStructure ( XMPFileHandler * thiz, bool doRepair )
{
	XMPFiles * parent = thiz->parent;
	XMP_IO* fileRef  = parent->ioRef;
	XMP_Int64 fileSize = fileRef->Length();

	// Check the basic file structure and try to repair if asked.

	fileRef->Rewind();
	QTErrorMode status = CheckAtomList ( fileRef, fileSize, 0 );

	if ( status != kBadQT_NoError ) {
		if ( doRepair || (status == kBadQT_SmallInner) || (status == kBadQT_SmallOuter) ) {
			AttemptFileRepair ( fileRef, fileSize, status );	// Will throw if the attempt fails.
		} else if ( status != kBadQT_SmallInner ) {
			XMP_Throw ( "Ill-formed QuickTime file", kXMPErr_BadFileFormat );
		} else {
			return;	// ! Ignore these, QT seems to be able to handle them.
			// *** Might want to throw for check-only, ignore when repairing.
		}
	}

}	// CheckQTFileStructure;

// =================================================================================================
// CheckFinalBox
// =============
//
// Before appending anything new, check if the final top level box has a "to EoF" length. If so, fix
// it to have an explicit length.

static void CheckFinalBox ( XMP_IO* fileRef, XMP_AbortProc abortProc, void * abortArg )
{
	const bool checkAbort = (abortProc != 0);

	XMP_Uns64 fileSize = fileRef->Length();

	// Find the last 2 boxes in the file. Need the previous to last in case it is an Apple 'wide' box.

	XMP_Uns64 prevPos, lastPos, nextPos;
	ISOMedia::BoxInfo prevBox, lastBox;
	XMP_Uns8 buffer [16];	// Enough to create an extended header.

	memset ( &prevBox, 0, sizeof(prevBox) );	// AUDIT: Using sizeof(prevBox) is safe.
	memset ( &lastBox, 0, sizeof(lastBox) );	// AUDIT: Using sizeof(lastBox) is safe.
	prevPos = lastPos = nextPos = 0;
	while ( nextPos != fileSize ) {
		if ( checkAbort && abortProc(abortArg) ) {
			XMP_Throw ( "MPEG4_MetaHandler::CheckFinalBox - User abort", kXMPErr_UserAbort );
		}
		prevBox = lastBox;
		prevPos = lastPos;
		lastPos = nextPos;
		nextPos = ISOMedia::GetBoxInfo ( fileRef, lastPos, fileSize, &lastBox, true /* throw errors */ );
	}

	// See if the last box is valid and has a "to EoF" size.

	if ( lastBox.headerSize < 8 ) XMP_Throw ( "MPEG-4 final box is invalid", kXMPErr_EnforceFailure );
	fileRef->Seek ( lastPos, kXMP_SeekFromStart );
	fileRef->Read ( buffer, 4 );
	XMP_Uns64 lastSize = GetUns32BE ( &buffer[0] );	// ! Yes, the file has a 32-bit value.
	if ( lastSize != 0 ) return;

	// Have a final "to EoF" box, try to write the explicit size.

	lastSize = lastBox.headerSize + lastBox.contentSize;
	if ( lastSize <= 0xFFFFFFFFUL ) {

		// Fill in the 32-bit exact size.
		PutUns32BE ( (XMP_Uns32)lastSize, &buffer[0] );
		fileRef->Seek ( lastPos, kXMP_SeekFromStart );
		fileRef->Write ( buffer, 4 );

	} else {

		// Try to convert to using an extended header.

		if ( (prevBox.boxType != ISOMedia::k_wide) || (prevBox.headerSize != 8) || (prevBox.contentSize != 0) ) {
			XMP_Throw ( "Can't expand final box header", kXMPErr_EnforceFailure );
		}
		XMP_Assert ( prevPos == (lastPos - 8) );

		PutUns32BE ( 1, &buffer[0] );
		PutUns32BE ( lastBox.boxType, &buffer[4] );
		PutUns64BE ( lastSize, &buffer[8] );
		fileRef->Seek ( prevPos, kXMP_SeekFromStart );
		fileRef->Write ( buffer, 16 );

	}

}	// CheckFinalBox

// =================================================================================================
// WriteBoxHeader
// ==============

static void WriteBoxHeader ( XMP_IO* fileRef, XMP_Uns32 boxType, XMP_Uns64 boxSize )
{
	XMP_Uns32 u32;
	XMP_Uns64 u64;
	XMP_Enforce ( boxSize >= 8 );	// The size must be the full size, not just the content.

	if ( boxSize <= 0xFFFFFFFF ) {

		u32 = MakeUns32BE ( (XMP_Uns32)boxSize );
		fileRef->Write ( &u32, 4 );
		u32 = MakeUns32BE ( boxType );
		fileRef->Write ( &u32, 4 );

	} else {

		u32 = MakeUns32BE ( 1 );
		fileRef->Write ( &u32, 4 );
		u32 = MakeUns32BE ( boxType );
		fileRef->Write ( &u32, 4 );
		u64 = MakeUns64BE ( boxSize );
		fileRef->Write ( &u64, 8 );

	}

}	// WriteBoxHeader

// =================================================================================================
// WipeBoxFree
// ===========
//
// Change the box's type to 'free' (or create a 'free' box) and zero the content.

static XMP_Uns8 kZeroes [64*1024];	// C semantics guarantee zero initialization.

static void WipeBoxFree ( XMP_IO* fileRef, XMP_Uns64 boxOffset, XMP_Uns32 boxSize )
{
	if ( boxSize == 0 ) return;
	XMP_Enforce ( boxSize >= 8 );

	fileRef->Seek ( boxOffset, kXMP_SeekFromStart );
	XMP_Uns32 u32;
	u32 = MakeUns32BE ( boxSize );	// ! The actual size should not change, but might have had a long header.
	fileRef->Write ( &u32, 4 );
	u32 = MakeUns32BE ( ISOMedia::k_free );
	fileRef->Write ( &u32, 4 );

	XMP_Uns32 ioCount = sizeof ( kZeroes );
	for ( boxSize -= 8; boxSize > 0; boxSize -= ioCount ) {
		if ( ioCount > boxSize ) ioCount = boxSize;
		fileRef->Write ( &kZeroes[0], ioCount );
	}

}	// WipeBoxFree

// =================================================================================================
// CreateFreeSpaceList
// ===================

struct SpaceInfo {
	XMP_Uns64 offset, size;
	SpaceInfo() : offset(0), size(0) {};
	SpaceInfo ( XMP_Uns64 _offset, XMP_Uns64 _size ) : offset(_offset), size(_size) {};
};

typedef std::vector<SpaceInfo> FreeSpaceList;

static void CreateFreeSpaceList ( XMP_IO* fileRef, XMP_Uns64 fileSize,
								  XMP_Uns64 oldOffset, XMP_Uns32 oldSize, FreeSpaceList * spaceList )
{
	XMP_Uns64 boxPos, boxNext, adjacentFree;
	ISOMedia::BoxInfo currBox;

	fileRef->Rewind();
	spaceList->clear();

	for ( boxPos = 0; boxPos < fileSize; boxPos = boxNext ) {

		boxNext = ISOMedia::GetBoxInfo ( fileRef, boxPos, fileSize, &currBox, true /* throw errors */ );
		XMP_Uns64 currSize = currBox.headerSize + currBox.contentSize;

		if ( (currBox.boxType == ISOMedia::k_free) ||
			 (currBox.boxType == ISOMedia::k_skip) ||
			 ((boxPos == oldOffset) && (currSize == oldSize)) ) {

			if ( spaceList->empty() || (boxPos != adjacentFree) ) {
				spaceList->push_back ( SpaceInfo ( boxPos, currSize ) );
				adjacentFree = boxPos + currSize;
			} else {
				SpaceInfo * lastSpace = &spaceList->back();
				lastSpace->size += currSize;
			}

		}

	}

}	// CreateFreeSpaceList

// =================================================================================================
// MPEG4_MetaHandler::CacheFileData
// ================================
//
// There are 3 file variants: normal ISO Base Media, modern QuickTime, and classic QuickTime. The
// XMP is placed differently between the ISO and two QuickTime forms, and there is different but not
// colliding native metadata. The entire 'moov' subtree is cached, along with the top level 'uuid'
// box of XMP if present.

void MPEG4_MetaHandler::CacheFileData()
{
	XMP_Assert ( ! this->containsXMP );

	XMPFiles * parent = this->parent;
	XMP_OptionBits openFlags = parent->openFlags;

	XMP_IO* fileRef  = parent->ioRef;

	XMP_AbortProc abortProc  = parent->abortProc;
	void *        abortArg   = parent->abortArg;
	const bool    checkAbort = (abortProc != 0);

	// First do some special case repair to QuickTime files, based on bad files in the wild.

	const bool isUpdate = XMP_OptionIsSet ( openFlags, kXMPFiles_OpenForUpdate );
	const bool doRepair = XMP_OptionIsSet ( openFlags, kXMPFiles_OpenRepairFile );

	if ( isUpdate && (parent->format == kXMP_MOVFile) ) {
		CheckQTFileStructure ( this, doRepair );	// Will throw for failure.
	}

	// Cache the top level 'moov' and 'uuid'/XMP boxes.

	XMP_Uns64 fileSize = fileRef->Length();

	XMP_Uns64 boxPos, boxNext;
	ISOMedia::BoxInfo currBox;

	bool xmpOnly = XMP_OptionIsSet ( openFlags, kXMPFiles_OpenOnlyXMP );
	bool haveISOFile = (this->fileMode == MOOV_Manager::kFileIsNormalISO);

	bool uuidFound = (! haveISOFile);			// Ignore the XMP 'uuid' box for QuickTime files.
	bool moovIgnored = (xmpOnly & haveISOFile);	// Ignore the 'moov' box for XMP-only ISO files.
	bool moovFound = moovIgnored;

	for ( boxPos = 0; boxPos < fileSize; boxPos = boxNext ) {

		if ( checkAbort && abortProc(abortArg) ) {
			XMP_Throw ( "MPEG4_MetaHandler::CacheFileData - User abort", kXMPErr_UserAbort );
		}


		boxNext = ISOMedia::GetBoxInfo ( fileRef, boxPos, fileSize, &currBox );

		if ( (! moovFound) && (currBox.boxType == ISOMedia::k_moov) ) {

			XMP_Uns64 fullMoovSize = currBox.headerSize + currBox.contentSize;
			if ( fullMoovSize > moovBoxSizeLimit ) {	// From here on we know 32-bit offsets are safe.
				XMP_Throw ( "Oversize 'moov' box", kXMPErr_EnforceFailure );
			}

			this->moovMgr.fullSubtree.assign ( (XMP_Uns32)fullMoovSize, 0 );
			fileRef->Seek ( boxPos, kXMP_SeekFromStart );
			fileRef->Read ( &this->moovMgr.fullSubtree[0], (XMP_Uns32)fullMoovSize );

			this->moovBoxPos = boxPos;
			this->moovBoxSize = (XMP_Uns32)fullMoovSize;
			moovFound = true;
			if ( uuidFound ) break;	// Exit the loop when both are found.

		} else if ( (! uuidFound) && (currBox.boxType == ISOMedia::k_uuid) ) {

			if ( currBox.contentSize < 16 ) continue;

			XMP_Uns8 uuid [16];
			fileRef->ReadAll ( uuid, 16 );
			if ( memcmp ( uuid, ISOMedia::k_xmpUUID, 16 ) != 0 ) continue;	// Check for the XMP GUID.

			XMP_Uns64 fullUuidSize = currBox.headerSize + currBox.contentSize;
			if ( fullUuidSize > moovBoxSizeLimit ) {	// From here on we know 32-bit offsets are safe.
				XMP_Throw ( "Oversize XMP 'uuid' box", kXMPErr_EnforceFailure );
			}

			this->packetInfo.offset = boxPos + currBox.headerSize + 16;	// The 16 is for the UUID.
			this->packetInfo.length = (XMP_Int32) (currBox.contentSize - 16);

			this->xmpPacket.assign ( this->packetInfo.length, ' ' );
			fileRef->ReadAll ( (void*)this->xmpPacket.data(), this->packetInfo.length );

			this->xmpBoxPos = boxPos;
			this->xmpBoxSize = (XMP_Uns32)fullUuidSize;
			uuidFound = true;
			if ( moovFound ) break;	// Exit the loop when both are found.

		}

	}

	if ( (! moovFound) && (! moovIgnored) ) XMP_Throw ( "No 'moov' box", kXMPErr_BadFileFormat );

}	// MPEG4_MetaHandler::CacheFileData

// =================================================================================================
// MPEG4_MetaHandler::ProcessXMP
// =============================

void MPEG4_MetaHandler::ProcessXMP()
{
	if ( this->processedXMP ) return;
	this->processedXMP = true;	// Make sure only called once.

	XMPFiles * parent = this->parent;
	XMP_OptionBits openFlags = parent->openFlags;

	bool xmpOnly = XMP_OptionIsSet ( openFlags, kXMPFiles_OpenOnlyXMP );
	bool haveISOFile = (this->fileMode == MOOV_Manager::kFileIsNormalISO);

	// Process the cached XMP (from the 'uuid' box) if that is all we want and this is an ISO file.

	if ( xmpOnly & haveISOFile ) {

		this->containsXMP = this->havePreferredXMP = (this->packetInfo.length != 0);

		if ( this->containsXMP ) {
			FillPacketInfo ( this->xmpPacket, &this->packetInfo );
			this->xmpObj.ParseFromBuffer ( this->xmpPacket.c_str(), (XMP_StringLen)this->xmpPacket.size() );
			this->xmpObj.DeleteProperty ( kXMP_NS_XMP, "NativeDigests" );	// No longer used.
		}

		return;

	}

	// Parse the cached 'moov' subtree, parse the preferred XMP.

	if ( this->moovMgr.fullSubtree.empty() ) XMP_Throw ( "No 'moov' box", kXMPErr_BadFileFormat );
	this->moovMgr.ParseMemoryTree ( this->fileMode );

	if ( (this->xmpBoxPos == 0) || (! haveISOFile) ) {

		// Look for the QuickTime moov/uuid/XMP_ box.

		MOOV_Manager::BoxInfo xmpInfo;
		MOOV_Manager::BoxRef  xmpRef = this->moovMgr.GetBox ( "moov/udta/XMP_", &xmpInfo );

		if ( (xmpRef != 0) && (xmpInfo.contentSize != 0) ) {

			this->xmpBoxPos = this->moovBoxPos + this->moovMgr.GetParsedOffset ( xmpRef );
			this->packetInfo.offset = this->xmpBoxPos + this->moovMgr.GetHeaderSize ( xmpRef );
			this->packetInfo.length = xmpInfo.contentSize;

			this->xmpPacket.assign ( (char*)xmpInfo.content, this->packetInfo.length );
			this->havePreferredXMP = (! haveISOFile);

		}

	}

	if ( this->xmpBoxPos != 0 ) {
		this->containsXMP = true;
		FillPacketInfo ( this->xmpPacket, &this->packetInfo );
		this->xmpObj.ParseFromBuffer ( this->xmpPacket.c_str(), (XMP_StringLen)this->xmpPacket.size() );
		this->xmpObj.DeleteProperty ( kXMP_NS_XMP, "NativeDigests" );	// No longer used.
	}

	// Import the non-XMP items. Do the imports in reverse priority order, last import wins!

	MOOV_Manager::BoxInfo mvhdInfo;
	MOOV_Manager::BoxRef  mvhdRef = this->moovMgr.GetBox ( "moov/mvhd", &mvhdInfo );
	bool mvhdFound = ((mvhdRef != 0) && (mvhdInfo.contentSize != 0));

	MOOV_Manager::BoxInfo udtaInfo;
	MOOV_Manager::BoxRef  udtaRef = this->moovMgr.GetBox ( "moov/udta", &udtaInfo );
	std::vector<MOOV_Manager::BoxInfo> cprtBoxes;

	if ( udtaRef != 0 ) {
		for ( XMP_Uns32 i = 0; i < udtaInfo.childCount; ++i ) {
			MOOV_Manager::BoxInfo currInfo;
			MOOV_Manager::BoxRef  currRef = this->moovMgr.GetNthChild ( udtaRef, i, &currInfo );
			if ( currRef == 0 ) break;	// Sanity check, should not happen.
			if ( currInfo.boxType != ISOMedia::k_cprt ) continue;
			cprtBoxes.push_back ( currInfo );
		}
	}
	bool cprtFound = (! cprtBoxes.empty());

	bool tradQTFound = this->tradQTMgr.ParseCachedBoxes ( this->moovMgr );
	bool tmcdFound = this->ParseTimecodeTrack();

	if ( this->fileMode == MOOV_Manager::kFileIsNormalISO ) {

		if ( mvhdFound )   this->containsXMP |= ImportMVHDItems ( mvhdInfo, &this->xmpObj );
		if ( cprtFound )   this->containsXMP |= ImportISOCopyrights ( cprtBoxes, &this->xmpObj );
	} else {	// This is a QuickTime file, either traditional or modern.

		if ( mvhdFound )   this->containsXMP |= ImportMVHDItems ( mvhdInfo, &this->xmpObj );
		if ( cprtFound )   this->containsXMP |= ImportISOCopyrights ( cprtBoxes, &this->xmpObj );
		if ( tmcdFound | tradQTFound ) {
			// Some of the timecode items are in the .../udta/©... set but handled by ImportTimecodeItems.
			this->containsXMP |= ImportTimecodeItems ( this->tmcdInfo, this->tradQTMgr, &this->xmpObj );
		}

		this->containsXMP |= ImportCr8rItems ( this->moovMgr, &this->xmpObj );

	}

}	// MPEG4_MetaHandler::ProcessXMP

// =================================================================================================
// MPEG4_MetaHandler::ParseTimecodeTrack
// =====================================

bool MPEG4_MetaHandler::ParseTimecodeTrack()
{
	MOOV_Manager::BoxInfo drefInfo;
	MOOV_Manager::BoxRef drefRef = FindTimecode_dref ( this->moovMgr );
	bool qtTimecodeIsExternal=false;
	if( drefRef != 0 )
	{
		this->moovMgr.GetBoxInfo( drefRef , &drefInfo );
		// After dref atom in a QT file we should only 
		// proceed further to check the Data refernces 
		// if the total size of the content is greater
		// than 8 bytes which suggests that there is atleast
		// one data reference to check for external references.
		if ( drefInfo.contentSize>8)
		{
			XMP_Uns32 noOfDrefs=GetUns32BE(drefInfo.content+4);
			if(noOfDrefs>0)
			{
				const XMP_Uns8* dataReference = drefInfo.content + 8;
				const XMP_Uns8* nextDataref   = 0;
				const XMP_Uns8* boxlimit      = drefInfo.content + drefInfo.contentSize;
				ISOMedia::BoxInfo dataRefernceInfo;
				while(noOfDrefs--)
				{
					nextDataref= ISOMedia::GetBoxInfo( dataReference , boxlimit, 
						&dataRefernceInfo);
					//The content atleast contains the flag and some data
					if ( dataRefernceInfo.contentSize > 4 )
					{
						if (dataRefernceInfo.boxType==ISOMedia::k_alis &&
							*((XMP_Uns8*)(dataReference + dataRefernceInfo.headerSize + 4)) !=1 )
						{
							qtTimecodeIsExternal=true;
							break;
						}
					}
					dataReference=nextDataref;
				}
			}
		}
	}

	MOOV_Manager::BoxRef stblRef = FindTimecode_stbl ( this->moovMgr );
	if ( stblRef == 0 ) return false;

	// Find the .../stbl/stsd box and process the first table entry.

	MOOV_Manager::BoxInfo stsdInfo;
	MOOV_Manager::BoxRef  stsdRef;

	stsdRef = this->moovMgr.GetTypeChild ( stblRef, ISOMedia::k_stsd, &stsdInfo );
	if ( stsdRef == 0 ) return false;
	if ( stsdInfo.contentSize < (8 + sizeof ( MOOV_Manager::Content_stsd_entry )) ) return false;
	if ( GetUns32BE ( stsdInfo.content + 4 ) == 0 ) return false;	// Make sure the entry count is non-zero.

	const MOOV_Manager::Content_stsd_entry * stsdRawEntry = (MOOV_Manager::Content_stsd_entry*) (stsdInfo.content + 8);

	XMP_Uns32 stsdEntrySize = GetUns32BE ( &stsdRawEntry->entrySize );
	if ( stsdEntrySize > (stsdInfo.contentSize - 4) ) stsdEntrySize = stsdInfo.contentSize - 4;
	if ( stsdEntrySize < sizeof ( MOOV_Manager::Content_stsd_entry ) ) return false;

	XMP_Uns32 stsdEntryFormat = GetUns32BE ( &stsdRawEntry->format );
	if ( stsdEntryFormat != ISOMedia::k_tmcd ) return false;

	this->tmcdInfo.timeScale = GetUns32BE ( &stsdRawEntry->timeScale );
	this->tmcdInfo.frameDuration = GetUns32BE ( &stsdRawEntry->frameDuration );

	double floatCount = (double)this->tmcdInfo.timeScale / (double)this->tmcdInfo.frameDuration;
	XMP_Uns8 expectedCount = (XMP_Uns8) (floatCount + 0.5);
	if ( expectedCount != stsdRawEntry->frameCount ) {
		double countRatio = (double)stsdRawEntry->frameCount / (double)expectedCount;
		this->tmcdInfo.timeScale = (XMP_Uns32) (((double)this->tmcdInfo.timeScale * countRatio) + 0.5);
	}

	XMP_Uns32 flags = GetUns32BE ( &stsdRawEntry->flags );
	this->tmcdInfo.isDropFrame = flags & 0x1;

	// Look for a trailing 'name' box on the first stsd table entry.

	XMP_Uns32 stsdTrailerSize = stsdEntrySize - sizeof ( MOOV_Manager::Content_stsd_entry );
	if ( stsdTrailerSize > 8 ) {	// Room for a non-empty 'name' box?

		const XMP_Uns8 * trailerStart = stsdInfo.content + 8 + sizeof ( MOOV_Manager::Content_stsd_entry );
		const XMP_Uns8 * trailerLimit = trailerStart + stsdTrailerSize;
		const XMP_Uns8 * trailerPos;
		const XMP_Uns8 * trailerNext;
		ISOMedia::BoxInfo trailerInfo;

		for ( trailerPos = trailerStart; trailerPos < trailerLimit; trailerPos = trailerNext ) {

			trailerNext = ISOMedia::GetBoxInfo ( trailerPos, trailerLimit, &trailerInfo );

			if ( trailerInfo.boxType == ISOMedia::k_name ) {

				this->tmcdInfo.nameOffset = (XMP_Uns32) (trailerPos - stsdInfo.content);

				if ( trailerInfo.contentSize > 4 ) {

					XMP_Uns16 textLen = GetUns16BE ( trailerPos + trailerInfo.headerSize );
					this->tmcdInfo.macLang = GetUns16BE ( trailerPos + trailerInfo.headerSize + 2 );

					if ( trailerInfo.contentSize >= (XMP_Uns64)(textLen + 4) ) {
						const char * textPtr = (char*) (trailerPos + trailerInfo.headerSize + 4);
						this->tmcdInfo.macName.assign ( textPtr, textLen );
					}

				}

				break;	// Done after finding the first 'name' box.

			}

		}

	}

	// Find the timecode sample.
	// Read the timecode only if we are sure that it is not External
	// This way we never find stsdBox and ExportTimecodeItems and
	// ImportTimecodeItems doesn't do anything with timeCodeSample
	// Also because sampleOffset is/remains zero UpdateFile doesn't
	// update the timeCodeSample value
	if(!qtTimecodeIsExternal)
	{
		XMP_Uns64 sampleOffset = 0;
		MOOV_Manager::BoxInfo tempInfo;
		MOOV_Manager::BoxRef  tempRef;

		tempRef = this->moovMgr.GetTypeChild ( stblRef, ISOMedia::k_stsc, &tempInfo );
		if ( tempRef == 0 ) return false;
		if ( tempInfo.contentSize < (8 + sizeof ( MOOV_Manager::Content_stsc_entry )) ) return false;
		if ( GetUns32BE ( tempInfo.content + 4 ) == 0 ) return false;	// Make sure the entry count is non-zero.

		XMP_Uns32 firstChunkNumber = GetUns32BE ( tempInfo.content + 8 );	// Want first field of first entry.

		tempRef = this->moovMgr.GetTypeChild ( stblRef, ISOMedia::k_stco, &tempInfo );

		if ( tempRef != 0 ) {

			if ( tempInfo.contentSize < (8 + 4) ) return false;
			XMP_Uns32 stcoCount = GetUns32BE ( tempInfo.content + 4 );
			if ( stcoCount < firstChunkNumber ) return false;
			XMP_Uns32 * stcoPtr = (XMP_Uns32*) (tempInfo.content + 8);
			sampleOffset = GetUns32BE ( &stcoPtr[firstChunkNumber-1] );	// ! Chunk number is 1-based.

		} else {

			tempRef = this->moovMgr.GetTypeChild ( stblRef, ISOMedia::k_co64, &tempInfo );
			if ( (tempRef == 0) || (tempInfo.contentSize < (8 + 8)) ) return false;
			XMP_Uns32 co64Count = GetUns32BE ( tempInfo.content + 4 );
			if ( co64Count < firstChunkNumber ) return false;
			XMP_Uns64 * co64Ptr = (XMP_Uns64*) (tempInfo.content + 8);
			sampleOffset = GetUns64BE ( &co64Ptr[firstChunkNumber-1] );	// ! Chunk number is 1-based.

		}

		if ( sampleOffset != 0 ) {	// Read the timecode sample.

			XMPFiles_IO* localFile = 0;

			if ( this->parent->ioRef == 0 ) {	// Local read-only files get closed in CacheFileData.
				XMP_Assert ( this->parent->UsesLocalIO() );
				localFile = XMPFiles_IO::New_XMPFiles_IO ( this->parent->GetFilePath().c_str(), Host_IO::openReadOnly, &this->parent->errorCallback);
				XMP_Enforce ( localFile != 0 );
				this->parent->ioRef = localFile;
			}

			this->parent->ioRef->Seek ( sampleOffset, kXMP_SeekFromStart  );
			this->parent->ioRef->ReadAll ( &this->tmcdInfo.timecodeSample, 4 );
			this->tmcdInfo.timecodeSample = MakeUns32BE ( this->tmcdInfo.timecodeSample );
			if ( localFile != 0 ) {
				localFile->Close();
				delete localFile;
				this->parent->ioRef = 0;
			}

		}
	
		// If this is a QT file, look for an edit list offset to add to the timecode sample. Look in the
		// timecode track for an edts/elst box. The content is a UInt8 version, UInt8[3] flags, a UInt32
		// entry count, and a sequence of UInt32 triples (trackDuration, mediaTime, mediaRate). Take
		// mediaTime from the first entry, divide it by tmcdInfo.frameDuration, add that to
		// tmcdInfo.timecodeSample.

		bool isQT = (this->fileMode == MOOV_Manager::kFileIsModernQT) ||
					(this->fileMode == MOOV_Manager::kFileIsTraditionalQT);

		MOOV_Manager::BoxRef elstRef = 0;
		if ( isQT ) elstRef = FindTimecode_elst ( this->moovMgr );
		if ( elstRef != 0 ) {

			MOOV_Manager::BoxInfo elstInfo;
			this->moovMgr.GetBoxInfo ( elstRef, &elstInfo );
		
			if ( elstInfo.contentSize >= (4+4+12) ) {
				XMP_Uns32 elstCount = GetUns32BE ( elstInfo.content + 4 );
				if ( elstCount >= 1 ) {
					XMP_Uns32 mediaTime = GetUns32BE ( elstInfo.content + (4+4+4) );
					this->tmcdInfo.timecodeSample += (mediaTime / this->tmcdInfo.frameDuration);
				}
			}
		
		}

		// Finally update this->tmcdInfo to remember (for update) that there is an OK timecode track.

		this->tmcdInfo.stsdBoxFound = true;
		this->tmcdInfo.sampleOffset = sampleOffset;
	}
	return true;

}	// MPEG4_MetaHandler::ParseTimecodeTrack

// =================================================================================================
// MPEG4_MetaHandler::UpdateTopLevelBox
// ====================================

void MPEG4_MetaHandler::UpdateTopLevelBox ( XMP_Uns64 oldOffset, XMP_Uns32 oldSize,
											const XMP_Uns8 * newBox, XMP_Uns32 newSize )
{
	if ( (oldSize == 0) && (newSize == 0) ) return;	// Sanity check, should not happen.

	XMP_IO* fileRef = this->parent->ioRef;
	XMP_Uns64 oldFileSize = fileRef->Length();

	XMP_AbortProc abortProc = this->parent->abortProc;
	void *        abortArg  = this->parent->abortArg;

	if ( newSize == oldSize ) {

		// Trivial case, update the existing box in-place.
		fileRef->Seek ( oldOffset, kXMP_SeekFromStart );
		fileRef->Write ( newBox, oldSize );

	} else if ( (oldOffset + oldSize) == oldFileSize ) {

		// The old box was at the end, write the new and truncate the file if necessary.
		fileRef->Seek ( oldOffset, kXMP_SeekFromStart );
		fileRef->Write ( newBox, newSize );
		fileRef->Truncate ( (oldOffset + newSize) );	// Does nothing if new size is bigger.

	} else if ( (newSize < oldSize) && ((oldSize - newSize) >= 8) ) {

		// The new size is smaller and there is enough room to create a free box.
		fileRef->Seek ( oldOffset, kXMP_SeekFromStart );
		fileRef->Write ( newBox, newSize );
		WipeBoxFree ( fileRef, (oldOffset + newSize), (oldSize - newSize) );

	} else {

		// Look for a trailing free box with enough space. If not found, consider any free space.
		// If still not found, append the new box and make the old one free.

		ISOMedia::BoxInfo nextBoxInfo;
		(void) ISOMedia::GetBoxInfo ( fileRef, (oldOffset + oldSize), oldFileSize, &nextBoxInfo, true /* throw errors */ );

		XMP_Uns64 totalRoom = oldSize + nextBoxInfo.headerSize + nextBoxInfo.contentSize;

		bool nextIsFree = (nextBoxInfo.boxType == ISOMedia::k_free) || (nextBoxInfo.boxType == ISOMedia::k_skip);
		bool haveEnoughRoom = (newSize == totalRoom) ||
							  ( (newSize < totalRoom) && ((totalRoom - newSize) >= 8) );

		if ( nextIsFree & haveEnoughRoom ) {

			fileRef->Seek ( oldOffset, kXMP_SeekFromStart );
			fileRef->Write ( newBox, newSize );

			if ( newSize < totalRoom ) {
				// Don't wipe, at most 7 old bytes left, it will be covered by the free header.
				WriteBoxHeader ( fileRef, ISOMedia::k_free, (totalRoom - newSize) );
			}

		} else {

			// Create a list of all top level free space, including the old space as free. Use the
			// earliest space that fits. If none, append.

			FreeSpaceList spaceList;
			CreateFreeSpaceList ( fileRef, oldFileSize, oldOffset, oldSize, &spaceList );

			size_t freeSlot, limit;
			for ( freeSlot = 0, limit = spaceList.size(); freeSlot < limit; ++freeSlot ) {
				XMP_Uns64 freeSize = spaceList[freeSlot].size;
				if ( (newSize == freeSize) || ( (newSize < freeSize) && ((freeSize - newSize) >= 8) ) ) break;
			}

			if ( freeSlot == spaceList.size() ) {

				// No available free space, append the new box.
				CheckFinalBox ( fileRef, abortProc, abortArg );
				fileRef->ToEOF();
				fileRef->Write ( newBox, newSize );
				WipeBoxFree ( fileRef, oldOffset, oldSize );

			} else {

				// Use the available free space. Wipe non-overlapping parts of the old box. The old
				// box is either included in the new space, or is fully disjoint.

				SpaceInfo & newSpace = spaceList[freeSlot];

				bool oldIsDisjoint = ((oldOffset + oldSize) <= newSpace.offset) ||		// Old is in front.
									 ((newSpace.offset + newSpace.size) <= oldOffset);	// Old is behind.

				XMP_Assert ( (newSize == newSpace.size) ||
							 ( (newSize < newSpace.size) && ((newSpace.size - newSize) >= 8) ) );

				XMP_Assert ( oldIsDisjoint ||
							 ( (newSpace.offset <= oldOffset) &&
							   ((oldOffset + oldSize) <= (newSpace.offset + newSpace.size)) ) /* old is included */ );

				XMP_Uns64 newFreeOffset = newSpace.offset + newSize;
				XMP_Uns64 newFreeSize   = newSpace.size - newSize;

				fileRef->Seek ( newSpace.offset, kXMP_SeekFromStart );
				fileRef->Write ( newBox, newSize );

				if ( newFreeSize > 0 ) WriteBoxHeader ( fileRef, ISOMedia::k_free, newFreeSize );

				if ( oldIsDisjoint ) {

					WipeBoxFree ( fileRef, oldOffset, oldSize );

				} else {

					// Clear the exposed portion of the old box.

					XMP_Uns64 zeroStart = newFreeOffset + 8;
					if ( newFreeSize > 0xFFFFFFFF ) zeroStart += 8;
					if ( oldOffset > zeroStart ) zeroStart = oldOffset;
					XMP_Uns64 zeroEnd = newFreeOffset + newFreeSize;
					if ( (oldOffset + oldSize) < zeroEnd ) zeroEnd = oldOffset + oldSize;

					if ( zeroStart < zeroEnd ) {	// The new box might cover the old.
						XMP_Assert ( (zeroEnd - zeroStart) <= (XMP_Uns64)oldSize );
						XMP_Uns32 zeroSize = (XMP_Uns32) (zeroEnd - zeroStart);
						fileRef->Seek ( zeroStart, kXMP_SeekFromStart );
						for ( XMP_Uns32 ioCount = sizeof ( kZeroes ); zeroSize > 0; zeroSize -= ioCount ) {
							if ( ioCount > zeroSize ) ioCount = zeroSize;
							fileRef->Write ( &kZeroes[0], ioCount );
						}
					}

				}

			}

		}

	}

}	// MPEG4_MetaHandler::UpdateTopLevelBox

// =================================================================================================
// MPEG4_MetaHandler::UpdateFile
// =============================
//
// Revamp notes:
// The 'moov' subtree and possibly the XMP 'uuid' box get updated. Compose the new copy of each and
// see if it fits in existing space, incorporating adjacent 'free' boxes if necessary. If that won't
// work, look for a sufficient 'free' box anywhere in the file. As a last resort, append the new copy.
// Assume no location sensitive data within 'moov', i.e. no offsets into it. This lets it be moved
// and its children freely rearranged.

void MPEG4_MetaHandler::UpdateFile ( bool doSafeUpdate )
{
	if ( ! this->needsUpdate ) {	// If needsUpdate is set then at least the XMP changed.
		return;
	}

	this->needsUpdate = false;	// Make sure only called once.
	XMP_Assert ( ! doSafeUpdate );	// This should only be called for "unsafe" updates.

	XMP_AbortProc abortProc  = this->parent->abortProc;
	void *        abortArg   = this->parent->abortArg;
	const bool    checkAbort = (abortProc != 0);

	XMP_IO* fileRef  = this->parent->ioRef;
	XMP_Uns64   fileSize = fileRef->Length();

	bool haveISOFile = (this->fileMode == MOOV_Manager::kFileIsNormalISO);

	// Update the 'moov' subtree with exports from the XMP, but not the XMP itself (for QT files).

	ExportMVHDItems ( this->xmpObj, &this->moovMgr );
	ExportISOCopyrights ( this->xmpObj, &this->moovMgr );
	ExportQuickTimeItems ( this->xmpObj, &this->tradQTMgr, &this->moovMgr );
	ExportTimecodeItems ( this->xmpObj, &this->tmcdInfo, &this->tradQTMgr, &this->moovMgr );

	if ( ! haveISOFile ) ExportCr8rItems ( this->xmpObj, &this->moovMgr );

	// Set up progress tracking if necessary. At this point just include the XMP size, we don't
	// know the 'moov' box size until later.

	bool localProgressTracking = false;
	XMP_ProgressTracker* progressTracker = this->parent->progressTracker;
	if ( progressTracker != 0 ) {
		float xmpSize = (float)this->xmpPacket.size();
		if ( progressTracker->WorkInProgress() ) {
			progressTracker->AddTotalWork ( xmpSize );
		} else {
			localProgressTracking = true;
			progressTracker->BeginWork ( xmpSize );
		}
	}

	// Try to update the XMP in-place if that is all that changed, or if it is in a preferred 'uuid' box.
	// The XMP has already been serialized by common code to the appropriate length. Otherwise, update
	// the 'moov'/'udta'/'XMP_' box in the MOOV_Manager, or the 'uuid' XMP box in the file.

	bool useUuidXMP = (this->fileMode == MOOV_Manager::kFileIsNormalISO);
	bool inPlaceXMP = (this->xmpPacket.size() == (size_t)this->packetInfo.length) &&
	                  ( (useUuidXMP & this->havePreferredXMP) || (! this->moovMgr.IsChanged()) );


	if ( inPlaceXMP ) {

		// Update the existing XMP in-place.
		fileRef->Seek ( this->packetInfo.offset, kXMP_SeekFromStart );
		fileRef->Write ( this->xmpPacket.c_str(), (XMP_Int32)this->xmpPacket.size() );

	} else if ( useUuidXMP ) {

		// Don't leave an old 'moov'/'udta'/'XMP_' box around.
		MOOV_Manager::BoxRef udtaRef = this->moovMgr.GetBox ( "moov/udta", 0 );
		if ( udtaRef != 0 ) this->moovMgr.DeleteTypeChild ( udtaRef, ISOMedia::k_XMP_ );

	} else {

		// Don't leave an old uuid XMP around (if we know about it).
		if ( (! havePreferredXMP) && (this->xmpBoxSize != 0) ) {
			WipeBoxFree ( fileRef, this->xmpBoxPos, this->xmpBoxSize );
		}

		// The udta form of XMP has just the XMP packet.
		this->moovMgr.SetBox ( "moov/udta/XMP_", this->xmpPacket.c_str(), (XMP_Uns32)this->xmpPacket.size() );

	}

	// Update the 'moov' subtree if necessary, and finally update the timecode sample.

	if ( this->moovMgr.IsChanged() ) {
		this->moovMgr.UpdateMemoryTree();
		if ( progressTracker != 0 ) {
			progressTracker->AddTotalWork ( (float)this->moovMgr.fullSubtree.size() );
		}
		this->UpdateTopLevelBox ( moovBoxPos, moovBoxSize, &this->moovMgr.fullSubtree[0],
								  (XMP_Uns32)this->moovMgr.fullSubtree.size() );
	}

	if ( this->tmcdInfo.sampleOffset != 0 ) {
		fileRef->Seek ( this->tmcdInfo.sampleOffset, kXMP_SeekFromStart );
		XMP_Uns32 sample = MakeUns32BE ( this->tmcdInfo.timecodeSample );
		fileRef->Write ( &sample, 4 );
	}

	// Update the 'uuid' XMP box if necessary.

	if ( useUuidXMP & (! inPlaceXMP) ) {

		// The uuid form of XMP has the 16-byte UUID in front of the XMP packet. Form the complete
		// box (including size/type header) for UpdateTopLevelBox.
		RawDataBlock uuidBox;
		XMP_Uns32 uuidSize = 4+4 + 16 + (XMP_Uns32)this->xmpPacket.size();
		uuidBox.assign ( uuidSize, 0 );
		PutUns32BE ( uuidSize, &uuidBox[0] );
		PutUns32BE ( ISOMedia::k_uuid, &uuidBox[4] );
		memcpy ( &uuidBox[8], ISOMedia::k_xmpUUID, 16 );
		memcpy ( &uuidBox[24], this->xmpPacket.c_str(), this->xmpPacket.size() );
		this->UpdateTopLevelBox ( this->xmpBoxPos, this->xmpBoxSize, &uuidBox[0], uuidSize );

	}

	if ( localProgressTracking ) progressTracker->WorkComplete();

}	// MPEG4_MetaHandler::UpdateFile

// =================================================================================================
// MPEG4_MetaHandler::WriteTempFile
// ================================
//
// Since the XMP and legacy is probably a miniscule part of the entire file, and since we can't
// change the offset of most of the boxes, just copy the entire original file to the temp file, then
// do an in-place update to the temp file.

void MPEG4_MetaHandler::WriteTempFile ( XMP_IO* tempRef )
{
	XMP_Assert ( this->needsUpdate );

	XMP_IO* originalRef = this->parent->ioRef;
	XMP_ProgressTracker* progressTracker = this->parent->progressTracker;

	tempRef->Rewind();
	originalRef->Rewind();
	if ( progressTracker != 0 ) progressTracker->BeginWork ( (float) originalRef->Length() );
	XIO::Copy ( originalRef, tempRef, originalRef->Length(),
			    this->parent->abortProc, this->parent->abortArg );

	try {
		this->parent->ioRef = tempRef;	// ! Fool UpdateFile into using the temp file.
		this->UpdateFile ( false );
		this->parent->ioRef = originalRef;
	} catch ( ... ) {
		this->parent->ioRef = originalRef;
		throw;
	}

	if ( progressTracker != 0 ) progressTracker->WorkComplete();

}	// MPEG4_MetaHandler::WriteTempFile

// =================================================================================================
