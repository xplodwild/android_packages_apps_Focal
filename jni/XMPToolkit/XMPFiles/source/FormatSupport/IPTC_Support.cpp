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

#include "XMPFiles/source/FormatSupport/IPTC_Support.hpp"
#include "source/EndianUtils.hpp"
#include "XMPFiles/source/FormatSupport/Reconcile_Impl.hpp"
#include "source/XIO.hpp"

// =================================================================================================
/// \file IPTC_Support.cpp
/// \brief XMPFiles support for IPTC (IIM) DataSets.
///
// =================================================================================================

enum { kUTF8_IncomingMode = 0, kUTF8_LosslessMode = 1, kUTF8_AlwaysMode = 2 };
#ifndef kUTF8_Mode
	#define kUTF8_Mode kUTF8_AlwaysMode
#endif

const DataSetCharacteristics kKnownDataSets[] =
	{ { kIPTC_ObjectType,        kIPTC_UnmappedText,   67, "",                "" },						// Not mapped to XMP.
	  { kIPTC_IntellectualGenre, kIPTC_MapSpecial,     68, kXMP_NS_IPTCCore,  "IntellectualGenre" },	// Only the name part is in the XMP.
	  { kIPTC_Title,             kIPTC_MapLangAlt,     64, kXMP_NS_DC,        "title" },
	  { kIPTC_EditStatus,        kIPTC_UnmappedText,   64, "",                "" },						// Not mapped to XMP.
	  { kIPTC_EditorialUpdate,   kIPTC_UnmappedText,    2, "",                "" },						// Not mapped to XMP.
	  { kIPTC_Urgency,           kIPTC_MapSimple,       1, kXMP_NS_Photoshop, "Urgency" },
	  { kIPTC_SubjectCode,       kIPTC_MapSpecial,    236, kXMP_NS_IPTCCore,  "SubjectCode" },			// Only the reference number is in the XMP.
	  { kIPTC_Category,          kIPTC_MapSimple,       3, kXMP_NS_Photoshop, "Category" },
	  { kIPTC_SuppCategory,      kIPTC_MapArray,       32, kXMP_NS_Photoshop, "SupplementalCategories" },
	  { kIPTC_FixtureIdentifier, kIPTC_UnmappedText,   32, "",                "" },						// Not mapped to XMP.
	  { kIPTC_Keyword,           kIPTC_MapArray,       64, kXMP_NS_DC,        "subject" },
	  { kIPTC_ContentLocCode,    kIPTC_UnmappedText,    3, "",                "" },						// Not mapped to XMP.
	  { kIPTC_ContentLocName,    kIPTC_UnmappedText,   64, "",                "" },						// Not mapped to XMP.
	  { kIPTC_ReleaseDate,       kIPTC_UnmappedText,    8, "",                "" },						// Not mapped to XMP.
	  { kIPTC_ReleaseTime,       kIPTC_UnmappedText,   11, "",                "" },						// Not mapped to XMP.
	  { kIPTC_ExpDate,           kIPTC_UnmappedText,    8, "",                "" },						// Not mapped to XMP.
	  { kIPTC_ExpTime,           kIPTC_UnmappedText,   11, "",                "" },						// Not mapped to XMP.
	  { kIPTC_Instructions,      kIPTC_MapSimple,     256, kXMP_NS_Photoshop, "Instructions" },
	  { kIPTC_ActionAdvised,     kIPTC_UnmappedText,    2, "",                "" },						// Not mapped to XMP.
	  { kIPTC_RefService,        kIPTC_UnmappedText,   10, "",                "" },						// Not mapped to XMP. ! Interleave 2:45, 2:47, 2:50!
	  { kIPTC_RefDate,           kIPTC_UnmappedText,    8, "",                "" },						// Not mapped to XMP. ! Interleave 2:45, 2:47, 2:50!
	  { kIPTC_RefNumber,         kIPTC_UnmappedText,    8, "",                "" },						// Not mapped to XMP. ! Interleave 2:45, 2:47, 2:50!
	  { kIPTC_DateCreated,       kIPTC_MapSpecial,      8, kXMP_NS_Photoshop, "DateCreated" },			// ! Reformatted date. Combined with 2:60, TimeCreated.
	  { kIPTC_TimeCreated,       kIPTC_UnmappedText,   11, "",                "" },						// ! Combined with 2:55, DateCreated.
	  { kIPTC_DigitalCreateDate, kIPTC_Map3Way,         8, "",                "" },						// ! 3 way Exif-IPTC-XMP date/time set. Combined with 2:63, DigitalCreateTime.
	  { kIPTC_DigitalCreateTime, kIPTC_UnmappedText,   11, "",                "" },						// ! Combined with 2:62, DigitalCreateDate.
	  { kIPTC_OriginProgram,     kIPTC_UnmappedText,   32, "",                "" },						// Not mapped to XMP.
	  { kIPTC_ProgramVersion,    kIPTC_UnmappedText,   10, "",                "" },						// Not mapped to XMP.
	  { kIPTC_ObjectCycle,       kIPTC_UnmappedText,    1, "",                "" },						// Not mapped to XMP.
	  { kIPTC_Creator,           kIPTC_Map3Way,        32, "",                "" },						// ! In the 3 way Exif-IPTC-XMP set.
	  { kIPTC_CreatorJobtitle,   kIPTC_MapSimple,      32, kXMP_NS_Photoshop, "AuthorsPosition" },
	  { kIPTC_City,              kIPTC_MapSimple,      32, kXMP_NS_Photoshop, "City" },
	  { kIPTC_Location,          kIPTC_MapSimple,      32, kXMP_NS_IPTCCore,  "Location" },
	  { kIPTC_State,             kIPTC_MapSimple,      32, kXMP_NS_Photoshop, "State" },
	  { kIPTC_CountryCode,       kIPTC_MapSimple,       3, kXMP_NS_IPTCCore,  "CountryCode" },
	  { kIPTC_Country,           kIPTC_MapSimple,      64, kXMP_NS_Photoshop, "Country" },
	  { kIPTC_JobID,             kIPTC_MapSimple,      32, kXMP_NS_Photoshop, "TransmissionReference" },
	  { kIPTC_Headline,          kIPTC_MapSimple,     256, kXMP_NS_Photoshop, "Headline" },
	  { kIPTC_Provider,          kIPTC_MapSimple,      32, kXMP_NS_Photoshop, "Credit" },
	  { kIPTC_Source,            kIPTC_MapSimple,      32, kXMP_NS_Photoshop, "Source" },
	  { kIPTC_CopyrightNotice,   kIPTC_Map3Way,       128, "",                "" },						// ! In the 3 way Exif-IPTC-XMP set.
	  { kIPTC_Contact,           kIPTC_UnmappedText,  128, "",                "" },						// Not mapped to XMP.
	  { kIPTC_Description,       kIPTC_Map3Way,      2000, "",                "" },						// ! In the 3 way Exif-IPTC-XMP set.
	  { kIPTC_DescriptionWriter, kIPTC_MapSimple,      32, kXMP_NS_Photoshop, "CaptionWriter" },
	  { kIPTC_RasterizedCaption, kIPTC_UnmappedBin,  7360, "",                "" },						// Not mapped to XMP. ! Binary data!
	  { kIPTC_ImageType,         kIPTC_UnmappedText,    2, "",                "" },						// Not mapped to XMP.
	  { kIPTC_ImageOrientation,  kIPTC_UnmappedText,    1, "",                "" },						// Not mapped to XMP.
	  { kIPTC_LanguageID,        kIPTC_UnmappedText,    3, "",                "" },						// Not mapped to XMP.
	  { kIPTC_AudioType,         kIPTC_UnmappedText,    2, "",                "" },						// Not mapped to XMP.
	  { kIPTC_AudioSampleRate,   kIPTC_UnmappedText,    6, "",                "" },						// Not mapped to XMP.
	  { kIPTC_AudioSampleRes,    kIPTC_UnmappedText,    2, "",                "" },						// Not mapped to XMP.
	  { kIPTC_AudioDuration,     kIPTC_UnmappedText,    6, "",                "" },						// Not mapped to XMP.
	  { kIPTC_AudioOutcue,       kIPTC_UnmappedText,   64, "",                "" },						// Not mapped to XMP.
	  { kIPTC_PreviewFormat,     kIPTC_UnmappedBin,     2, "",                "" },						// Not mapped to XMP. ! Binary data!
	  { kIPTC_PreviewFormatVers, kIPTC_UnmappedBin,     2, "",                "" },						// Not mapped to XMP. ! Binary data!
	  { kIPTC_PreviewData,       kIPTC_UnmappedBin, 256000, "",               "" },						// Not mapped to XMP. ! Binary data!
	  { 255,                     kIPTC_MapSpecial,      0, 0,                 0 } };					// ! Must be last as a sentinel.

// A combination of the IPTC "Subject Reference System Guidelines" and IIMv4.1 Appendix G.
const IntellectualGenreMapping kIntellectualGenreMappings[] =
{ { "001", "Current" },
  { "002", "Analysis" },
  { "003", "Archive material" },
  { "004", "Background" },
  { "005", "Feature" },
  { "006", "Forecast" },
  { "007", "History" },
  { "008", "Obituary" },
  { "009", "Opinion" },
  { "010", "Polls and surveys" },
  { "010", "Polls & Surveys" },
  { "011", "Profile" },
  { "012", "Results listings and statistics" },
  { "012", "Results Listings & Tables" },
  { "013", "Side bar and supporting information" },
  { "013", "Side bar & Supporting information" },
  { "014", "Summary" },
  { "015", "Transcript and verbatim" },
  { "015", "Transcript & Verbatim" },
  { "016", "Interview" },
  { "017", "From the scene" },
  { "017", "From the Scene" },
  { "018", "Retrospective" },
  { "019", "Synopsis" },
  { "019", "Statistics" },
  { "020", "Update" },
  { "021", "Wrapup" },
  { "021", "Wrap-up" },
  { "022", "Press release" },
  { "022", "Press Release" },
  { "023", "Quote" },
  { "024", "Press-digest" },
  { "025", "Review" },
  { "026", "Curtain raiser" },
  { "027", "Actuality" },
  { "028", "Question and answer" },
  { "029", "Music" },
  { "030", "Response to a question" },
  { "031", "Raw sound" },
  { "032", "Scener" },
  { "033", "Text only" },
  { "034", "Voicer" },
  { "035", "Fixture" },
  { 0,     0 } };	// ! Must be last as a sentinel. 
  
// =================================================================================================
// FindKnownDataSet
// ================

static const DataSetCharacteristics* FindKnownDataSet ( XMP_Uns8 dsNum )
{
	size_t i = 0;

	while ( kKnownDataSets[i].dsNum < dsNum ) ++i;	// The list is short enough for a linear search.
	
	if ( kKnownDataSets[i].dsNum != dsNum ) return 0;
	return &kKnownDataSets[i];	

}	// FindKnownDataSet

// =================================================================================================
// IPTC_Manager::ParseMemoryDataSets
// =================================
//
// Parse the IIM block. All datasets are put into the map, although we only really care about 1:90
// and the known 2:xx ones. This approach is tolerant of ill-formed IIM where the datasets are not
// sorted by ascending record number.

void IPTC_Manager::ParseMemoryDataSets ( const void* data, XMP_Uns32 length, bool copyData /* = true */ )
{
	// Get rid of any existing data.

	DataSetMap::iterator dsPos = this->dataSets.begin();
	DataSetMap::iterator dsEnd = this->dataSets.end();
	
	for ( ; dsPos != dsEnd; ++dsPos ) this->DisposeLooseValue ( dsPos->second );

	this->dataSets.clear();
	
	if ( this->ownedContent ) free ( this->iptcContent );
	this->ownedContent = false;	// Set to true later if the content is copied.
	this->iptcContent = 0;
	this->iptcLength = 0;

	this->changed = false;
	
	if ( length == 0 ) return;
	if ( (data == 0) || (*((XMP_Uns8*)data) != 0x1C) ) XMP_Throw ( "Not valid IPTC, no leading 0x1C", kXMPErr_BadIPTC );
	
	// Allocate space for the full in-memory data and copy it.
	
	if ( length > 10*1024*1024 ) XMP_Throw ( "Outrageous length for memory-based IPTC", kXMPErr_BadIPTC );
	this->iptcLength = length;
	
	if ( ! copyData ) {
		this->iptcContent = (XMP_Uns8*)data;
	} else {
		this->iptcContent = (XMP_Uns8*) malloc(length);
		if ( this->iptcContent == 0 ) XMP_Throw ( "Out of memory", kXMPErr_NoMemory );
		memcpy ( this->iptcContent, data, length );	// AUDIT: Safe, malloc'ed length bytes above.
		this->ownedContent = true;
	}
	
	// Build the map of the DataSets. The records should be in ascending order, but we tolerate out
	// of order IIM produced by some unknown apps. The DataSets in a record can be in any order.
	// There are no record markers, just DataSets, so the ordering is really just clumping of
	// DataSets by record. A normal DataSet has a 5 byte header followed by the value. An extended
	// DataSet has a special length in the header, a variable sized value length, and the value.
	//
	// Normal DataSet
	// 0	uint8	0x1C
	// 1	uint8	record number
	// 2	uint8	DataSet number
	// 3	uint16	big endian value size, 0..32767, larger means extended DataSet
	//
	// In an extended DataSet the extended length size is the low 15 bits of the standard size. The
	// extended length follows as a big endian unsigned number. The IPTC does not specify, but we
	// require the extended length size to be in the range 1..4. It should only be 3 or 4, we allow
	// the degenerate cases.
	
	XMP_Uns8* iptcPtr   = this->iptcContent;
	XMP_Uns8* iptcEnd   = iptcPtr + length;
	XMP_Uns8* iptcLimit = iptcEnd - kMinDataSetSize;
	XMP_Uns32 dsLen;	// ! The large form can have values up to 4GB in length.
	
	this->utf8Encoding = false;
	
	for ( ; iptcPtr <= iptcLimit; iptcPtr += dsLen ) {
	
		// iptcLimit - last possible DataSet, 5 bytes before IIM block end
	
		// iptcPtr	- working pointer to the current byte of interest
		// dsPtr	- pointer to the current DataSet's header
		// dsLen	- value length, does not include extended size bytes
	
		XMP_Uns8* dsPtr  = iptcPtr;
		XMP_Uns8  oneC   = *iptcPtr;
		XMP_Uns8  recNum = *(iptcPtr+1);
		XMP_Uns8  dsNum  = *(iptcPtr+2);

		if ( oneC != 0x1C ) break;	// No more DataSets.
		
		dsLen  = GetUns16BE ( iptcPtr+3 );	// ! Compute dsLen before any "continue", needed for loop increment!
		iptcPtr += 5;	// Advance to the data (or extended length).
		
		if ( (dsLen & 0x8000) != 0 ) {
			XMP_Assert ( dsLen <= 0xFFFF );
			XMP_Uns32 lenLen = dsLen & 0x7FFF;
			if ( (lenLen == 0) || (lenLen > 4) ) break;	// Bad DataSet, can't find the next so quit.
			if ( iptcPtr > (iptcEnd - lenLen) ) break;	// Bad final DataSet. Throw instead?
			dsLen = 0;
			for ( XMP_Uns16 i = 0; i < lenLen; ++i, ++iptcPtr ) {
				dsLen = (dsLen << 8) + *iptcPtr;
			}
		}
		
		if ( iptcPtr > (iptcEnd - dsLen) ) break;	// Bad final DataSet. Throw instead?

		// Make a special check for 1:90 denoting UTF-8 text.
		if ( (recNum == 1) && (dsNum == 90) ) {
			if ( (dsLen == 3) && (memcmp ( iptcPtr, "\x1B\x25\x47", 3 ) == 0) ) this->utf8Encoding = true;
		}

		XMP_Uns16 mapID = recNum*1000 + dsNum;	
		DataSetInfo dsInfo ( recNum, dsNum, dsLen, iptcPtr );
		DataSetMap::iterator dsPos = this->dataSets.find ( mapID );
		
		bool repeatable = false;
		
		const DataSetCharacteristics* knownDS = FindKnownDataSet ( dsNum );

		if ( (knownDS == 0) || (knownDS->mapForm == kIPTC_MapArray) ) {
			repeatable = true;	// Allow repeats for unknown DataSets.
		} else if ( (dsNum == kIPTC_Creator) || (dsNum == kIPTC_SubjectCode) ) {
			repeatable = true;
		}
		
		if ( repeatable || (dsPos == this->dataSets.end()) ) {
			DataSetMap::value_type mapValue ( mapID, dsInfo );
			(void) this->dataSets.insert ( this->dataSets.upper_bound ( mapID ), mapValue );
		} else {
			this->DisposeLooseValue ( dsPos->second );
			dsPos->second = dsInfo;	// Keep the last copy of illegal repeats.
		}
	
	}

}	// IPTC_Manager::ParseMemoryDataSets

// =================================================================================================
// IPTC_Manager::GetDataSet
// ========================

size_t IPTC_Manager::GetDataSet ( XMP_Uns8 dsNum, DataSetInfo* info, size_t which /* = 0 */ ) const
{

	XMP_Uns16 mapID = 2000 + dsNum;	// ! Only deal with 2:xx datasets.
	DataSetMap::const_iterator dsPos = this->dataSets.lower_bound ( mapID );
	if ( (dsPos == this->dataSets.end()) || (dsPos->second.recNum != 2) || (dsNum != dsPos->second.dsNum) ) return 0;
	
	size_t dsCount = this->dataSets.count ( mapID );
	if ( which >= dsCount ) return 0;	// Valid range for which is 0 .. count-1.
	
	if ( info != 0 ) {
		for ( size_t i = 0; i < which; ++i ) ++dsPos;	// Can't do "dsPos += which", no iter+int operator.
		*info = dsPos->second;
	}
	
	return dsCount;

}	// IPTC_Manager::GetDataSet

// =================================================================================================
// IPTC_Manager::GetDataSet_UTF8
// =============================
	
size_t IPTC_Manager::GetDataSet_UTF8 ( XMP_Uns8 dsNum, std::string * utf8Str, size_t which /* = 0 */ ) const
{
	if ( utf8Str != 0 ) utf8Str->erase();
	
	DataSetInfo dsInfo;
	size_t dsCount = GetDataSet ( dsNum, &dsInfo, which );
	if ( dsCount == 0 ) return 0;
	
	if ( utf8Str != 0 ) {
		if ( this->utf8Encoding ) {
			utf8Str->assign ( (char*)dsInfo.dataPtr, dsInfo.dataLen );
		} else if ( ! ignoreLocalText ) {
			ReconcileUtils::LocalToUTF8 ( dsInfo.dataPtr, dsInfo.dataLen, utf8Str );
		} else if ( ReconcileUtils::IsASCII ( dsInfo.dataPtr, dsInfo.dataLen ) ) {
			utf8Str->assign ( (char*)dsInfo.dataPtr, dsInfo.dataLen );
		}
	}
	
	return dsCount;
	
}	// IPTC_Manager::GetDataSet_UTF8

// =================================================================================================
// IPTC_Manager::DisposeLooseValue
// ===============================
//
// Dispose of loose values from SetDataSet calls after the last UpdateMemoryDataSets.

// ! Don't try to make the DataSetInfo struct be self-cleaning. It is a primary public type, returned
// ! from GetDataSet. Making it self-cleaning would get into nasty assignment and pointer ownership
// ! issues, far worse than doing this explicit cleanup.

void IPTC_Manager::DisposeLooseValue ( DataSetInfo & dsInfo )
{
	if ( dsInfo.dataLen == 0 ) return;
	
	XMP_Uns8* dataBegin = this->iptcContent;
	XMP_Uns8* dataEnd   = dataBegin + this->iptcLength;
	
	if ( ((XMP_Uns8*)dsInfo.dataPtr < dataBegin) || ((XMP_Uns8*)dsInfo.dataPtr >= dataEnd) ) {
		free ( (void*) dsInfo.dataPtr );
		dsInfo.dataPtr = 0;
	}
	
}	// IPTC_Manager::DisposeLooseValue

// =================================================================================================
// IPTC_Manager::AppendDataSet
//
// ! Calling instance must make sure that dsPtr is large enough to hold data from dsInfo !
// ===========================

XMP_Uns8* IPTC_Manager::AppendDataSet ( XMP_Uns8* dsPtr, const DataSetInfo & dsInfo ) {
	
	dsPtr[0] = 0x1C;
	dsPtr[1] = dsInfo.recNum;
	dsPtr[2] = dsInfo.dsNum;
	dsPtr += 3;
	
	XMP_Uns32 dsLen = dsInfo.dataLen;
	if ( dsLen <= 0x7FFF ) {
		PutUns16BE ( (XMP_Uns16)dsLen, dsPtr );
		dsPtr += 2;
	} else {
		PutUns16BE ( 0x8004, dsPtr );
		PutUns32BE ( dsLen, dsPtr+2 );
		dsPtr += 6;
	}
	// AUDIT: Calling instance must make sure that dsPtr is large enough.
	// Currently this function is only called from UpdateMemoryDataSets where the size of dsPtr 
	// is calculated including all data sets in the list, so the dsInfo data should always fit.
	memcpy ( dsPtr, dsInfo.dataPtr, dsLen );	
	dsPtr += dsLen;

	return dsPtr;
	
}	// IPTC_Manager::AppendDataSet

// =================================================================================================
// =================================================================================================

// =================================================================================================
// IPTC_Writer::~IPTC_Writer
// =========================
//
// Dispose of loose values from SetDataSet calls after the last UpdateMemoryDataSets.

IPTC_Writer::~IPTC_Writer()
{
	DataSetMap::iterator dsPos = this->dataSets.begin();
	DataSetMap::iterator dsEnd = this->dataSets.end();
	
	for ( ; dsPos != dsEnd; ++dsPos ) this->DisposeLooseValue ( dsPos->second );
	
}	// IPTC_Writer::~IPTC_Writer

// =================================================================================================
// IPTC_Writer::SetDataSet_UTF8
// ============================

void IPTC_Writer::SetDataSet_UTF8 ( XMP_Uns8 dsNum, const void* utf8Ptr, XMP_Uns32 utf8Len, long which /* = -1 */ )
{
	const DataSetCharacteristics* knownDS = FindKnownDataSet ( dsNum );
	if ( knownDS == 0 ) XMP_Throw ( "Can only set known IPTC DataSets", kXMPErr_InternalFailure );
	
	// Decide which character encoding to use and get a temporary pointer to the value.
	
	XMP_Uns8 *  tempPtr;
	XMP_Uns32   dataLen;
	std::string localStr;
	
	if ( kUTF8_Mode == kUTF8_AlwaysMode ) {

		// Always use UTF-8.
		if ( ! this->utf8Encoding ) this->ConvertToUTF8();
		tempPtr = (XMP_Uns8*) utf8Ptr;
		dataLen = utf8Len;
	
	} else if ( kUTF8_Mode == kUTF8_IncomingMode ) {

		// Only use UTF-8 if that was what the parsed block used.
		if ( this->utf8Encoding ) {
			tempPtr = (XMP_Uns8*) utf8Ptr;
			dataLen = utf8Len;
		} else if ( ! ignoreLocalText ) {
			ReconcileUtils::UTF8ToLocal ( utf8Ptr, utf8Len, &localStr );
			tempPtr = (XMP_Uns8*) localStr.data();
			dataLen = (XMP_Uns32) localStr.size();
		} else {
			if ( ! ReconcileUtils::IsASCII ( utf8Ptr, utf8Len ) ) return;
			tempPtr = (XMP_Uns8*) utf8Ptr;
			dataLen = utf8Len;
		}
	
	} else if ( kUTF8_Mode == kUTF8_LosslessMode ) {
	
		// Convert to UTF-8 if needed to prevent round trip loss.
		if ( this->utf8Encoding ) {
			tempPtr = (XMP_Uns8*) utf8Ptr;
			dataLen = utf8Len;
		} else if ( ! ignoreLocalText ) {
			std::string rtStr;
			ReconcileUtils::UTF8ToLocal ( utf8Ptr, utf8Len, &localStr );
			ReconcileUtils::LocalToUTF8 ( localStr.data(), localStr.size(), &rtStr );
			if ( (rtStr.size() == utf8Len) && (memcmp ( rtStr.data(), utf8Ptr, utf8Len ) == 0) ) {
				tempPtr = (XMP_Uns8*) localStr.data();	// No loss, keep local encoding.
				dataLen = (XMP_Uns32) localStr.size();
			} else {
				this->ConvertToUTF8();	// Had loss, change everything to UTF-8.
				XMP_Assert ( this->utf8Encoding );
				tempPtr = (XMP_Uns8*) utf8Ptr;
				dataLen = utf8Len;
			}
		} else {
			if ( ! ReconcileUtils::IsASCII ( utf8Ptr, utf8Len ) ) return;
			tempPtr = (XMP_Uns8*) utf8Ptr;
			dataLen = utf8Len;
		}
		
	}
	
	// Set the value for this DataSet, making a non-transient copy of the value. Respect UTF-8 character
	// boundaries when truncating. This is easy to check. If the first truncated byte has 10 in the
	// high order 2 bits then we are in the middle of a UTF-8 multi-byte character.
	// Back up to just before a byte with 11 in the high order 2 bits.

	if ( dataLen > knownDS->maxLen ) {
		dataLen = (XMP_Uns32)knownDS->maxLen;
		if ( this->utf8Encoding && ((tempPtr[dataLen] >> 6) == 2) ) {
			for ( ; (dataLen > 0) && ((tempPtr[dataLen] >> 6) != 3); --dataLen ) {}
		}
	}
	
	XMP_Uns16 mapID = 2000 + dsNum;	// ! Only deal with 2:xx datasets.
	DataSetMap::iterator dsPos = this->dataSets.find ( mapID );
	long currCount = (long) this->dataSets.count ( mapID );
	
	bool repeatable = false;
		
	if ( knownDS->mapForm == kIPTC_MapArray ) {
		repeatable = true;
	} else if ( (dsNum == kIPTC_Creator) || (dsNum == kIPTC_SubjectCode) ) {
		repeatable = true;
	}
	
	if ( ! repeatable ) {

		if ( which > 0 ) XMP_Throw ( "Non-repeatable IPTC DataSet", kXMPErr_BadParam );

	} else {
	
		if ( which < 0 ) which = currCount;	// The default is to append.

		if ( which > currCount ) {
			XMP_Throw ( "Invalid index for IPTC DataSet", kXMPErr_BadParam );
		} else if ( which == currCount ) {
			dsPos = this->dataSets.end();	// To make later checks do the right thing.
		} else {
			dsPos = this->dataSets.lower_bound ( mapID );
			for ( ; which > 0; --which ) ++dsPos;
		}
	
	}

	if ( dsPos != this->dataSets.end() ) {
		if ( (dsPos->second.dataLen == dataLen) && (memcmp ( dsPos->second.dataPtr, tempPtr, dataLen ) == 0) ) {
			return;	// ! New value matches the old, don't update.
		}
	}
	
	XMP_Uns8 * dataPtr = (XMP_Uns8*) malloc ( dataLen );
	if ( dataPtr == 0 ) XMP_Throw ( "Out of memory", kXMPErr_NoMemory );
	memcpy ( dataPtr, tempPtr, dataLen );	// AUDIT: Safe, malloc'ed dataLen bytes above.
	
	DataSetInfo dsInfo ( 2, dsNum, dataLen, dataPtr );

	if ( dsPos != this->dataSets.end() ) {
		this->DisposeLooseValue ( dsPos->second );
		dsPos->second = dsInfo;
	} else {
		DataSetMap::value_type mapValue ( mapID, dsInfo );
		(void) this->dataSets.insert ( this->dataSets.upper_bound ( mapID ), mapValue );
	}
	
	this->changed = true;

}	// IPTC_Writer::SetDataSet_UTF8

// =================================================================================================
// IPTC_Writer::DeleteDataSet
// ==========================

void IPTC_Writer::DeleteDataSet ( XMP_Uns8 dsNum, long which /* = -1 */ )
{
	XMP_Uns16 mapID = 2000 + dsNum;	// ! Only deal with 2:xx datasets.
	DataSetMap::iterator dsBegin = this->dataSets.lower_bound ( mapID );	// Set for which == -1.
	DataSetMap::iterator dsEnd   = this->dataSets.upper_bound ( mapID );
	
	if ( dsBegin == dsEnd ) return;	// Nothing to delete.

	if ( which >= 0 ) {
		long currCount = (long) this->dataSets.count ( mapID );
		if ( which >= currCount ) return;	// Nothing to delete.
		for ( ; which > 0; --which ) ++dsBegin;
		dsEnd = dsBegin; ++dsEnd;	// ! Can't do "dsEnd = dsBegin+1"!
	}

	for ( DataSetMap::iterator dsPos = dsBegin; dsPos != dsEnd; ++dsPos ) {
		this->DisposeLooseValue ( dsPos->second );
	}

	this->dataSets.erase ( dsBegin, dsEnd );
	this->changed = true;

}	// IPTC_Writer::DeleteDataSet

// =================================================================================================
// IPTC_Writer::UpdateMemoryDataSets
// =================================
//
// Reconstruct the entire IIM block. This does not include any final alignment padding, that is an
// artifact of some specific wrappers such as Photoshop image resources.

void IPTC_Writer::UpdateMemoryDataSets()
{
	if ( ! this->changed ) return;

	DataSetMap::iterator dsPos;
	DataSetMap::iterator dsEnd = this->dataSets.end();
	
	if ( kUTF8_Mode == kUTF8_LosslessMode ) {
		if ( this->utf8Encoding ) {
			if ( ! this->CheckRoundTripLoss() ) this->ConvertToLocal();
		} else {
			if ( this->CheckRoundTripLoss() ) this->ConvertToUTF8();
		}
	}
	
	// Compute the length of the new IIM block. All DataSets other than 1:90 and 2:xx are preserved
	// as-is. If local text is used then 1:90 is omitted, if UTF-8 text is used then 1:90 is written
	// to say so. The map key of (record*1000 + dataset) provides the desired overall order.

	XMP_Uns32 newLength = (5+2);	// We always write 2:00 for the IIM version.
	if ( this->utf8Encoding ) newLength += (5+3);	// For 1:90, if written.
	
	for ( dsPos = this->dataSets.begin(); dsPos != dsEnd; ++dsPos ) {	// Accumulate the other sizes.
		const XMP_Uns16 mapID = dsPos->first;
		if ( (mapID == 1090) || (mapID == 2000) ) continue;	// Already dealt with 1:90 and 2:00.
		XMP_Uns32 dsLen = dsPos->second.dataLen;
		newLength += (5 + dsLen);
		if ( dsLen > 0x7FFF ) newLength += 4;	// We always use a 4 byte extended length for big values.
	}
		
	// Allocate the new IIM block.
	
	XMP_Uns8* newContent = (XMP_Uns8*) malloc ( newLength );
	if ( newContent == 0 ) XMP_Throw ( "Out of memory", kXMPErr_NoMemory );
	
	XMP_Uns8* dsPtr = newContent;

	// Write the record 0 DataSets. There should not be any, but let's be safe.
	
	for ( dsPos = this->dataSets.begin(); dsPos != dsEnd; ++dsPos ) {
		const DataSetInfo & currDS = dsPos->second;
		if ( currDS.recNum > 0 ) break;
		dsPtr = AppendDataSet ( dsPtr, currDS );
	}
	
	// Write 1:90 then any other record 1 DataSets.
	
	if ( this->utf8Encoding ) {	// Write 1:90 only if text is UTF-8.
		memcpy ( dsPtr, "\x1C\x01\x5A\x00\x03\x1B\x25\x47", (5+3) );	// AUDIT: Within range of allocation.
		dsPtr += (5+3);
	}
	
	for ( ; dsPos != dsEnd; ++dsPos ) {
		const DataSetInfo & currDS = dsPos->second;
		if ( currDS.recNum > 1 ) break;
		XMP_Assert ( currDS.recNum == 1 );
		if ( currDS.dsNum == 90 ) continue;
		dsPtr = AppendDataSet ( dsPtr, currDS );
	}
	
	// Write 2:00 then all of the other DataSets from all records.
	
	if ( this->utf8Encoding ) {
		// Start with 2:00 for version 4.
		memcpy ( dsPtr, "\x1C\x02\x00\x00\x02\x00\x04", (5+2) );	// AUDIT: Within range of allocation.
		dsPtr += (5+2);
	} else {
		// Start with 2:00 for version 2.
		// *** We should probably write version 4 all the time. This is a late CS3 change, don't want
		// *** to risk breaking other apps that might be strict about version checking.
		memcpy ( dsPtr, "\x1C\x02\x00\x00\x02\x00\x02", (5+2) );	// AUDIT: Within range of allocation.
		dsPtr += (5+2);
	}
	
	for ( ; dsPos != dsEnd; ++dsPos ) {
		const DataSetInfo & currDS = dsPos->second;
		XMP_Assert ( currDS.recNum > 1 );
		if ( dsPos->first == 2000 ) continue;	// Check both the record number and DataSet number.
		dsPtr = AppendDataSet ( dsPtr, currDS );
	}
	
	XMP_Assert ( dsPtr == (newContent + newLength) );
	
	// Parse the new block, it is the best way to reset internal info and rebuild the map. 
	
	this->ParseMemoryDataSets ( newContent, newLength, false );	// Don't make another copy of the content.
	XMP_Assert ( this->iptcLength == newLength );
	this->ownedContent = (newLength > 0);	// We really do own the new content, if not empty.

}	// IPTC_Writer::UpdateMemoryDataSets

// =================================================================================================
// IPTC_Writer::ConvertToUTF8
// ==========================
//
// Convert the values of existing text DataSets to UTF-8. For now we only accept text DataSets.

void IPTC_Writer::ConvertToUTF8()
{
	XMP_Assert ( ! this->utf8Encoding );
	std::string utf8Str;

	DataSetMap::iterator dsPos = this->dataSets.begin();
	DataSetMap::iterator dsEnd = this->dataSets.end();
	
	for ( ; dsPos != dsEnd; ++dsPos ) {

		DataSetInfo & dsInfo = dsPos->second;

		ReconcileUtils::LocalToUTF8 ( dsInfo.dataPtr, dsInfo.dataLen, &utf8Str );
		this->DisposeLooseValue ( dsInfo );

		dsInfo.dataLen = (XMP_Uns32)utf8Str.size();
		dsInfo.dataPtr = (XMP_Uns8*) malloc ( dsInfo.dataLen );
		if ( dsInfo.dataPtr == 0 ) XMP_Throw ( "Out of memory", kXMPErr_NoMemory );
		memcpy ( dsInfo.dataPtr, utf8Str.data(), dsInfo.dataLen );	// AUDIT: Safe, malloc'ed dataLen bytes above.

	}
	
	this->utf8Encoding = true;

}	// IPTC_Writer::ConvertToUTF8

// =================================================================================================
// IPTC_Writer::ConvertToLocal
// ===========================
//
// Convert the values of existing text DataSets to local. For now we only accept text DataSets.

void IPTC_Writer::ConvertToLocal()
{
	XMP_Assert ( this->utf8Encoding );
	std::string localStr;

	DataSetMap::iterator dsPos = this->dataSets.begin();
	DataSetMap::iterator dsEnd = this->dataSets.end();
	
	for ( ; dsPos != dsEnd; ++dsPos ) {

		DataSetInfo & dsInfo = dsPos->second;

		ReconcileUtils::UTF8ToLocal ( dsInfo.dataPtr, dsInfo.dataLen, &localStr );
		this->DisposeLooseValue ( dsInfo );

		dsInfo.dataLen = (XMP_Uns32)localStr.size();
		dsInfo.dataPtr = (XMP_Uns8*) malloc ( dsInfo.dataLen );
		if ( dsInfo.dataPtr == 0 ) XMP_Throw ( "Out of memory", kXMPErr_NoMemory );
		memcpy ( dsInfo.dataPtr, localStr.data(), dsInfo.dataLen );	// AUDIT: Safe, malloc'ed dataLen bytes above.

	}
	
	this->utf8Encoding = false;

}	// IPTC_Writer::ConvertToLocal

// =================================================================================================
// IPTC_Writer::CheckRoundTripLoss
// ===============================
//
// See if we still need UTF-8 because of round-trip loss. Returns true if there is loss.

bool IPTC_Writer::CheckRoundTripLoss()
{
	XMP_Assert ( this->utf8Encoding );
	std::string localStr, rtStr;

	DataSetMap::iterator dsPos = this->dataSets.begin();
	DataSetMap::iterator dsEnd = this->dataSets.end();
	
	for ( ; dsPos != dsEnd; ++dsPos ) {

		DataSetInfo & dsInfo = dsPos->second;

		XMP_StringPtr utf8Ptr = (XMP_StringPtr) dsInfo.dataPtr;
		XMP_StringLen utf8Len = dsInfo.dataLen;

		ReconcileUtils::UTF8ToLocal ( utf8Ptr, utf8Len, &localStr );
		ReconcileUtils::LocalToUTF8 ( localStr.data(), localStr.size(), &rtStr );

		if ( (rtStr.size() != utf8Len) || (memcmp ( rtStr.data(), utf8Ptr, utf8Len ) != 0) ) {
			return true; // Had round-trip loss, keep UTF-8.
		}

	}
	
	return false;	// No loss.

}	// IPTC_Writer::CheckRoundTripLoss

// =================================================================================================
